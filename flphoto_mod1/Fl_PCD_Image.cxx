//
// "$Id: Fl_PCD_Image.cxx 321 2005-01-23 03:52:44Z easysw $"
//
// PhotoCD image routines for flphoto.
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
// Contents:
//
//   Fl_PCD_Image::Fl_PCD_Image() - Load a PhotoCD image...
//   Fl_PCD_Image::Fl_PCD_Image() - Load a PhotoCD image...
//   Fl_PCD_Image::check()        - Try loading the named file.
//   Fl_PCD_Image::load_image()   - Load the image from the file.
//

//
// Include necessary header files...
//

#include <FL/Fl.H>
#include "Fl_PCD_Image.H"
#include <FL/Fl_JPEG_Image.H>
#include "pcd.h"
#include <stdlib.h>
#include "flstring.h"


//
// 'Fl_PCD_Image::Fl_PCD_Image()' - Load a PhotoCD image...
//

Fl_PCD_Image::Fl_PCD_Image(const char *name)	// I - Filename to read from
  : Fl_RGB_Image(0,0,0) {
  FILE		*fp;				// File pointer


  // Open the image file...
  if ((fp = fopen(name, "rb")) == NULL)
    return;

  // Load the image...
  load_image(fp);

  // Close the image file...
  fclose(fp);
}


//
// 'Fl_PCD_Image::Fl_PCD_Image()' - Load a PhotoCD image...
//

Fl_PCD_Image::Fl_PCD_Image(FILE *fp)		// I - File stream to read from
  : Fl_RGB_Image(0,0,0) {
  // Load the image...
  load_image(fp);
}


//
// 'Fl_PCD_Image::check()' - Try loading the named file.
//

Fl_Image *					// O - New image or NULL
Fl_PCD_Image::check(const char *name,		// I - Name of file
		    uchar      *header,		// I - Header of file
		    int        headerlen)	// I - Number of header bytes
{
  FILE		*fp;				// Image file
  char		header2[7];			// More header data
  Fl_Image	*img;				// New image


  // Check for a JPEG image with a thumbnail...
  if (memcmp(header, "\377\330\377", 3) == 0 &&	// Start-of-Image
      header[3] == 0xdb)			// JPEG thumbname file...
    return (new Fl_JPEG_Image(name));

  // Otherwise open the file and see if this is a PhotoCD image...
  if ((fp = fopen(name, "rb")) == NULL)
    return (0);

  fseek(fp, 2048, SEEK_SET);
  memset(header2, 0, sizeof(header2));
  fread(header2, 1, sizeof(header2), fp);

  if (memcmp(header2, "PCD_IPI", 7) == 0)	// PhotoCD image header
    img = new Fl_PCD_Image(fp);
  else
    img = 0;

  // Close the file and return the image we loaded, if any...
  fclose(fp);

  return (img);
}


//
// 'Fl_PCD_Image::load_image()' - Load the image from the file.
//

void
Fl_PCD_Image::load_image(FILE *fp)		// I - File to load
{
  int		left,				// Left position of image
		top,				// Top position of image
		width,				// Width of image
		height,				// Height of image
		rotation;			// Rotation of image
  PCD_IMAGE	pcd;				// Image data


  // Open the PhotoCD file...
  if (pcd_open_fd(&pcd, fileno(fp)))
    return;

  // Get the image size and orientation...
  rotation = pcd_get_rot(&pcd, 0);
  left     = 0;
  top      = 0;
  width    = 0;
  height   = 0;

  if (pcd_select(&pcd, pcd_get_maxres(&pcd), 0, 0, 0, rotation,
                 &left, &top, &width, &height))
  {
    pcd_close(&pcd);
    return;
  }

  // Allocate and initialize...
  w(width);
  h(height);
  d(3);

  array       = new uchar[w() * h() * d()];
  alloc_array = 1;

  // Read the image...
  pcd_decode(&pcd);
  if (pcd_get_image(&pcd, (uchar *)array, PCD_TYPE_RGB, 0))
  {
    pcd_close(&pcd);
    return;
  }

  // Free memory we don't need anymore...
  pcd_close(&pcd);
}


//
// End of "$Id: Fl_PCD_Image.cxx 321 2005-01-23 03:52:44Z easysw $".
//
