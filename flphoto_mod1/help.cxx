//
// "$Id: help.cxx 321 2005-01-23 03:52:44Z easysw $"
//
// Help methods.
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
//   flphoto::help_cb() - Display the help dialog.
//

#include "flphoto.h"
#include <stdio.h>
#include <stdlib.h>
#include "flstring.h"
#include <FL/Fl_Help_Dialog.H>

#ifdef WIN32
#  include <windows.h>
#endif // WIN32


//
// 'flphoto::help_cb()' - Display the help dialog.
//

void
flphoto::help_cb(const char *helpname)	// I - Help file
{
  char			filename[1024];	// Full pathname of help file
  const char		*docdir;	// Documentation directory
  static Fl_Help_Dialog	*hd = 0;	// Help dialog window
#ifdef WIN32				//// Do registry magic...
  HKEY			key;		// Registry key
  DWORD			size;		// Size of string
  char			doc[1024];	// Documentation directory
#endif // WIN32


  // Find the help directory...
#ifdef WIN32
  // Open the registry...
  if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                    "SOFTWARE\\com.easysw\\flPhoto", 0,
                    KEY_READ, &key))
  {
    // Grab the installed directories...
    size = sizeof(doc);

    if (!RegQueryValueEx(key, "InstallDir", NULL, NULL, (unsigned char *)doc,
                         &size))
    {
      strlcat(doc, "doc", sizeof(doc));
      docdir = doc;
    }

    RegCloseKey(key);
  }
  else
#endif // WIN32
  if ((docdir = getenv("FLPHOTO_DOCDIR")) == NULL)
    docdir = FLPHOTO_DOCDIR;

  // Figure out the real filename...
  snprintf(filename, sizeof(filename), "%s/%s", docdir, helpname);

  // Create the help dialog as needed...
  if (!hd)
    hd = new Fl_Help_Dialog();

  // Load and show the help...
  hd->load(filename);
  hd->show();
}


//
// End of "$Id: help.cxx 321 2005-01-23 03:52:44Z easysw $".
//
