//
// "$Id: Fl_CRW_Image.H 405 2006-11-12 17:09:30Z mike $"
//
// Raw camera image header file for flphoto.
//
// Copyright 2002-2006 by Michael Sweet.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//

#ifndef Fl_CRW_Image_H
#  define Fl_CRW_Image_H
#  include <FL/Fl_Image.H>

class FL_EXPORT Fl_CRW_Image : public Fl_RGB_Image
{
  public:

  Fl_CRW_Image(const char *filename);

  static Fl_Image *check(const char *name, uchar *header, int headerlen);
};

#endif // !Fl_CRW_Image_H

//
// End of "$Id: Fl_CRW_Image.H 405 2006-11-12 17:09:30Z mike $".
//
