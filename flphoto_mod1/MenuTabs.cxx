//
// "$Id: MenuTabs.cxx 401 2006-11-11 03:19:07Z mike $"
//
//   Menu-driven tabs widget implementation.
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
//   MenuTabs::MenuTabs() - Create a new MenuTabs widget.
//   MenuTabs::draw()     - Redraw the widget.
//   MenuTabs::handle()   - Handle clicks in the widget.
//   MenuTabs::resize()   - Resize the widget.
//

//
// Include necessary headers...
//

#include "MenuTabs.h"
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Menu_Item.H>
#include <FL/fl_draw.H>
#include <string.h>


//
// 'MenuTabs::MenuTabs()' - Create a new MenuTabs widget.
//

MenuTabs::MenuTabs(int        X,	// I - X position
                   int        Y,	// I - Y position
		   int        W,	// I - Width
		   int        H,	// I - Height
		   const char *L)	// I - Label
  : Fl_Tabs(X, Y, W, H, L)
{
  box(FL_THIN_DOWN_BOX);
  color(FL_GRAY - 1);

  menu_x_ = menu_w_ = 0;
}


//
// 'MenuTabs::~MenuTabs()' - Destroy a MenuTabs widget.
//

MenuTabs::~MenuTabs()
{
}


//
// 'MenuTabs::draw()' - Redraw the widget.
//

void
MenuTabs::draw()
{
  draw_box(box(), x(), y() + 13, w(), h() - 13, color());

  Fl_Widget *v = value();

  if (v)
  {
    fl_font(labelfont(), labelsize());
    menu_w_ = (int)fl_width(v->label()) + 45;
    menu_x_ = x() + (w() - menu_w_) / 2;

    draw_box(FL_UP_BOX, menu_x_, y(), menu_w_, 25, FL_GRAY);
    fl_color(active_r() ? labelcolor() : fl_inactive(labelcolor()));
    fl_draw(v->label(), menu_x_ + 10, y(), menu_w_ - 35, 25, FL_ALIGN_LEFT);

    int X = menu_x_ + menu_w_ - 15;
    fl_polygon(X, y() + 10, X + 3, y() + 7, X + 6, y() + 10);
    fl_polygon(X + 6, y() + 14, X + 3, y() + 17, X, y() + 14);

    fl_color(fl_darker(color()));
    fl_yxline(X - 7, y() + 4, y() + 20);

    fl_color(fl_lighter(color()));
    fl_yxline(X - 6, y() + 4, y() + 20);

    fl_push_clip(x(), y() + 35, w(), h() - 35);
      draw_child(*v);
    fl_pop_clip();
  }
}


//
// 'MenuTabs::handle()' - Handle clicks in the widget.
//

int					// O - 1 if handled, 0 otherwise
MenuTabs::handle(int event)		// I - Event
{
  if (event == FL_PUSH && Fl::event_x() >= menu_x_ &&
      Fl::event_x() < (menu_x_ + menu_w_) && Fl::event_y() >= y() &&
      Fl::event_y() < (y() + 25))
  {
    // Click on menu button...
    int			i;		// Looping var
    Fl_Menu_Item	*items;		// Array of menu items
    const Fl_Menu_Item	*picked;	// Picked item


    items = new Fl_Menu_Item[children() + 1];
    memset(items, 0, (children() + 1) * sizeof(Fl_Menu_Item));

    for (i = 0, picked = 0; i < children(); i ++)
    {
      items[i].text = child(i)->label();

      if (child(i)->visible())
        picked = items + i;
    }

    picked = items->pulldown(menu_x_, y(), menu_w_, 25, picked);

    if (picked)
    {
      value(child(picked - items));
      do_callback();
    }

    delete[] items;

    return (1);
  }
  else
    return (Fl_Group::handle(event));
}


//
// 'MenuTabs::resize()' - Resize the widget.
//

void
MenuTabs::resize(int X,			// I - New X
                 int Y,			// I - New Y
		 int W,			// I - New width
		 int H)			// I - New height
{
  for (int i = 0; i < children(); i ++)
  {
    Fl_Widget *c = child(i);

    c->resize(X, Y + 25, auto_resize_ ? c->w() : W, c->h());
  }

  menu_x_ = X + (W - menu_w_) / 2;

  Fl_Widget::resize(X, Y, W, H);

  redraw();
}


//
// 'MenuTabs::value()' - Change the visible tab.
//

int					// O - 1 if something changed
MenuTabs::value(Fl_Widget *g)		// I - Group widget to show
{
  if (auto_resize_)
  {
    // Resize the window to fit...
    Fl_Window *win = window();
    int W = win->w() + g->w() - w();
    int H = win->h() + g->y() + g->h() - y() - h();

    win->resize(win->x(), win->y(), W, H);
  }

  return (Fl_Tabs::value(g));
}


//// Test code
#ifdef TEST
#  undef TEST
#  include "SpringWindow.cxx"
#  include <FL/Fl_Box.H>
#  include <FL/Fl_Button.H>


int					// O - Exit code
main(int  argc,				// I - Number of command-line args
     char *argv[])			// I - Command-line arguments
{
  SpringWindow win(300, 235, "MenuTabs");
  MenuTabs tabs(10, 10, 280, 180);
  Fl_Group tab1(10, 35, 220, 45, "Tab 1");
    Fl_Button button1(20, 45, 200, 25, "Button 1");
    tab1.end();
  Fl_Group tab2(10, 35, 220, 80, "Tab 2");
    Fl_Button button2a(20, 45, 200, 25, "Button 2a");
    Fl_Button button2b(20, 80, 200, 25, "Button 2b");
    tab2.end();
  Fl_Group tab3(10, 35, 280, 155, "Tab 3");
    Fl_Button button3a(20, 45, 200, 25, "Button 3a");
    Fl_Button button3b(80, 155, 200, 25, "Button 3b");
    tab3.end();
  tabs.end();

  Fl_Group bar(10, 200, 280, 25);
    Fl_Box resizebox(10, 200, 90, 25);
    Fl_Button ok(100, 200, 90, 25, "OK");
    Fl_Button cancel(200, 200, 90, 25, "Cancel");
    bar.resizable(&resizebox);
    bar.end();

  win.resizable(&tabs);
  win.end();

  tabs.value(&tab1);

  win.show(argc, argv);

  return (Fl::run());
}
#endif // TEST


//
// End of "$Id: MenuTabs.cxx 401 2006-11-11 03:19:07Z mike $".
//
