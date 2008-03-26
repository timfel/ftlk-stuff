//
// "$Id: SpringWindow.cxx 401 2006-11-11 03:19:07Z mike $"
//
//   Spring-loaded window implementation.
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
// Contents:
//
//   SpringWindow::SpringWindow()  - Create a spring-loaded window.
//   SpringWindow::~SpringWindow() - Destroy a spring-loaded window.
//   SpringWindow::resize()        - Resize/reposition the window via animation.
//   SpringWindow::resize_cb()     - Animate a resize.
//

//
// Include necessary headers...
//

#include <FL/Fl.H>
#include "SpringWindow.h"
#include "debug.h"


//
// 'SpringWindow::SpringWindow()' - Create a spring-loaded window.
//

SpringWindow::SpringWindow(
    int        X,			// I - X position
    int        Y,			// I - Y position
    int        W,			// I - Width
    int        H,			// I - Height
    const char *L)			// I - Window title
  : Fl_Double_Window(X, Y, W, H, L)
{
  target_w_     = 0;
  target_h_     = 0;
  in_resize_cb_ = false;
}


SpringWindow::SpringWindow(
    int        W,			// I - Width
    int        H,			// I - Height
    const char *L)			// I - Window title
  : Fl_Double_Window(W, H, L)
{
  target_w_     = 0;
  target_h_     = 0;
  in_resize_cb_ = false;
}


//
// 'SpringWindow::~SpringWindow()' - Destroy a spring-loaded window.
//

SpringWindow::~SpringWindow()
{
  Fl::remove_timeout(resize_cb, this);
}


//
// 'SpringWindow::resize()' - Resize/reposition the window via animation.
//

void
SpringWindow::resize(int X,		// I - X position
                     int Y,		// I - Y position
		     int W,		// I - Width
                     int H)		// I - Height
{
  DEBUG_printf(("SpringWindow::resize(X=%d, Y=%d, W=%d, H=%d)\n", X, Y, W, H));
  DEBUG_printf(("    shown()=%d, visible()=%d\n", shown(), visible()));

  if (!in_resize_cb_ && shown() && visible() && X == x() && Y == y() &&
      (W != w() || H != h()))
  {
    target_w_ = W;
    target_h_ = H;

    DEBUG_puts("    adding timeout");

    Fl::remove_timeout(resize_cb, this);
    Fl::add_timeout(0.05, resize_cb, this);
  }
  else
    Fl_Double_Window::resize(X, Y, W, H);
}


//
// 'SpringWindow::resize_cb()' - Animate a resize.
//

void
SpringWindow::resize_cb(void *data)	// I - Spring-loaded window
{
  SpringWindow	*sw;			// Spring-loaded window
  int		dW, dH;			// Change in size
  int		W, H;			// New size


  DEBUG_puts("SpringWindow::resize_cb()");

  sw = (SpringWindow *)data;

  dW = sw->target_w_ - sw->w();
  dH = sw->target_h_ - sw->h();

  if (dW < -1 || dW > 1)
    dW /= 2;

  if (dH < -1 || dH > 1)
    dH /= 2;

  if (dW || dH)
  {
    W = sw->w() + dW;
    H = sw->h() + dH;

    DEBUG_printf(("    dW=%d, dH=%d\n", dW, dH));
    DEBUG_printf(("    W=%d, H=%d\n", W, H));

    sw->in_resize_cb_ = true;
    sw->size_range(W, H, W, H);
    sw->force_size(W, H);
    sw->in_resize_cb_ = false;

    if (W != sw->target_w_ || H != sw->target_h_)
    {
      DEBUG_puts("    adding timeout");
      Fl::add_timeout(0.05, resize_cb, data);
    }
  }
}


//// Test code
#ifdef TEST
#  include <FL/Fl_Box.H>
#  include <FL/Fl_Button.H>

void
button_cb(Fl_Widget *wi,		// I - Button widget
          void      *d)			// I - Data (unused)
{
  SpringWindow	*sw = (SpringWindow *)wi->window();

  sw->size(wi->w() * 5, wi->h() * 5);
}


int					// O - Exit status
main(int  argc,				// I - Number of command-line args
     char *argv[])			// I - Command-line args
{
  SpringWindow win(200, 200, "SpringWindow");

  Fl_Button b1(10, 10, 40, 40);
  b1.callback(button_cb);

  Fl_Button b2(100, 10, 80, 40);
  b2.callback(button_cb);

  Fl_Button b3(10, 100, 40, 80);
  b3.callback(button_cb);

  Fl_Button b4(100, 100, 80, 80);
  b4.callback(button_cb);

  Fl_Box box(190, 190, 10, 10);

  win.resizable(&box);
  win.end();
  win.show(argc, argv);

  return (Fl::run());
}
#endif // TEST


//
// End of "$Id: SpringWindow.cxx 401 2006-11-11 03:19:07Z mike $".
//
