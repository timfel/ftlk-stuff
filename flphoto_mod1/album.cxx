//
// "$Id: album.cxx 406 2006-11-13 01:20:48Z mike $"
//
// Album methods.
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
//   flphoto::do_file_chooser()  - Show the file chooser and get files/dirs.
//   flphoto::new_album_cb()     - Create a new photo album.
//   flphoto::open_album_cb()    - Open an existing album.
//   flphoto::open_prev_cb)()    - Open a previous album.
//   flphoto::add_file_cb()      - Add an image file.
//   flphoto::add_dir_cb()       - Add an image directory.
//   flphoto::save_album_cb()    - Save the current album.
//   flphoto::save_album_as_cb() - Save the current album to a new file.
//   flphoto::close_album_cb()   - Close the current album.
//   flphoto::quit_cb()          - Quit the application.
//   flphoto::browser_cb()       - Handle clicks in the image browser.
//   flphoto::sort_album_cb()    - Sort the current album by date or name.
//   flphoto::open_album()       - Open an album file.
//   flphoto::open_dir()         - Open an image directory.
//   flphoto::open_file()        - Open an image file.
//   flphoto::save_album()       - Save an album file.
//   flphoto::props_album_cb()   - Show album properties.
//   flphoto::props_ok_cb()      - Apply comments to the current album or image.
//   flphoto::update_stats()     - Update the image statistics line.
//   flphoto::update_history()   - Update the file history.
//   flphoto::update_title()     - Update the title bar.
//

#include "flphoto.h"
#include "Fl_EXIF_Data.H"
#include "i18n.h"
#include <stdio.h>
#include <stdlib.h>
#include "flstring.h"
#if defined(WIN32) && !defined(__CYGWIN__)
#  include <direct.h>
#  include <io.h>
#else
#  include <unistd.h>
#endif // WIN32 && !__CYGWIN__
#include <sys/stat.h>
#include <errno.h>
#include <time.h>


//
// Class globals...
//

int		flphoto::album_count_ = 0;
flphoto		*flphoto::album_first_ = 0;
Fl_File_Chooser	*flphoto::file_chooser_ = 0;
char		flphoto::history_[10][1024];
Fl_Preferences	flphoto::prefs(Fl_Preferences::USER, "easysw.com", "flphoto");


//
// 'flphoto::do_file_chooser()' - Show the file chooser and get files/dirs.
//

int						// O - Number of selected files
flphoto::do_file_chooser(const char *title,	// I - Title string
                         const char *pattern,	// I - Filename patterns
			 int        type,	// I - Type of file to select
			 const char *value)	// I - Initial value
{
  // Create the file chooser if it doesn't exist...
  if (!file_chooser_)
  {
    if (!value)
      value = ".";

    file_chooser_ = new Fl_File_Chooser(value, pattern, type, title);
  }
  else
  {
    file_chooser_->type(type);
    file_chooser_->filter(pattern);
    file_chooser_->label(title);

    if (value && *value)
      file_chooser_->value(value);
    else
      file_chooser_->rescan();
  }

  // Show the chooser and wait for something to happen...
  file_chooser_->show();

  while (file_chooser_->shown())
    Fl::wait();

  return (file_chooser_->count());
}


//
// 'flphoto::new_album_cb()' - Create a new photo album.
//

void
flphoto::new_album_cb()
{
  flphoto	*album;				// New album


  album = new flphoto();
  album->show();
}


//
// 'flphoto::open_album_cb()' - Open an existing album.
//

void
flphoto::open_album_cb()
{
  int		i;				// Looping var
  const char	*f;				// Filename
  flphoto	*album;				// New album


  if (do_file_chooser(_("Album to Open?"), _("Album Files (*.album)"),
                      Fl_File_Chooser::MULTI, NULL))
  {
    // Load all of the selected files...
    for (i = 1; i <= file_chooser_->count(); i ++)
    {
      f = file_chooser_->value(i);

      if (browser_->count() || i > 1)
      {
	album = new flphoto(f);
	album->show();
      }
      else
	open_album(f);
    }
  }
}

