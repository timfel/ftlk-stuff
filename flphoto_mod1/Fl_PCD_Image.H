//
// "$Id: Fl_PCD_Image.H 322 2005-01-23 03:55:19Z easysw $"
//
// PhotoCD image header file for flphoto.
//
// Copyright 2002-2005 by Michael Sweet.
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

#ifndef Fl_PCD_Image_H
#define Fl_PCD_Image_H
#  include <stdio.h>
#  include <FL/Fl_Image.H>

class FL_EXPORT Fl_PCD_Image : public Fl_RGB_Image {
  void load_image(FILE *fp);

  public:

  Fl_PCD_Image(const char* filename);
  Fl_PCD_Image(FILE *fp);

  static Fl_Image *check(const char *name, uchar *header, int headerlen);
};

#endif

//
// End of "$Id: Fl_PCD_Image.H 322 2005-01-23 03:55:19Z easysw $".
//
