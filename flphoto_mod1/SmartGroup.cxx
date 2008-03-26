//
// "$Id: SmartGroup.cxx 401 2006-11-11 03:19:07Z mike $"
//
//   Smart group widget code.
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
//   SmartGroup::SmartGroup()  - Create a new widget.
//   SmartGroup::~SmartGroup() - Destroy a widget.
//   SmartGroup::do_layout()   - Layout the widgets in the group and resize
//                               to fit.
//   SmartGroup::resize()      - Resize/reposition the widget.
//

//
// Include necessary headers...
//

#include "SmartGroup.h"
#include <FL/Fl_Window.H>
#include "debug.h"


//
// 'SmartGroup::SmartGroup()' - Create a new widget.
//

SmartGroup::SmartGroup(
    int        X,			// I - X position
    int        Y,			// I - Y position
    int        W,			// I - Width
    int        H,			// I - Height
    const char *L) :			// I - Label
  Fl_Group(X, Y, W, H, L)
{
  child_align(FL_ALIGN_TOP_LEFT);
}


//
// 'SmartGroup::~SmartGroup()' - Destroy a widget.
//

SmartGroup::~SmartGroup()
{
}


//
// 'SmartGroup::do_layout()' - Layout the widgets in the group and resize
//                             to fit.
//

void
SmartGroup::do_layout(
    Fl_Align a,				// I - Widget alignment
    int      margin,			// I - Inner margin
    int      hspacing,			// I - Horizontal spacing between widgets
    int      vspacing,			// I - Vertical spacing between widgets
    bool     resize_win)		// I - Resize the parent window?
{
  int		i,			// Looping var
		ww, hh,			// Label width and height
		xx, yy,			// Current position
		lasty,			// Last Y position
		max_height,		// Maximum height of widget
  		max_label,		// Maximum label width
		max_widget,		// Maximum widget width
		req_width,		// Required width
		req_height;		// Required height
  Fl_Widget	*c;			// Current widget
  Fl_Align	ca;			// Current alignment


  // Start by computing the required width and height...
  for (i = 0, max_height = 0, max_label = 0, max_widget = 0, req_width = 0,
           req_height = 0, lasty = 0;
       i < children();
       i ++)
  {
    // Get the next child...
    c  = child(i);
    ca = c->align();

    if (!c->visible())
      continue;

    // Figure out the label dimensions...
    ww = 0;
    hh = 0;

    c->measure_label(ww, hh);

    // If the label is aligned to the left, update the maximum label
    // width as needed.  If the label is inside the widget and the
    // width is > 0, update the widget width (for buttons)...
    if (ca == FL_ALIGN_LEFT && ww > max_label)
      max_label = ww;
    else if ((!ca || (ca & FL_ALIGN_INSIDE)) && ww > 0 && c->h() == 25)
      c->resize(c->x(), c->y(), ww + 20, c->h());

    // Update the maximum widget width and required width/height...
    if (a == FL_ALIGN_TOP)
    {
      // Stack widgets...
      if (c->label() && c->w() > max_widget)
	max_widget = c->w();
      else if (c->w() > req_width)
        req_width = c->w();

      req_height += c->h() + vspacing;
    }
    else
    {
      // Pack widgets...
      if (c->y() > lasty)
      {
        max_height = c->h();
        max_widget = 0;
	lasty      = c->y();

        req_height += max_height + vspacing;
      }
      else if (c->h() > max_height)
      {
        req_height += c->h() - max_height;
        max_height = c->h();
      }

      if (ca && !(ca & FL_ALIGN_INSIDE))
        max_widget += ww;

      max_widget += c->w() + hspacing;

      if (max_widget > req_width)
        req_width = max_widget;
    }
  }

  if (a == FL_ALIGN_TOP)
  {
    ww = max_label + max_widget;
    if (ww > req_width)
      req_width = ww;

    req_width += 2 * margin;
  }
  else if (req_width > 0)
    req_width += 2 * margin - hspacing;

  if (req_height > 0)
    req_height += 2 * margin - vspacing;

//  printf("req_width=%d, req_height=%d, children()=%d, label()=\"%s\"\n",
//         req_width, req_height, children(), label());

  if (req_width <= 0 || req_height <= 0)
    return;

  if (resize_win && window())
  {
    Fl_Window *win = window();

    win->size(win->w() + req_width - w(), win->h() + req_height - h());
  }

  if (a != FL_ALIGN_TOP && req_width < w())
    Fl_Widget::resize(x(), y(), w(), req_height);
  else
    Fl_Widget::resize(x(), y(), req_width, req_height);

  // Then move things around based on the alignment...
  switch (a)
  {
    case FL_ALIGN_TOP :
        // Align the widgets in a form arrangement like this:
	//
	//   +--------------------------------------+
	//   |      Label: [widget]                 |
	//   | Long Label: [widget with long label] |
	//   +--------------------------------------+

        for (i = 0, xx = x() + max_label + margin, yy = y() + margin;
	     i < children();
	     i ++)
	{
	  c = child(i);

          if (c->label())
	    c->resize(xx, yy, c->w(), c->h());
	  else
	    c->resize(x() + req_width - margin - c->w(), yy, c->w(), c->h());

	  yy += c->h() + vspacing;
	}
	break;

    case FL_ALIGN_LEFT :
        // Left-align the widgets like this:
	//
	//   +--------------------------------------+
	//   | [widget] [widget] [widget]           |
	//   +--------------------------------------+

        for (i = 0, xx = x() + margin, yy = y() + margin,
	         lasty = child(0)->y(), max_height = 0;
	     i < children();
	     i ++)
	{
	  c = child(i);

          if (c->y() > lasty)
	  {
	    lasty      = c->y();
	    yy         += max_height + vspacing;
	    xx         = x() + margin;
	    max_height = c->h();
	  }
	  else if (c->h() > max_height)
	    max_height = c->h();

          if (c->align() && !(c->align() & FL_ALIGN_INSIDE))
	  {
	    // Figure out the label dimensions...
	    ww = 0;
	    hh = 0;

	    c->measure_label(ww, hh);
	    xx += ww;
	  }

	  c->resize(xx, yy, c->w(), c->h());

          xx += c->w() + hspacing;
	}
	break;


    case FL_ALIGN_RIGHT :
        // Right-align the widgets like this:
	//
	//   +--------------------------------------+
	//   |           [widget] [widget] [widget] |
	//   +--------------------------------------+

        for (i = 0, xx = x() + w() + margin - req_width, yy = y() + margin,
	         lasty = child(0)->y(), max_height = 0;
	     i < children();
	     i ++)
	{
	  c = child(i);

          if (c->y() > lasty)
	  {
	    lasty      = c->y();
	    yy         += max_height + vspacing;
	    xx         = x() + w() + margin - req_width;
	    max_height = c->h();
	  }
	  else if (c->h() > max_height)
	    max_height = c->h();

          if (c->align() && !(c->align() & FL_ALIGN_INSIDE))
	  {
	    // Figure out the label dimensions...
	    ww = 0;
	    hh = 0;

	    c->measure_label(ww, hh);
	    xx += ww;
	  }

	  c->resize(xx, yy, c->w(), c->h());

          xx += c->w() + hspacing;
	}
	break;

    default :
        // Any other alignment is currently undefined, don't move anything...
	break;
  }

  redraw();
}