void
flphoto::open_album_cb(const char* f)
{
  flphoto	*album;				// New album
  
      if (browser_->count())
      {
				album = new flphoto(f);
				for (int i = 0; i < albums_->size(); i++)
  				album->albums_->add(albums_->text(i));
				album->show();
      }
      else
				open_album(f);
}


//
// 'flphoto::open_prev_cb)()' - Open a previous album.
//

void
flphoto::open_prev_cb(int f)		// I - File number
{
  flphoto	*album;			// Current GUI


  // See if the file is already open...
  for (album = album_first_; album; album = album->album_next_)
    if (!strcmp(history_[f], album->album_filename_))
    {
      // Yes, raise/move the window and return...
      update_history(album->album_filename_);

      album->window_->hotspot(album->window_);
      album->window_->show();
      return;
    }

  // Nope, create a new window and show it...
  if (album_filename_[0])
  {
    album = new flphoto(history_[f]);
    album->show();
  }
  else
    open_album(history_[f]);
}


//
// 'flphoto::add_file_cb()' - Add an image file.
//

void
flphoto::add_file_cb()
{
  int	i;					// Looping var


  if (do_file_chooser(_("Image File(s) to Import?"),
                      _("Image Files (*.{arw,avi,bay,bmp,bmq,cr2,crw,cs1,"
		        "dc2,dcr,dng,erf,fff,hdr,jpg,k25,kdc,mdc,mos,nef,"
			"orf,pcd,pef,png,ppm,pxn,raf,raw,rdc,sr2,srf,sti,"
			"tif,x3f})"),
                      Fl_File_Chooser::MULTI, NULL))
  {
    // Add all of the selected image files...
    for (i = 1; i <= file_chooser_->count(); i ++)
      open_file(file_chooser_->value(i), i == file_chooser_->count());
  }
}


//
// 'flphoto::add_dir_cb()' - Add an image directory.
//

void
flphoto::add_dir_cb()
{
  if (do_file_chooser(_("Image Directory to Import?"), "",
                      Fl_File_Chooser::DIRECTORY, NULL))
  {
    // Open all of the images in the specified directory...
    open_dir(file_chooser_->value());
  }
}


//
// 'flphoto::save_album_cb()' - Save the current album.
//

void
flphoto::save_album_cb()
{
  if (!album_filename_[0])
    save_album_as_cb();
  else
    save_album(album_filename_);
}


//
// 'flphoto::save_album_as_cb()' - Save the current album to a new file.
//

void
flphoto::save_album_as_cb()
{
  char	filename[1024];			// Filename


  if (do_file_chooser(_("Album to Save?"), _("Album Files (*.album)"),
                      Fl_File_Chooser::CREATE, album_filename_))
  {
    if (fl_filename_match(file_chooser_->value(), "*.album"))
      save_album(file_chooser_->value());
    else
    {
      // Add .album to the name...
      snprintf(filename, sizeof(filename), "%s.album", file_chooser_->value());
      save_album(filename);
    }
  }
}


//
// 'flphoto::close_album_cb()' - Close the current album.
//

