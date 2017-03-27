#ifndef IMGUI_HPP_INCLUDED
#define IMGUI_HPP_INCLUDED

namespace imgui
{

	bool		begin_window( const char* pTitle );
	void		end_window();

	bool		button( const char* pText );
	void		text( const char* pText );

	void		quit();

}

#endif
