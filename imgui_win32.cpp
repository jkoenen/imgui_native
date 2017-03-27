#include "imgui.hpp"

#include <assert.h>
#include <stdlib.h>

#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#include <windows.h>

#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

void test_ui();

namespace imgui
{
	enum WidgetType
	{
		WidgetType_Window,
		WidgetType_Button,
		WidgetType_Text,
		WidgetType_Count
	};

	typedef uintptr_t WidgetId;

	struct Rect2f
	{
		float		x;
		float		y;
		float		w;
		float		h;
	};

	static inline bool isRectEqual( Rect2f lhs, Rect2f rhs ) { return lhs.x == rhs.x && lhs.y == rhs.y && lhs.w == rhs.w && lhs.h == rhs.h; }

	struct Widget
	{
		WidgetId		id;
		WidgetId		parentId;
		WidgetType		type;
		char			text[ 128u ];
		Rect2f			rect;		
	};

	static Widget	s_widgets[ 128u ];
	static unsigned int	s_widgetCount = 0u;
	static WidgetId		s_widgetStack[ 16u ];
	static unsigned int	s_widgetStackSize = 0u;

	struct WidgetWindow
	{
		WidgetId		id;
		unsigned int	lastUpdateId;
		unsigned int	controlId;

		// event data:
		bool			wasPressed;
		bool			wasClosed;

		// actual control:
		HWND			hWnd;
		Rect2f			rect;
		char			text[ 128u ];
		bool			isVisible;
	};
	static WidgetWindow	s_widgetWindows[ 128u ];	
	static unsigned int	s_widgetWindowCount = 0u;

	static bool s_quit = false;

	WidgetWindow* findWidgetWindow( WidgetId id )
	{
		for( size_t i = 0u; i < s_widgetWindowCount; ++i )
		{
			if( s_widgetWindows[ i ].hWnd != NULL && s_widgetWindows[ i ].id == id )
			{
				return &s_widgetWindows[ i ];
			}
		}
		return nullptr;
	}

	WidgetWindow* findWidgetWindowByHwnd( HWND hWnd )
	{
		for( size_t i = 0u; i < s_widgetWindowCount; ++i )
		{
			if( s_widgetWindows[ i ].hWnd == hWnd )
			{
				return &s_widgetWindows[ i ];
			}
		}
		return nullptr;
	}

	WidgetWindow* findWidgetWindowByControlId( unsigned int id )
	{
		for( size_t i = 0u; i < s_widgetWindowCount; ++i )
		{
			if( s_widgetWindows[ i ].controlId == id )
			{
				return &s_widgetWindows[ i ];
			}
		}
		return nullptr;
	}

	Widget* add_widget( WidgetType type, WidgetId id )
	{
		if( s_widgetCount >= _countof( s_widgets ) )
		{
			return nullptr;
		}
		Widget* pResult = &s_widgets[ s_widgetCount++ ];
		ZeroMemory( pResult, sizeof( *pResult ) );
		pResult->type	= type;
		pResult->id		= id;

		if( s_widgetStackSize > 0u )
		{
			pResult->parentId = s_widgetStack[ s_widgetStackSize -1u ];
		}
		return pResult;
	}

	bool begin_window( const char* pTitle )
	{
		// 
		Widget* pWidget = add_widget( WidgetType_Window, (uintptr_t)pTitle );
		assert( pWidget );		
		strcpy_s( pWidget->text, sizeof( pWidget->text ), pTitle );
		assert( s_widgetStackSize < _countof( s_widgetStack ) );
		s_widgetStack[ s_widgetStackSize++ ] = pWidget->id;

		WidgetWindow* pWindow = findWidgetWindow( pWidget->id );
		if( pWindow != nullptr )
		{
			return !pWindow->wasClosed;
		}
		return true;
	}

	void end_window()
	{
		s_widgetStackSize--;
	}
	