void
flphoto::close_album_cb()
{
  int			i;			// Looping var
  Fl_Image_Browser::ITEM *item;			// Current image item
  int			yes_to_all;		// Save all images?


  if (Fl::event_key() == FL_Escape)
    return;

  // First save the album as needed...
  if (album_changed_)
  {
    switch (fl_choice(_("Album has changed, do you wish to save it before closing?"),
                      _("Discard Album"), _("Save Album"), _("Cancel")))
    {
      case 0 : // Discard
          break;

      case 1 : // Save
          save_album_cb();

	  if (album_changed_)
	    return;
	  else
	    break;

      case 2 : // Cancel
          return;
    }
  }

  // Then save the individual images, as needed...
  yes_to_all = 0;

  for (i = 0; i < browser_->count(); i ++)
  {
    item = browser_->value(i);

    if (!item->changed)
      continue;

    open_image_cb(i);

    if (yes_to_all)
    {
      save_image(item->filename, 1);

      if (item->changed)
	return;
    }
    else
    {
      switch (fl_choice(_("The image %s has not been saved?\n\nSave it now?"),
                	_("Discard Image"), _("Save Image"),
			_("Save All Images"), item->label))
      {
	case 0 : // Discard
            break;

	case 2 : // Save All
            yes_to_all = 1;

	case 1 : // Save
            save_image(item->filename);

	    if (item->changed)
	      return;
            break;
      }
    }
  }

  // Close the current album...
  if (this == album_first_ && !album_next_ && browser_->count())
  {
    // Just clear this album out...
    window_->redraw();
    display_->value(0);
    browser_->clear();

    album_changed_     = 0;
    album_comment_[0]  = '\0';
    album_filename_[0] = '\0';
    image_item_        = 0;

    update_stats();
    update_title();
  }
  else
  {
    // Delete this album...
    delete this;
  }
}


//
// 'flphoto::quit_cb()' - Quit the application.
//

void
flphoto::quit_cb()
{
  flphoto	*album,				// Current album
		*next;				// Next album


  for (album = album_first_; album; album = album->album_next_)
    if (album->album_changed_)
      break;

  if (album)
  {
    switch (fl_choice(_("Some albums have been changed, do you wish to save them before closing?"),
                      _("Discard Albums"), _("Save Albums"), _("Cancel")))
    {
      case 0 : // Discard
          break;

      case 1 : // Save
	  for (album = album_first_; album; album = next)
	  {
	    next = album->album_next_;

            album->close_album_cb();

	    if (album_first_ == album && album->album_changed_)
	    {
	      // This album wasn't removed from the front of the list,
	      // so the user canceled or the save failed...
	      return;
	    }
	  }
	  break;

      case 2 : // Cancel
          return;
    }
  }

  exit(0);
}


//
// 'flphoto::browser_cb()' - Handle clicks in the image browser.
//

void
flphoto::browser_cb()
{
  int	i;				// Auto open setting

  prefs.get("auto_open", i, 1);

  if ((Fl::event_clicks() || (i && !browser_->changed())) &&
      browser_->selected() >= 0)
    open_image_cb();

  if (browser_->changed())
  {
    // Dragging pictures around...
    album_changed_ = 1;
    update_title();
  }
}


//
// 'flphoto::sort_album_cb()' - Sort the current album by date or name.
//

