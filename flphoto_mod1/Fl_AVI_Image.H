//
// "$Id: Fl_AVI_Image.H 307 2004-09-07 03:01:59Z easysw $"
//
// AVI image header file for the Fast Light Tool Kit (FLTK).
//
// Copyright 2004 by Michael Sweet.
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

#ifndef Fl_AVI_Image_H
#  define Fl_AVI_Image_H
#  include <stdio.h>
#  include <FL/Fl_Image.H>


class FL_EXPORT Fl_AVI_Image : public Fl_RGB_Image
{
  char	*get_chunk(FILE *fp, char name[5], unsigned &length);
  void	load_frame(FILE *fp, unsigned length);
  void	load_image(FILE *fp);
  void	load_list(FILE *fp, long pos);

  public:

  Fl_AVI_Image(const char *filename);
  Fl_AVI_Image(FILE *fp);

  static Fl_Image *check(const char *name, uchar *header, int headerlen);
};

#endif // !Fl_AVI_Image_H

//
// End of "$Id: Fl_AVI_Image.H 307 2004-09-07 03:01:59Z easysw $".
//
