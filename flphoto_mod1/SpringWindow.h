//
// "$Id: SpringWindow.h 401 2006-11-11 03:19:07Z mike $"
//
//   Spring-loaded window definitions.
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

#ifndef SpringWindow_h
#  define SpringWindow_h

//
// Include necessary headers...
//

#  include <FL/Fl_Double_Window.H>


//
// Window class...
//

class SpringWindow : public Fl_Double_Window	//// Spring-loaded window class
{
  int		target_w_,			// Target width
		target_h_;			// Target height
  bool		in_resize_cb_;			// In the resize callback?

  static void	resize_cb(void *data);

  public:

		SpringWindow(int X, int Y, int W, int H, const char *L = 0);
		SpringWindow(int W, int H, const char *L = 0);
  virtual	~SpringWindow();
  void		force_size(int W, int H) { Fl_Double_Window::resize(x(), y(), W, H); }
  virtual void	resize(int X, int Y, int W, int H);
  void		size(int W, int H) { resize(x(), y(), W, H); }
};

#endif // !SpringWindow_h

//
// End of "$Id: SpringWindow.h 401 2006-11-11 03:19:07Z mike $".
//