void
flphoto::sort_album_cb(int type)	// I - 0 for date, 1 for name
{
  Fl_Image_Browser::ITEM *item;		// Current item in browser
  Fl_EXIF_Data	*data;			// EXIF data for file
  const char	*s;			// String for comparison
  struct stat	fileinfo;		// File information
  struct tm	*date;			// UNIX date/time data
  char		datebuf[255];		// File date/time buffer
  int		i, j;			// Looping vars
  int		dir;			// Direction
  int		count;			// Number of images...
  char		**strings;		// Strings to compare...


  // Do nothing if the album is empty...
  if ((count = browser_->count()) <= 0)
    return;

  // Create an array to hold each image's key info...
  strings = new char *[count];

  // Then loop through the album and grab the strings to compare...
//  puts("---- BEFORE ----");

  for (i = 0; i < count; i ++)
  {
    // Grab this item...
    item = browser_->value(i);

    switch (type)
    {
      case 0 :				// Date/time
      case 2 :				// Date/time
	  // Try loading EXIF data for the file...
	  if ((data = new Fl_EXIF_Data(item->filename)) != NULL)
	  {
	    if ((s = data->get_ascii(Fl_EXIF_Data::TAG_DATE_TIME)) == NULL)
	      s = data->get_ascii(Fl_EXIF_Data::TAG_KODAK_DATE_TIME_ORIGINAL);

	    delete data;
	  }
	  else
	    s = NULL;

	  if (!s)
	  {
	    if (stat(item->filename, &fileinfo))
	    {
	      s = "UNKNOWN";
	    }
	    else
	    {
	      date = gmtime(&(fileinfo.st_mtime));

	      snprintf(datebuf, sizeof(datebuf),
	               "%04d:%02d:%02d %02d:%02d:%02d",
		       date->tm_year + 1900, date->tm_mon + 1, date->tm_mday,
		       date->tm_hour, date->tm_min, date->tm_sec);

	      s = datebuf;
	    }
	  }
	  break;

      default :
          s = item->filename;
          break;
    }

    strings[i] = new char[strlen(s) + 1];
    strcpy(strings[i], s);

//    printf("%5d: %s\n", i, strings[i]);
  }

  // Next, do a sort (simple bubblesort...) using the comparison strings...
  dir = (type & 2) ? -1 : 1;

  for (i = 0; i < (count - 1); i ++)
    for (j = i + 1; j < count; j ++)
      if ((dir * strcmp(strings[j], strings[i])) < 0)
      {
        // Swap these two strings...
	browser_->move(i, j);
	browser_->move(j, i);

        s          = strings[i];
	strings[i] = strings[j];
	strings[j] = (char *)s;

	// Mark the album as changed...
	album_changed_ = 1;
      }

  // Free the strings...
//  puts("---- AFTER ----");
  for (i = 0; i < count; i ++)
  {
//    printf("%5d: %s\n", i, strings[i]);

    delete[] strings[i];
  }

  delete[] strings;

  // Update the title bar as needed...
  if (album_changed_)
    update_title();
}


//
// 'flphoto::open_album()' - Open an album file.
//

void
flphoto::open_album(const char *filename)	// I - Album to open
{
  int			i;			// Looping var
  Fl_Image_Browser::ITEM *item;			// Item in browser
  Fl_EXIF_Data		*data;			// EXIF data in image
  const char		*date_time;		// Date/time from EXIF


  show();

  if (fl_filename_isdir(filename))
  {
    // If the user supplies a directory name, then just load the directory...
    open_dir(filename);
  }
  else if (!fl_filename_match(filename, "*.album"))
  {
    // If the user supplies an image filename, then just load the image...
    open_file(filename, 0);
  }
  else
  {
    // Otherwise open the album file and read the list of files.
    FILE			*fp;
    char			line[1024],
				*ptr,
				absname[1024],
				comment[1024];
    Fl_Image_Browser::ITEM	*item;


    if ((fp = fopen(filename, "rb")) == NULL)
    {
      fl_alert(_("Unable to open album file \"%s\":\n\n%s"), filename,
               strerror(errno));
      return;
    }

    strlcpy(line, filename, sizeof(line));
    if ((ptr = strrchr(line + 1, '/')) != NULL)
    {
      *ptr = '\0';
      chdir(line);
    }

    strlcpy(album_filename_, filename, sizeof(album_filename_));

    while (fgets(line, sizeof(line), fp) != NULL)
    {
      // Strip trailing newline/carriage return/whitespace...
      for (ptr = line + strlen(line) - 1; ptr >= line; ptr --)
        if (!isspace(*ptr) && *ptr != '\r')
	  break;
	else
	  *ptr = '\0';

      // Skip header lines...
      if (line[0] == '#' || !line[0])
        continue;

      // Process either "COMMENT: text" or "IMAGE: filename"...
      if ((ptr = strchr(line, ':')) == NULL)
        continue;

      for (ptr ++; isspace(*ptr); ptr ++);

      if (strncmp(line, "COMMENT:", 8) == 0)
      {
        // Add the comment to the string...
	if (album_comment_[0])
	{
	  char	temp[1024];

	  strcpy(temp, album_comment_);
	  snprintf(album_comment_, sizeof(album_comment_), "%s\n%s", temp, ptr);
        }
	else
	  strlcpy(album_comment_, ptr, sizeof(album_comment_));
      }
      else if (strncmp(line, "IMAGE:", 6) == 0)
      {
        // Add the image to the browser...
	fl_filename_absolute(absname, sizeof(absname), ptr);
        browser_->add(absname);
	browser_->make_visible(browser_->count() - 1);
	update_stats();
	Fl::check();
      }
      else if (strncmp(line, "ICOMMENT:", 9) == 0)
      {
        // Add the comment to the string...
        item = browser_->value(browser_->count() - 1);

	if (item->comments)
	{
	  snprintf(comment, sizeof(comment), "%s\n%s", item->comments, ptr);
	  delete[] item->comments;
        }
	else
	  strlcpy(comment, ptr, sizeof(comment));

        item->comments = new char[strlen(comment) + 1];
	strcpy(item->comments, comment);
      }
    }

    update_history(album_filename_);
    show_last_image();

    album_changed_ = 0;
    update_title();
  }

  // Now add the date for all images without comments...
  for (i = 0; i < browser_->count(); i ++)
  {
    // Only set the comment for those that don't have one...
    item = browser_->value(i);

    if (item->comments)
      continue;

    // Try loading EXIF data for the file...
    if ((data = new Fl_EXIF_Data(item->filename)) != NULL)
    {
      // The default comment is the date/time from the image...
      if ((date_time = data->get_ascii(Fl_EXIF_Data::TAG_DATE_TIME)) != NULL)
      {
	item->comments = new char[strlen(date_time) + 1];
	strcpy(item->comments, date_time);
      }

      delete data;
    }
  }
}


