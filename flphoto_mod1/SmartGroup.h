//
// "$Id: SmartGroup.h 401 2006-11-11 03:19:07Z mike $"
//
//   Smart group class definitions.
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

#ifndef _SMARTGROUP_H_
#  define _SMARTGROUP_H_


//
// Include necessary headers...
//

#  include <FL/Fl_Group.H>


//
// Widget class...
//

class SmartGroup : public Fl_Group	//// Smart resizing group widget
{
  Fl_Align	child_align_;		// Alignment for children


  public:

 		SmartGroup(int X, int Y, int W, int H,
		           const char *L = (const char *)0);
  virtual	~SmartGroup();
  Fl_Align	child_align() const { return (child_align_); }
  void		child_align(Fl_Align a) { child_align_ = a; }
  void		do_layout(Fl_Align a = FL_ALIGN_TOP, int margin = 10,
		          int hspacing = 10, int vspacing = 10,
			  bool resize_win = false);
  virtual void	resize(int X, int Y, int W, int H);
};

#endif // !_SMARTGROUP_H_

//
// End of "$Id: SmartGroup.h 401 2006-11-11 03:19:07Z mike $".
//