//
// 'SmartGroup::resize()' - Resize/reposition the widget.
//

void
SmartGroup::resize(int X,		// I - X position
		   int Y,		// I - Y position
		   int W,		// I - Width
		   int H)		// I - Height
{
  int	dX = X - x(),			// Change in X
	dY = Y - y();			// Change in Y


  DEBUG_printf(("SmartGroup::resize(X=%d, Y=%d, W=%d, H=%d)\n", X, Y, W, H));
  DEBUG_printf(("    x()=%d, y()=%d, w()=%d, h()=%d, child_align()=%x\n",
        	x(), y(), w(), h(), child_align()));

  if (child_align() & FL_ALIGN_RIGHT)
    dX += W - w();
  else if (!(child_align() & FL_ALIGN_LEFT))
    dX += (W - w()) / 2;

  if (child_align() & FL_ALIGN_BOTTOM)
    dY += H - h();
  else if (!(child_align() & FL_ALIGN_TOP))
    dY += (H - h()) / 2;

  DEBUG_printf(("    dX=%d, dY=%d\n", dX, dY));

  if (dX || dY)
  {
    for (int j = 0; j < children(); j ++)
    {
      Fl_Widget *c = child(j);

      c->resize(c->x() + dX, c->y() + dY, c->w(), c->h());
    }
  }

  Fl_Widget::resize(X, Y, W, H);
}