//
// 'flphoto::open_dir()' - Open an image directory.
//

void
flphoto::open_dir(const char *dirname)		// I - Directory to import
{
  browser_->load(dirname);
  update_stats();
}


//
// 'flphoto::open_file()' - Open an image file.
//

void
flphoto::open_file(const char *filename,	// I - File to import
                   int        openit)		// I - 1 = open it in the window
{
  browser_->add(filename);
  update_stats();
  Fl::check();

  if (openit)
    open_image_cb(browser_->count() - 1);
}


//
// 'flphoto::save_album()' - Save an album file.
//

void
flphoto::save_album(const char *filename)	// I - Name of album file
{
  FILE			*fp;			// File pointer
  int			i;			// Looping var
  char			temp[1024],		// Comment string
			*ptr,			// Pointer into string
			*next;			// Next line in string
  Fl_Image_Browser::ITEM *item;			// Current image item


  snprintf(temp, sizeof(temp), "%s.bck", filename);
  rename(filename, temp);

  if ((fp = fopen(filename, "wb")) == NULL)
  {
    fl_alert(_("Unable to save album file \"%s\":\n\n%s"), filename,
             strerror(errno));
    rename(temp, filename);
    return;
  }

  strlcpy(temp, filename, sizeof(temp));
  if ((ptr = strrchr(temp + 1, '/')) != NULL)
  {
    *ptr = '\0';
    chdir(temp);
  }

  fputs("#flphoto " FLPHOTO_VERSION "\n", fp);

  strlcpy(temp, album_comment_, sizeof(temp));

  for (ptr = temp; ptr; ptr = next)
  {
    if ((next = strchr(ptr, '\n')) != NULL)
      *next++ = '\0';

    fprintf(fp, "COMMENT: %s\n", ptr);
  }

  for (i = 0; i < browser_->count(); i ++)
  {
    item = browser_->value(i);
    fl_filename_relative(temp, sizeof(temp), item->filename);

    fprintf(fp, "IMAGE: %s\n", temp);

    if (item->comments)
    {
      strlcpy(temp, item->comments, sizeof(temp));

      for (ptr = temp; ptr; ptr = next)
      {
	if ((next = strchr(ptr, '\n')) != NULL)
	  *next++ = '\0';

	fprintf(fp, "ICOMMENT: %s\n", ptr);
      }
    }
  }

  fclose(fp);

  if (filename != album_filename_)
    strlcpy(album_filename_, filename, sizeof(album_filename_));

  album_changed_ = 0;

  update_history(album_filename_);

  update_title();
}


