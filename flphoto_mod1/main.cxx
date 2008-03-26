//
// "$Id: main.cxx 400 2006-11-10 05:44:56Z mike $"
//
// FLTK photo program main entry.
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
//   main() - Main entry for flphoto.
//

#include "flphoto.h"
#include "i18n.h"
#include "Fl_AVI_Image.H"
#include "Fl_CRW_Image.H"
#include "Fl_PCD_Image.H"
#include <FL/filename.H>
#include <FL/Fl_Select_Browser.H>
#include "i18n.h"


//
// 'main()' - Main entry for flphoto.
//

int					// O - Exit status
main(int  argc,				// I - Number of command-line arguments
     char *argv[])			// I - Command-line arguments
{
  int		i;			// Looping var
  flphoto	*app = 0,		// Current application window
		*album = 0;		// Current album window
  float		val;			// Gamma value


  // Localize things...
  fl_yes    = _("Yes");
  fl_no     = _("No");
  fl_ok     = _("OK");
  fl_cancel = _("Cancel");
  fl_close  = _("Close");

  Fl_File_Chooser::add_favorites_label    = _("Add to Favorites");
  Fl_File_Chooser::all_files_label        = _("All Files (*)");
  Fl_File_Chooser::custom_filter_label    = _("Custom Filter");
  Fl_File_Chooser::existing_file_label    = _("Please choose an existing file!");
  Fl_File_Chooser::favorites_label        = _("Favorites");
  Fl_File_Chooser::filename_label         = _("Filename:");
  Fl_File_Chooser::filesystems_label      = _("File Systems");
  Fl_File_Chooser::manage_favorites_label = _("Manage Favorites");
  Fl_File_Chooser::new_directory_label    = _("New Directory?");
  Fl_File_Chooser::preview_label          = _("Preview");
  Fl_File_Chooser::show_label             = _("Show:");

  Fl::scheme("gtk+");

  // Register all image formats...
  fl_register_images();
  Fl_Shared_Image::add_handler(Fl_AVI_Image::check);
  Fl_Shared_Image::add_handler(Fl_CRW_Image::check);
  Fl_Shared_Image::add_handler(Fl_PCD_Image::check);

  flphoto::prefs.get("gamma", val, 2.2);
  Fl_Image_Display::set_gamma(val);

	// Open the config_dir with the managed albums
	app = new flphoto();
	int num_albums;
	char help[512] = {'\0'};
	strcpy(help,getenv("HOME"));
	strcat(help,"/.flphoto/");
	dirent** files;
	const char* config_dir = help;
	num_albums = fl_filename_list(config_dir, &files);
	
	for (i = 0; i < num_albums; i++)
	{
		if (fl_filename_match(files[i]->d_name, "*.album"))
		{
			printf("%s \n", files[i]->d_name);
			app->albums_->add(files[i]->d_name);
		}
	}
	char firstfile[512] = {'\0'};
	strcpy(firstfile,config_dir);
	strcat(firstfile,files[2]->d_name);
	printf("%s \n", firstfile);
	app->open_album(firstfile);
	app->show();
	
  // Loop through the command-line looking for albums, files, and
  // directories.
  for (i = 1; i < argc; i ++)
  {
    if (!strcmp(argv[i], "--camera"))
    {
      if (album)
        album->show_camera();
      else if (app)
        app->show_camera();
      else
      {
				app = new flphoto(argv[i]);
				app->show();
        app->show_camera();
      }
    }
    else if (fl_filename_match(argv[i], "*.album"))
    {
      album = new flphoto(argv[i]);
      album->show_last_image();
      album->show();
    }
    else if (!app)
    {
      app = new flphoto(argv[i]);
      app->show();
    }
    else
      app->open_album(argv[i]);
  }

  // Create an empty album if no files are provided on the command-line.
  if (!app && !album)
  {
    app = new flphoto();
    app->show();
  }

  if (app)
    app->show_last_image();

  // Optionally show the license agreement...
  flphoto::prefs.get("license_version", i, 0);

  if (i < FLPHOTO_VERNUMBER)
  {
    flphoto::prefs.set("license_version", FLPHOTO_VERNUMBER);
    flphoto::help_cb("license.html");
  }

  // Run the app until the user quits...
  return (Fl::run());
}


//
// End of "$Id: main.cxx 400 2006-11-10 05:44:56Z mike $".
//