#ifdef TEST
#  include <FL/Fl.H>
#  include <FL/Fl_Window.H>
#  include <FL/Fl_Button.H>
#  include <FL/Fl_Input.H>

int					// O - Exit status
main(int  argc,				// I - Number of command-line args
     char *argv[])			// I - Command-line arguments
{
  Fl_Window	win(300, 300, "SmartGroup");
  Fl_Group	group(10, 10, 280, 280);

  SmartGroup	lefttop(10, 10, 135, 135, "LEFT_TOP");
  Fl_Button	but1(20, 20, 25, 25, "1");
  lefttop.align(FL_ALIGN_CENTER);
  lefttop.child_align(FL_ALIGN_LEFT_TOP);
  lefttop.box(FL_THIN_DOWN_BOX);
  lefttop.color(FL_GRAY - 1);
  lefttop.end();

  SmartGroup	righttop(155, 10, 135, 135, "RIGHT_TOP");
  Fl_Button	but2(255, 20, 25, 25, "2");
  righttop.align(FL_ALIGN_CENTER);
  righttop.child_align(FL_ALIGN_RIGHT_TOP);
  righttop.box(FL_THIN_DOWN_BOX);
  righttop.color(FL_GRAY - 1);
  righttop.end();

  SmartGroup	leftbottom(10, 155, 135, 135, "LEFT_BOTTOM");
  Fl_Button	but3(20, 255, 25, 25, "3");
  leftbottom.align(FL_ALIGN_CENTER);
  leftbottom.child_align(FL_ALIGN_LEFT_BOTTOM);
  leftbottom.box(FL_THIN_DOWN_BOX);
  leftbottom.color(FL_GRAY - 1);
  leftbottom.end();

  SmartGroup	rightbottom(155, 155, 135, 135, "RIGHT_BOTTOM");
  Fl_Button	but4(255, 255, 25, 25, "4");
  rightbottom.align(FL_ALIGN_CENTER);
  rightbottom.child_align(FL_ALIGN_RIGHT_BOTTOM);
  rightbottom.box(FL_THIN_DOWN_BOX);
  rightbottom.color(FL_GRAY - 1);
  rightbottom.end();

  group.end();

  win.end();
  win.resizable(&group);

  win.show(argc, argv);

  Fl_Window win2(400, 400, "SmartGroup do_layout");
  SmartGroup top_group(10, 30, 380, 160, "FL_ALIGN_TOP");
  top_group.align(FL_ALIGN_TOP_LEFT);
  top_group.color(FL_GRAY - 1);
  top_group.box(FL_THIN_DOWN_BOX);

  Fl_Input input1(20, 40, 100, 25, "Input 1:");
  Fl_Input input2(20, 40, 100, 25, "Input 2:");
  Fl_Input input3(20, 40, 150, 25, "Long Input 3:");

  top_group.end();
  top_group.do_layout(FL_ALIGN_TOP, 10, 10, 10, false);

  SmartGroup left_group(10, 230, 380, 60, "FL_ALIGN_LEFT");
  left_group.align(FL_ALIGN_TOP_LEFT);
  left_group.color(FL_GRAY - 1);
  left_group.box(FL_THIN_DOWN_BOX);

  Fl_Input input4(20, 240, 100, 25, "Input 4:");
  Fl_Button but5(20, 240, 50, 25, "Button 5");
  Fl_Input input5(20, 275, 100, 25, "Long Input 5:");
  Fl_Button but6(20, 275, 50, 25, "Long Button 6");

  left_group.end();
  left_group.do_layout(FL_ALIGN_LEFT, 10, 10, 10, false);

  SmartGroup right_group(10, 330, 380, 60, "FL_ALIGN_RIGHT");
  right_group.align(FL_ALIGN_TOP_LEFT);
  right_group.color(FL_GRAY - 1);
  right_group.box(FL_THIN_DOWN_BOX);

  Fl_Input input6(20, 340, 100, 25, "Input 6:");
  Fl_Button but7(20, 340, 50, 25, "Button 7");
  Fl_Input input7(20, 375, 100, 25, "Long Input 7:");
  Fl_Button but8(20, 375, 50, 25, "Long Button 8");

  right_group.end();
  right_group.do_layout(FL_ALIGN_RIGHT, 10, 10, 10, false);

  win2.end();
  win2.show();

  return (Fl::run());
}
#endif // TEST


//
// End of "$Id: SmartGroup.cxx 401 2006-11-11 03:19:07Z mike $".
//