//
// 'flphoto::props_album_cb()' - Show album properties.
//

void
flphoto::props_album_cb()
{
  props_comments_field_->resize(10, 25, 285, 260);
  props_comments_field_->value(album_comment_);
  props_exif_field_->hide();
  props_window_->hotspot(props_window_);
  props_window_->show();
}


//
// 'flphoto::props_ok_cb()' - Apply comments to the current album or image.
//

void
flphoto::props_ok_cb()
{
  if (props_exif_field_->visible())
  {
    // Save image comments...
    if (image_item_->comments)
      delete[] image_item_->comments;

    image_item_->comments = new char[props_comments_field_->size() + 1];
    strcpy(image_item_->comments, props_comments_field_->value());
  }
  else
  {
    // Save album comments...
    strcpy(album_comment_, props_comments_field_->value());
  }

  album_changed_ = 1;
  update_title();

  props_window_->hide();
}


//
// 'flphoto::update_stats()' - Update the image statistics line.
//

void
flphoto::update_stats()
{
  int			i;		// Looping var
  Fl_Image_Browser::ITEM *item;		// Current item
  double		total;		// Total megabytes of images
  struct stat		fileinfo;	// File information


  for (i = 0, total = 0.0; i < browser_->count(); i ++)
  {
    item = browser_->value(i);

    if (!stat(item->filename, &fileinfo))
      total += fileinfo.st_size / (1024.0 * 1024.0);
  }

  snprintf(stats_label_, sizeof(stats_label_), _("%d image(s), %.1fMB"),
           browser_->count(), total);
  stats_box_->label(stats_label_);
}



//
// 'flphoto::update_history()' - Update the file history.
//

void
flphoto::update_history(const char *f)	// I - File to add or NULL
{
  int		i;			// Looping var
  const char	*basename;		// Base filename
  Fl_Menu_Item	*menu;			// Pointer to menu items


  if (f)
  {
    // Add/move this file in the history list...
    for (i = 0; i < 10; i ++)
      if (!strcmp(history_[i], f))
        break;

    if (i > 0)
    {
      // Move this file to the top...
      if (i > 9)
        i = 9;

      memmove(history_ + 1, history_, i * sizeof(history_[0]));
      strlcpy(history_[0], f, sizeof(history_[0]));
    }

    // Save history...
    for (i = 0; i < 10; i ++)
      if (history_[i][0])
        prefs.set(Fl_Preferences::Name("file%d", i), history_[i]);
      else
        break;
  }
  else
  {
    // Load history...
    for (i = 0; i < 10; i ++)
      prefs.get(Fl_Preferences::Name("file%d", i), history_[i], "",
                sizeof(history_[0]));
  }

  // Update the menu item labels...
  menu = (Fl_Menu_Item *)menubar_->menu();

  if (!history_[0][0])
  {
    // No files in history...
    menu[3].deactivate();
  }
  else
  {
    // Hide/show only the files that are present in the history...
    menu[3].activate();

    for (i = 0; i < 10; i ++)
      if (history_[i][0])
      {
	if ((basename = strrchr(history_[i], '/')) != NULL)
          basename ++;
	else
          basename = history_[i];

	menu[i + 4].show();
	menu[i + 4].label(basename);
      }
      else
	menu[i + 4].hide();
  }
}


//
// 'flphoto::update_title()' - Update the title bar.
//

void
flphoto::update_title()
{
  const char	*name;				// Name portion of filename


  if ((name = strrchr(album_filename_, '/')) != NULL)
    name ++;
  else if (album_filename_[0])
    name = album_filename_;
  else
    name = "New Album";

  snprintf(title_, sizeof(title_), "%s[%d] %s- flphoto " FLPHOTO_VERSION, name,
           album_count_, album_changed_ ? "(modified) " : "");
  window_->label(title_);
}


//
// End of "$Id: album.cxx 406 2006-11-13 01:20:48Z mike $".
//