	bool button( const char* pText )
	{
		Widget* pWidget = add_widget( WidgetType_Button, (uintptr_t)pText );
		assert( pWidget );
		strcpy_s( pWidget->text, sizeof( pWidget->text ), pText );

		// 
		WidgetWindow* pWindow = findWidgetWindow( pWidget->id );
		if( pWindow != nullptr )
		{
			return pWindow->wasPressed;
		}
		return false;
	}

	void text( const char* pText )
	{
		Widget* pWidget = add_widget( WidgetType_Text, (uintptr_t)pText );
		assert( pWidget );
		strcpy_s( pWidget->text, sizeof( pWidget->text ), pText );
	}

	void quit()
	{
		s_quit = true;
		PostQuitMessage( 0 );
	}

	HINSTANCE s_instance = NULL;
	HBRUSH s_hbrBkgnd = NULL;

	static int WndProcA( _In_ HWND hWnd, _In_ UINT msg, _In_ WPARAM wParam, _In_ LPARAM lParam )
	{
		switch( msg )
		{
		case WM_COMMAND:
			{
				// find widget window by hWnd:
				WidgetWindow* pControlWindow = findWidgetWindowByControlId( LOWORD(wParam) );
				if( pControlWindow != nullptr )
				{
					pControlWindow->wasPressed = true;
				}
			}
			break;

		case WM_CTLCOLORSTATIC:
			{
				HDC hdcStatic = (HDC)wParam;
				SetTextColor( hdcStatic, GetSysColor( COLOR_WINDOWTEXT ) );
				//SetBkMode( hdcStatic, TRANSPARENT );
				return (LRESULT)GetStockObject( COLOR_BACKGROUND );
			}

		case WM_CLOSE:
			{
				WidgetWindow* pWindow = findWidgetWindowByHwnd( hWnd );
				if( pWindow != nullptr )
				{
					pWindow->wasClosed = true;
				}
			}			
			break;
		}
		return DefWindowProcA( hWnd, msg, wParam, lParam );
	}

	static void init( HINSTANCE instance )
	{
		s_instance = instance;

		WNDCLASSEXA wc;
		ZeroMemory( &wc, sizeof( wc ) );
		wc.cbSize			= sizeof( wc );
		wc.style			= CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc		= (WNDPROC)WndProcA;
		wc.cbClsExtra		= 0u;
		wc.cbWndExtra		= sizeof( WidgetId );
		wc.hInstance		= s_instance;
		wc.hIcon			= NULL;
		wc.hCursor			= LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground	= (HBRUSH) GetStockObject(WHITE_BRUSH);
		wc.lpszMenuName		= NULL;
		wc.lpszClassName	= "imgui_window";
		wc.hIconSm			= NULL;
		if( !RegisterClassExA( &wc ) )
		{
			assert( false );
		}
	}

	static unsigned int s_controlId = 100;

	WidgetWindow* createWidgetWindow( const Widget& widget )
	{
		if( s_widgetWindowCount >= _countof( s_widgetWindows ) )
		{
			return nullptr;
		}
		WidgetWindow* pResult = &s_widgetWindows[ s_widgetWindowCount++ ];
		ZeroMemory( pResult, sizeof( *pResult ) );
		pResult->id = widget.id;
		strcpy_s( pResult->text, sizeof( pResult->text ), widget.text );
		pResult->rect = widget.rect;

		// :TODO: go up the stack until we find the actual window..
		WidgetWindow* pParent = findWidgetWindow( widget.parentId );		
		
		switch( widget.type )
		{
		case WidgetType_Window:
			pResult->hWnd = CreateWindowA( "imgui_window", widget.text, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, s_instance, NULL );
			break;

		case WidgetType_Button:
			{
				if( pParent == nullptr )
				{
					return nullptr;
				}
				pResult->controlId = s_controlId++;
				pResult->hWnd = CreateWindowA( "button", widget.text, WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON | WS_CLIPSIBLINGS, (int)widget.rect.x, (int)widget.rect.y, (int)widget.rect.w, (int)widget.rect.h, pParent->hWnd, (HMENU)pResult->controlId, s_instance, NULL );
				pResult->isVisible = true;
			}

		case WidgetType_Text:
			{
				if( pParent == nullptr )
				{
					return nullptr;
				}
				pResult->hWnd = CreateWindowA( "static", widget.text, WS_CHILD | WS_VISIBLE | SS_SIMPLE | WS_CLIPSIBLINGS, (int)widget.rect.x, (int)widget.rect.y, (int)widget.rect.w, (int)widget.rect.h, pParent->hWnd, NULL, s_instance, NULL );
				pResult->isVisible = true;
			}
		}


		return pResult;
	}

