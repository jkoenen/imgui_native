
#include "imgui.hpp"

void test_ui()
{
	if( !imgui::begin_window( "imgui_native test" ) )
	{
		imgui::quit();
		return;
	}
	static bool wasPressed = false;
	if( imgui::button( "Test" ) )
	{
		wasPressed = !wasPressed;
	}

	imgui::text( wasPressed ? "pressed" : "press it!" );

	if( imgui::button( "Quit" ) )
	{
		imgui::quit();
	}
	imgui::end_window();
}
