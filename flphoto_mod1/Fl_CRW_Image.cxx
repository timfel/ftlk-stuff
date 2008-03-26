//
// "$Id: Fl_CRW_Image.cxx 405 2006-11-12 17:09:30Z mike $"
//
// Raw camera image routines for flphoto.
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
// Contents:
//
//   Fl_CRW_Image::Fl_CRW_Image() - Load a raw image...
//   Fl_CRW_Image::check()        - Try loading the named file.
//

//
// Include necessary header files...
//

#include <FL/Fl.H>
#include <FL/filename.H>
#include "Fl_CRW_Image.H"
#include "flstring.h"
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>


//
// 'Fl_CRW_Image::Fl_CRW_Image()' - Load a raw image...
//

Fl_CRW_Image::Fl_CRW_Image(
    const char *name)			// I - Filename to read from
  : Fl_RGB_Image(0,0,0) {
  char	ppmname[1024];			// PPM (cached) file


  // See if the raw file has been converted...
  strlcpy(ppmname, name, sizeof(ppmname));
  fl_filename_setext(ppmname, sizeof(ppmname), ".ppm");

  if (access(ppmname, 0))
  {
    // Not cached, run fldcraw to convert it...
    pid_t	pid;			// Process ID of child
    int		status;			// Exit status of child


    if ((pid = fork()) < 0)
    {
      // Unable to fork, return...
      return;
    }
    else if (pid == 0)
    {
      // Child goes here...
      execlp("fldcraw", "fldcraw", name, (const char *)0);
      exit(errno);
    }
    else
    {
      // Parent goes here...
      if (waitpid(pid, &status, 0) < 0)
        return;

      if (status)
        return;

      if (access(ppmname, 0))
        return;
    }
  }

  // Open the cache file...
  FILE *fp = fopen(ppmname, "rb");	// PPM file
  char line[1024];			// Line from file


  // P6
  if (!fgets(line, sizeof(line), fp))
    goto cleanup;

  if (strncmp(line, "P6", 2))
    goto cleanup;

  // width height
  int width, height;

  if (!fgets(line, sizeof(line), fp))
    goto cleanup;

  if (sscanf(line, "%d%d", &width, &height) != 2 || width < 1 || height < 1)
    goto cleanup;

  // maxvalue
  if (!fgets(line, sizeof(line), fp))
    goto cleanup;

  // Allocate memory and read it in...
  array       = new uchar[width * height * 3];
  alloc_array = 1;

  if (fread((void *)array, width * height * 3, 1, fp) < 1)
  {
    delete[] array;
    array       = NULL;
    alloc_array = 0;
  }
  else
  {
    w(width);
    h(height);
    d(3);
  }

  // Close the file and return...
  cleanup:

  fclose(fp);
}


//
// 'Fl_CRW_Image::check()' - Try loading the named file.
//

Fl_Image *					// O - Image or NULL
Fl_CRW_Image::check(const char *name,		// I - Name of file
		    uchar      *header,		// I - Header of file
		    int        headerlen)	// I - Number of header bytes
{
  if (!memcmp(header, "II\032", 3))		// Raw camera file...
    return (new Fl_CRW_Image(name));
  else
    return (0);
}


//
// End of "$Id: Fl_CRW_Image.cxx 405 2006-11-12 17:09:30Z mike $".
//
