//
// "$Id: MenuTabs.h 401 2006-11-11 03:19:07Z mike $"
//
//   Menu-driven tabs widget definitions.
//
//   Copyright 2006 by Michael Sweet.
//
//   This program is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation; either version 2, or (at your option)
//   any later version.
//
//   This program is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//

#ifndef MenuTabs_h
#  define MenuTabs_h

//
// Include necessary headers...
//

#  include <FL/Fl_Tabs.H>


//
// Widget class...
//

class MenuTabs : public Fl_Tabs		//// Menu-driven tabs widget
{
  bool		auto_resize_;		// Automatically resize the window?
  int		menu_x_,		// X position of menu button
		menu_w_;		// Width of menu button

  static void	resize_cb(void *data);

  protected:

  void		draw();

  public:

		MenuTabs(int X, int Y, int W, int H, const char *L = 0);
		~MenuTabs();
  void		auto_resize(bool a) { auto_resize_ = a; }
  bool		auto_resize() const { return (auto_resize_); }
  int		handle(int event);
  void		resize(int X, int Y, int W, int H);
  int		value(Fl_Widget *g);
  Fl_Widget	*value() { return (Fl_Tabs::value()); }
};

#endif // !MenuTabs_h

//
// End of "$Id: MenuTabs.h 401 2006-11-11 03:19:07Z mike $".
//