	static unsigned int s_updateCounter = 0u;

	void update()
	{
		// clear buffers
		s_widgetCount = 0u;
		s_widgetStackSize = 0u;

		// run ui code:
		test_ui();

		// :TODO: layout the ui / determine positions..
		Rect2f rect;
		rect.x = 5.0f;
		rect.y = 5.0f;
		rect.w = 100.0f;
		rect.h = 30.0f;

		for( size_t i = 0u; i < s_widgetCount; ++i )
		{
			Widget* pWidget = &s_widgets[ i ];
			pWidget->rect = rect;

			rect.y += rect.h;
			rect.y += 5.0f;
		}

		// sync widgets to actual windows	
		s_updateCounter++;
		for( size_t i = 0u; i < s_widgetCount; ++i )
		{
			Widget* pWidget = &s_widgets[ i ];

			// check if we already have this window.. 
			WidgetWindow* pWindow = findWidgetWindow( pWidget->id );
			if( pWindow == nullptr )
			{
				// new window
				pWindow = createWidgetWindow( *pWidget );
				if( pWindow == nullptr )
				{
					continue;
				}
			}

			pWindow->lastUpdateId = s_updateCounter;

			// reset event notifications:
			pWindow->wasPressed = false;

			// :TODO: check parent
			
			// :TODO: check position
			if( !isRectEqual( pWindow->rect, pWidget->rect ) && pWidget->type != WidgetType_Window )
			{
				SetWindowPos( pWindow->hWnd, NULL, (int)pWidget->rect.x, (int)pWidget->rect.y, (int)pWidget->rect.w, (int)pWidget->rect.h, 0 );
			}

			// check text:
			if( strcmp( pWindow->text, pWidget->text ) != 0 )
			{
				SetWindowTextA( pWindow->hWnd, pWidget->text );
				strcpy_s( pWindow->text, sizeof( pWindow->text ), pWidget->text );
			}
		}

		// remove all unused windows:
		for( size_t i = 0u; i < s_widgetWindowCount; ++i )
		{
			WidgetWindow* pWidgetWindow = &s_widgetWindows[ i ];
			if( pWidgetWindow->hWnd == NULL )
			{
				continue;
			}
			if( pWidgetWindow->lastUpdateId != s_updateCounter )
			{
				DestroyWindow( pWidgetWindow->hWnd );
				pWidgetWindow->hWnd = NULL;
				continue;
			}
			if( !pWidgetWindow->isVisible )
			{
				ShowWindow( pWidgetWindow->hWnd, SW_SHOW );
				UpdateWindow( pWidgetWindow->hWnd );
				pWidgetWindow->isVisible = true;
			}
		}
	}


}

int __stdcall WinMain( _In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd )
{
	imgui::init( hInstance );
	imgui::update();

	MSG msg;
	ZeroMemory( &msg, sizeof( msg ) );
	while( msg.message != WM_QUIT )
	{
		if( GetMessage( &msg, NULL, 0U, 0U ) )
		{
			TranslateMessage( &msg );
			DispatchMessage( &msg );

			imgui::update();			
		}
	}
	return 0;
}
