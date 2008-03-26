//
// "$Id: directory.cxx 403 2006-11-11 05:56:04Z mike $"
//
// Directory export methods for flPhoto.
//
// Copyright 2005-2006 by Michael Sweet.
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
//   flphoto::export_dir_cb()    - Show the export directory dialog.
//   flphoto::export_dir_ok_cb() - Export the current album to a directory.
//

#include "flphoto.h"
#include "i18n.h"
#include "flstring.h"
#include <FL/filename.H>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>

#if defined(WIN32) && !defined(__CYGWIN__)
#  include <direct.h>
#  include <io.h>
#  define fl_mkdir(p)	mkdir(p)
#else
#  include <unistd.h>
#  define fl_mkdir(p)	mkdir(p, 0777)
#endif // WIN32 && !__CYGWIN__


//
// Function from export.cxx...
//

extern int	export_copy(const char *src, const char *dst);


//
// 'flphoto::export_dir_cb()' - Show the export directory dialog.
//

void
flphoto::export_dir_cb()
{
  directory_window_->hotspot(directory_window_);
  directory_window_->show();
}


//
// 'flphoto::export_dir_ok_cb()' - Export the current album to a directory.
//

void
flphoto::export_dir_ok_cb()
{
  const char		*dir;		// Export directory
  int			renumber;	// Renumber images?
  char			outname[1024];	// Output filename
  int			i;		// Looping var
  Fl_Image_Browser::ITEM *item;		// Current item


  dir      = directory_field_->value();
  renumber = directory_renumber_button_->value();

  if (!*dir)
  {
    fl_alert(_("Please choose a directory!"));
    return;
  }

  // Create the destination directory as needed...
  if (access(dir, 0))
  {
    if (fl_choice(_("Directory %s does not exist."), _("Cancel"),
                  _("Create Directory"), NULL, dir))
    {
      if (fl_mkdir(dir))
      {
	fl_alert(_("Unable to create directory %s:\n\n%s"), dir,
	         strerror(errno));
	return;
      }
    }
    else
      return;
  }

  directory_progress_->show();
  directory_button_group_->deactivate();
  directory_form_group_->deactivate();

  for (i = 0; i < browser_->count(); i ++)
  {
    // Update progress...
    item = browser_->value(i);

    export_progress_->label(item->label);
    export_progress_->value(100 * i / browser_->count());
    Fl::check();

    // Copy the file...
    if (renumber)
      snprintf(outname, sizeof(outname), "%s/img%05d%s", dir, i,
               fl_filename_ext(item->filename));
    else
      snprintf(outname, sizeof(outname), "%s/%s", dir,
               fl_filename_name(item->filename));

    if (export_copy(item->filename, outname))
    {
      fl_alert(_("Unable to copy \"%s\" to \"%s\":\n%s"), item->filename,
               outname, strerror(errno));
      break;
    }
  }

  // Hide the window...
  directory_button_group_->activate();
  directory_form_group_->activate();
  directory_progress_->hide();

  if (i >= browser_->count())
    directory_window_->hide();
}


//
// End of "$Id: directory.cxx 403 2006-11-11 05:56:04Z mike $".
//
