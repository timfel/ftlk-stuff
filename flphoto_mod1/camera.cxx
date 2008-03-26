//
// "$Id: camera.cxx 442 2007-02-16 03:59:37Z mike $"
//
// Camera imput methods.
//
// Copyright 2002-2007 by Michael Sweet.
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
//   flphoto::camera_cb()           - Import images from a digital camera.
//   flphoto::camera_chooser_cb()   - Select a camera.
//   flphoto::camera_close_cb()     - Close the camera dialog.
//   flphoto::camera_delete_cb()    - Delete files on the camera.
//   flphoto::camera_directory_cb() - Change the download directory.
//   flphoto::camera_download_cb()  - Download files from the cmera.
//   get_files()                    - Recursively get files from the camera.
//   flphoto::camera_select_new()   - Select new images from the camera.
//   progress_start()               - Update the progress widget in the camera
//                                    window.
//   progress_update()              - Update the progress widget in the camera
//                                    window.
//   purge_thumbnails()             - Purge the cached thumbnails...
//

#include "flphoto.h"
#include "i18n.h"
#include "flstring.h"
#include <stdlib.h>
#if defined(WIN32) && !defined(__CYGWIN__)
#  include <direct.h>
#  include <io.h>
#  define fl_mkdir(p)	mkdir(p)
#else
#  include <unistd.h>
#  define fl_mkdir(p)	mkdir(p, 0777)
#endif // WIN32 && !__CYGWIN__
#include <errno.h>
#include "Fl_EXIF_Data.H"
#include "Fl_Print_Dialog.H"
#include <FL/fl_ask.H>
#include <FL/Fl_File_Chooser.H>

#ifdef HAVE_LIBGPHOTO2
#  include <gphoto2.h>
#endif // HAVE_LIBGPHOTO2


//
// Local functions...
//

#ifdef HAVE_LIBGPHOTO2
static void	get_files(Camera *camera, const char *folder,
		          CameraList *list, GPContext *context);
static unsigned	progress_start(GPContext *context, float target,
			       const char *format, va_list args,
        		       void *data);
static void	progress_update(GPContext *context, unsigned id, float current,
		                void *data);
static void	purge_thumbnails(void);
#endif // HAVE_LIBGPHOTO2


//
// 'flphoto::camera_cb()' - Import images from a digital camera.
//

void
flphoto::camera_cb()
{
#ifdef HAVE_LIBGPHOTO2
  int			i, n;		// Looping vars
  CameraList		*list;		// List of cameras
  CameraAbilitiesList	*al;		// List of camera abilities
  GPPortInfoList	*il;		// List of ports
  char			defmodel[255];	// Default camera model
  const char		*model,		// Current model name
			*mptr;		// Pointer into model
  char			quoted[255],	// Quoted model name
			*qptr;		// Pointer into quoted name
  char			camdir[1024],	// Download directory for camera 
			temp[1024];	// Temporary string


  // Get the default camera model...
  prefs.get("camera", defmodel, "", sizeof(defmodel));

  // Get the default download directory...
  snprintf(camdir, sizeof(camdir), "%s/download_directory", defmodel);

  prefs.get(camdir, temp, "", sizeof(temp));
  if (!temp[0])
    prefs.get("download_directory", temp, "", sizeof(temp));

  camera_download_field_->value(temp);

#ifdef DEBUG
  printf("Default camera is \"%s\"...\n", defmodel);
#endif // DEBUG

  if (!context_)
  {
    context_ = gp_context_new();
    gp_context_set_progress_funcs((GPContext *)context_, progress_start,
                                  progress_update, 0, camera_progress_);
  }

  gp_abilities_list_new(&al);
  gp_abilities_list_load(al, (GPContext *)context_);
  gp_port_info_list_new(&il);
  gp_port_info_list_load(il);
  gp_list_new(&list);
  gp_abilities_list_detect(al, il, list, (GPContext *)context_);
  gp_abilities_list_free(al);
  gp_port_info_list_free(il);

  camera_chooser_->clear();
  camera_chooser_->add(_("No Camera Selected"));
  camera_chooser_->add(_("Memory Card"));

  if (!strcasecmp(defmodel, _("Memory Card")))
    camera_chooser_->value(1);
  else
    camera_chooser_->value(0);

  n = gp_list_count(list);

  for (i = 0; i < n; i ++)
  {
    gp_list_get_name(list, i, &model);

    for (qptr = quoted, mptr = model; *mptr;)
    {
      if (*mptr == '/' || *mptr == '\\')
        *qptr++ = '\\';

      *qptr++ = *mptr++;
    }

    *qptr = '\0';

    camera_chooser_->add(quoted);

    if (!strcasecmp(defmodel, model))
      camera_chooser_->value(i + 2);
  }

  gp_list_free(list);

  camera_window_->hotspot(camera_window_);
  camera_window_->show();

  camera_chooser_cb();

  if (camera_chooser_->size() < 3)
    fl_message(_("Sorry, no cameras were found.\n"
                 "Is your camera connected to a USB port and turned on?"));
#else
  fl_message(_("Sorry, no camera support available."));
#endif // HAVE_LIBGPHOTO2
}


//
// 'flphoto::camera_chooser_cb()' - Select a camera.
//

void
flphoto::camera_chooser_cb()
{
  const char	*defmodel;		// Default camera model
  char		camdir[1024],		// Download directory for camera 
		temp[1024];		// Temporary string


  camera_browser_->clear();

#ifdef HAVE_LIBGPHOTO2
  if (camera_)
  {
    gp_camera_unref((Camera *)camera_);
    camera_ = 0;
  }

  purge_thumbnails();
#endif // HAVE_LIBGPHOTO2

  if (camera_chooser_->value() == 0)
    return;

  defmodel = camera_chooser_->text(camera_chooser_->value());
  prefs.set("camera", defmodel);

  // Get the default download directory...
  snprintf(camdir, sizeof(camdir), "%s/download_directory", defmodel);

  prefs.get(camdir, temp, "", sizeof(temp));
  if (!temp[0])
    prefs.get("download_directory", temp, "", sizeof(temp));

  if (temp[0])
    camera_download_field_->value(temp);

#ifdef HAVE_LIBGPHOTO2
  int			i, n;		// Looping vars
  CameraList		*list;		// List of cameras
  CameraAbilitiesList	*al;		// List of camera abilities
  GPPortInfoList	*il;		// List of ports
  CameraAbilities	abilities;	// Camera abilities
  GPPortInfo		info;		// Post info
  CameraFile		*src;		// Camera image file
  const char		*srcname,	// Source filename
			*srcpath;	// Source path
  FILE			*dst;		// Destination file
  char			dstname[1024];	// Destination filename
  int			model,		// Model number
			port;		// Port number
  const char		*portname;	// Port name
  char			flashdir[1024],	// Path to memory card
			command[1024];	// Mount command
  char			thumbdir[1024];	// Thumbnail directory
  const char		*data;		// Thumbnail data
  unsigned long		size;		// Size of data


  // Open the camera...
  gp_camera_new((Camera **)&camera_);

  gp_abilities_list_new(&al);
  gp_abilities_list_load(al, (GPContext *)context_);
  gp_port_info_list_new(&il);
  gp_port_info_list_load(il);
  gp_list_new(&list);
  gp_abilities_list_detect(al, il, list, (GPContext *)context_);

  if (camera_chooser_->value() == 1)
    model = gp_abilities_list_lookup_model(al, "Directory Browse");
  else
    model = gp_abilities_list_lookup_model(al, defmodel);

  gp_abilities_list_get_abilities(al, model, &abilities);
  gp_camera_set_abilities((Camera *)camera_, abilities);

  //**** THIS NEEDS TO BE UPDATED TO SUPPORT NON-USB CAMERAS...
  if (camera_chooser_->value() == 1)
  {
    prefs.get("flash_dir", flashdir, "/mnt/flash", sizeof(flashdir));
    prefs.get("flash_mount", i, 1);

    if (i)
    {
      snprintf(command, sizeof(command), "mount %s", flashdir);
      system(command);
    }

    portname = flashdir;
    srcpath  = flashdir;
  }
  else
  {
    portname = "usb:";
    srcpath  = "/";
  }

//  gp_list_get_value(list, 0, &portname);
  port = gp_port_info_list_lookup_path(il, portname);
  gp_port_info_list_get_info(il, port, &info);
  gp_camera_set_port_info((Camera *)camera_, info);

  gp_abilities_list_free(al);
  gp_port_info_list_free(il);
  gp_list_free(list);

  // Create a temporary directory for thumbnails...
  prefs.getUserdataPath(thumbdir, sizeof(thumbdir));

  // Get a list of files on the camera...
  camera_progress_->value(0);
  camera_progress_->label(_("Getting Image List..."));
  camera_progress_->show();
  Fl::check();
  camera_group_->deactivate();

  gp_list_new(&list);
  get_files((Camera *)camera_, srcpath, list, (GPContext *)context_);

  n = gp_list_count(list);

  camera_window_->cursor(FL_CURSOR_WAIT);
  camera_progress_->maximum(n);

#ifdef DEBUG
  printf("Total files = %d\n", n);
#endif // DEBUG

  for (i = 0; i < n; i ++)
  {
    // Get the source name and path...
    gp_list_get_name(list, i, &srcname);
    gp_list_get_value(list, i, &srcpath);

    // Create the destination file...
    if (camera_chooser_->value() == 1)
    {
      snprintf(dstname, sizeof(dstname), "%s/%s", srcpath, srcname);
    }
    else
    {
      snprintf(dstname, sizeof(dstname), "%s/%s", thumbdir, srcname);

      if (access(dstname, 0))
      {
	// Thumbnail not downloaded...
	if ((dst = fopen(dstname, "wb")) == NULL)
	{
	  fl_alert(_("Unable to create temporary thumbnail image %s:\n\n%s"),
        	   dstname, strerror(errno));
	  break;
	}

	// Update progress...
	camera_progress_->label(srcname);
	camera_progress_->value(i);
	Fl::check();

	// Open the source file on the camera and copy...
	gp_file_new(&src);
	gp_camera_file_get((Camera *)camera_, srcpath, srcname,
                	   GP_FILE_TYPE_PREVIEW, src, (GPContext *)context_);
	gp_file_get_data_and_size(src, &data, &size);
	fwrite(data, 1, size, dst);
	fclose(dst);
	gp_file_unref(src);
      }
    }

    // Add the file to the browser...
    Fl::check();
    camera_browser_->add(dstname);
    camera_browser_->make_visible(camera_browser_->count() - 1);
  }

  camera_select_new();

  camera_browser_->redraw();
  camera_progress_->hide();
  camera_group_->activate();
  camera_window_->cursor(FL_CURSOR_DEFAULT);

  gp_list_free(list);
  gp_camera_exit((Camera *)camera_, (GPContext *)context_);
#endif // HAVE_LIBGPHOTO2
}


//
// 'flphoto::camera_close_cb()' - Close the camera dialog.
//

void
flphoto::camera_close_cb()
{
  if (!camera_group_->active())
  {
    Fl_Widget::default_callback(camera_close_button_, 0);
    return;
  }

#ifdef HAVE_LIBGPHOTO2
  if (camera_)
  {
    gp_camera_unref((Camera *)camera_);
    camera_ = 0;
  }

  purge_thumbnails();

  if (camera_chooser_->value() == 1)
  {
    int		i;			// Mount?
    char	flashdir[1024],		// Flash directory
		command[1024];		// umount command


    prefs.get("flash_dir", flashdir, "/mnt/flash", sizeof(flashdir));
    prefs.get("flash_mount", i, 1);

    if (i)
    {
      snprintf(command, sizeof(command), "umount %s", flashdir);
      system(command);
    }
  }
#endif // HAVE_LIBGPHOTO2

  // Hide the camera window...
  camera_window_->hide();
}


//
// 'flphoto::camera_delete_cb()' - Delete files on the camera.
//

void
flphoto::camera_delete_cb()
{
#ifdef HAVE_LIBGPHOTO2
  int			i, j, n;	// Looping vars
  CameraList		*list;		// List of files
  const char		*srcname,	// Source name
			*srcpath;	// Source path
  char			flashdir[1024];	// Path to memory card
  char			*thumbname;	// Thunbnail name
  Fl_Widget		*button;	// Button pressed


  // Confirm the deletion...
  if (!fl_choice(_("Really delete the selected images?"),
                 _("Cancel"), _("Delete"), NULL))
    return;

  // Get a list of files on the camera...
  camera_close_button_->label(_("Cancel"));
  camera_close_button_->labelcolor(FL_WHITE);
  camera_close_button_->color(FL_RED);
  camera_close_button_->redraw();

  camera_progress_->value(0);
  camera_progress_->label(_("Getting Image List..."));
  camera_progress_->show();
  camera_group_->deactivate();
  Fl::check();

  if (camera_chooser_->value() == 1)
  {
    prefs.get("flash_dir", flashdir, "/mnt/flash", sizeof(flashdir));
    srcpath = flashdir;
  }
  else
    srcpath = "/";

  gp_list_new(&list);
  get_files((Camera *)camera_, srcpath, list, (GPContext *)context_);

  n = gp_list_count(list);

  camera_window_->cursor(FL_CURSOR_WAIT);
  camera_progress_->maximum(n);

  while (Fl::readqueue());

  for (i = camera_browser_->count() - 1; i >= 0; i --)
    if (camera_browser_->selected(i))
    {
      camera_browser_->make_visible(i);

      thumbname = camera_browser_->value(i)->label;

      for (j = 0; j < n; j ++)
      {
	// Get the source name and path...
	gp_list_get_name(list, j, &srcname);
	gp_list_get_value(list, j, &srcpath);

        if (strcmp(srcname, thumbname) != 0)
	  continue;

	// Update progress...
        camera_progress_->label(srcname);
	Fl::check();

	while ((button = Fl::readqueue()) != NULL)
	  if (button == camera_close_button_)
	    break;

	if (button == camera_close_button_)
	{
	  i = 0;
	  break;
	}

	// Remove the file...
	gp_camera_file_delete((Camera *)camera_, srcpath, srcname,
                	      (GPContext *)context_);

	// Remove the file from the browser...
	camera_browser_->remove(i);
        break;
      }
    }

  camera_close_button_->label(_("Close"));
  camera_close_button_->labelcolor(FL_BLACK);
  camera_close_button_->color(FL_GRAY);
  camera_close_button_->redraw();
  camera_progress_->hide();
  camera_window_->cursor(FL_CURSOR_DEFAULT);
  camera_group_->activate();

  gp_list_free(list);
  gp_camera_exit((Camera *)camera_, (GPContext *)context_);

#endif // HAVE_LIBGPHOTO2
}


//
// 'flphoto::camera_directory_cb()' - Change the download directory.
//

void
flphoto::camera_directory_cb(int choose)// I - 1 = choose directory
{
  const char	*d;			// Download directory
  char		camdir[1024];		// Camera directory name


  if (choose)
  {
    // Show the directory chooser...
    if ((d = fl_dir_chooser(_("Download Directory?"),
                            camera_download_field_->value())) != NULL)
      camera_download_field_->value(d);
    else
      return;
  }

  // Set the default and camera-specific download directories...
  snprintf(camdir, sizeof(camdir), "%s/download_directory",
           camera_chooser_->text(camera_chooser_->value()));

  prefs.set("download_directory", camera_download_field_->value());
  prefs.set(camdir, camera_download_field_->value());
}


//
// 'flphoto::camera_download_cb()' - Download files from the camera.
//

void
flphoto::camera_download_cb()
{
#ifdef HAVE_LIBGPHOTO2
  int			i, j, n;	// Looping vars
  int			batch;		// Batch mode
  CameraList		*list;		// List of files
  CameraFile		*src;		// Source file
  const char		*srcname,	// Source filename
			*srcpath;	// Source path
  FILE			*dst;		// Destination file
  const char		*dstdir;	// Destination directory
  int			dstlen;		// Destination directory length
  char			dstname[1024],	// Destination filename
			*dstptr,	// Pointer into destination
			*thumbname;	// Thumbnail name
  char			flashdir[1024];	// Path to memory card
  const char		*data;		// File data
  unsigned long		size;		// Size of file data
  int			warning;	// 0 = no warning shown, 1 = warning shown
  Fl_Widget		*button;	// Button pressed


  // Get the destination directory...
  if ((dstdir = camera_download_field_->value()) == NULL)
    dstdir = "";

  if (!*dstdir)
  {
    if ((dstdir = fl_dir_chooser(_("Download Directory?"), NULL)) == NULL)
      return;

    camera_download_field_->value(dstdir);
    prefs.set("download_directory", dstdir);
  }

  if (access(dstdir, 0))
  {
    if (fl_choice(_("Directory %s does not exist.\n\nCreate directory?"),
                  _("Cancel"), _("Create Directory"), NULL, dstdir))
    {
      if (fl_mkdir(dstdir))
      {
	fl_alert(_("Unable to create directory %s:\n\n%s"), dstdir,
	         strerror(errno));
	return;
      }
    }
    else
      return;
  }

  dstlen = strlen(dstdir) + 1;

  // Get a list of files on the camera...
  camera_close_button_->label(_("Cancel"));
  camera_close_button_->labelcolor(FL_WHITE);
  camera_close_button_->color(FL_RED);
  camera_close_button_->redraw();
  camera_progress_->value(0);
  camera_progress_->label(_("Getting Image List..."));
  camera_progress_->show();
  camera_group_->deactivate();
  Fl::check();

  if (camera_chooser_->value() == 1)
  {
    prefs.get("flash_dir", flashdir, "/mnt/flash", sizeof(flashdir));
    srcpath = flashdir;
  }
  else
    srcpath = "/";

  gp_list_new(&list);
  get_files((Camera *)camera_, srcpath, list, (GPContext *)context_);

  n       = gp_list_count(list);
  warning = 0;
  button  = 0;

  camera_window_->cursor(FL_CURSOR_WAIT);

  while (Fl::readqueue());

  for (i = 0, batch = 0; i < camera_browser_->count(); i ++)
    if (camera_browser_->selected(i))
    {
      camera_browser_->make_visible(i);

      thumbname = camera_browser_->value(i)->label;

      for (j = 0; j < n; j ++)
      {
	// Get the source name and path...
	gp_list_get_name(list, j, &srcname);
	gp_list_get_value(list, j, &srcpath);

        if (strcmp(srcname, thumbname) != 0)
	  continue;

	// Update progress...
        camera_progress_->label(srcname);
	Fl::check();

	while ((button = Fl::readqueue()) != NULL)
	  if (button == camera_close_button_)
	    break;

	if (button == camera_close_button_)
	  break;

	// Create the destination file...
	snprintf(dstname, sizeof(dstname), "%s/%s", dstdir, srcname);
	for (dstptr = dstname + dstlen; *dstptr; dstptr ++)
	  *dstptr = tolower(*dstptr);

	if ((dst = fopen(dstname, "wb")) == NULL)
	{
	  fl_alert(_("Unable to create image %s:\n\n%s"),
        	   dstname, strerror(errno));
	  return;
	}

	// Open the source file on the camera and copy...
	gp_file_new(&src);
	gp_camera_file_get((Camera *)camera_, srcpath, srcname,
                	   GP_FILE_TYPE_NORMAL, src, (GPContext *)context_);
	gp_file_get_data_and_size(src, &data, &size);
	fwrite(data, 1, size, dst);
	fclose(dst);
	gp_file_unref(src);

        // See if this is an image file or something else...
	if (fl_filename_match(srcname,
	                      "*.{arw,avi,bay,bmp,bmq,cr2,crw,cs1,dc2,dcr,"
			      "dng,erf,fff,hdr,jpg,k25,kdc,mdc,mos,nef,orf,"
			      "pcd,pef,png,ppm,pxn,raf,raw,rdc,sr2,srf,sti,"
			      "tif,x3f}"))
	{
	  // Add the file to the main browser...
	  browser_->add(dstname);
	  browser_->make_visible(browser_->count() - 1);
	  update_stats();

	  // Load the EXIF data and see if we need to rotate the image...
	  Fl_EXIF_Data *exif;		// EXIF data from file...

	  exif = new Fl_EXIF_Data(dstname);

#ifdef DEBUG
          printf("width=%d, height=%d, orientation=%d\n",
	         exif->width(), exif->height(), 
		 exif->get_integer(Fl_EXIF_Data::TAG_ORIENTATION));
#endif // DEBUG

          switch (exif->get_integer(Fl_EXIF_Data::TAG_ORIENTATION))
	  {
	    case Fl_EXIF_Data::ORIENT_RIGHT_TOP : // 90 counter-clockwise
	        // Rotate the image if width > height...
	        if (exif->width() <= exif->height())
		  break;

                // Open the image...
                open_image_cb(browser_->count() - 1);

                // Rotate the image 90 degrees counter-clockwise...
                rotate_cb(-90);

                // Save the image...
		save_image(image_item_->filename, batch);

		// Don't show the save dialog for the rest of the images...
		batch = 1;
	        break;

	    case Fl_EXIF_Data::ORIENT_LEFT_BOTTOM : // 90 clockwise
	        // Rotate the image if width > height...
	        if (exif->width() <= exif->height())
		  break;

                // Open the image...
                open_image_cb(browser_->count() - 1);

                // Rotate the image 90 degrees clockwise...
                rotate_cb(90);

                // Save the image...
		save_image(image_item_->filename, batch);

		// Don't show the save dialog for the rest of the images...
		batch = 1;
		break;
	  }

          delete exif;
	}
	else if (!warning)
	{
	  // This image isn't usable by flPhoto; let the user know...
	  fl_message(_("\"%s\" is an unsupported movie or sound file and will not be added to the current album."),
	             srcname);
          warning = 1;
	}
        break;
      }

      if (button == camera_close_button_)
	break;

      // Deselect this image now that it is downloaded...
      camera_browser_->value(i)->selected = 0;
      camera_browser_->redraw();
    }

  browser_->select(browser_->count() - 1);
  open_image_cb();

  camera_close_button_->label(_("Close"));
  camera_close_button_->labelcolor(FL_BLACK);
  camera_close_button_->color(FL_GRAY);
  camera_close_button_->redraw();
  camera_progress_->hide();
  camera_window_->cursor(FL_CURSOR_DEFAULT);
  camera_group_->activate();

  gp_list_free(list);
  gp_camera_exit((Camera *)camera_, (GPContext *)context_);
#endif // HAVE_LIBGPHOTO2
}


#ifdef HAVE_LIBGPHOTO2
//
// 'get_files()' - Recursively get files from the camera.
//

static void
get_files(Camera	*camera,	// I - Camera
          const char	*folder,	// I - Folder to scan
	  CameraList	*list,		// I - List of files
	  GPContext	*context)	// I - Current context
{
  int		i, n;			// Looping vars
  char		path[1024];		// Full path to file
  CameraList	*flist;			// List of files
  const char	*name;			// Name of file


#ifdef DEBUG
  printf("get_files(camera=%p, folder=\"%s\", list=%p, context=%p)\n",
         camera, folder, list, context);
#endif // DEBUG

  gp_list_new(&flist);

  // List the files in this folder...
  gp_camera_folder_list_files(camera, folder, flist, context);

  n = gp_list_count(flist);

#ifdef DEBUG
  printf("# files = %d\n", n);
#endif // DEBUG

  for (i = 0; i < n; i ++)
  {
    gp_list_get_name(flist, i, &name);

#ifdef DEBUG
    int t = gp_list_append(list, name, folder);

    puts(name);
    printf("    append returned %d, count = %d\n", t, gp_list_count(list));
#else
    gp_list_append(list, name, folder);
#endif // DEBUG
  }

  gp_list_reset(flist);

  // List the folders in this folder...
  gp_camera_folder_list_folders(camera, folder, flist, context);

  n = gp_list_count(flist);

#ifdef DEBUG
  printf("# folders = %d\n", n);
#endif // DEBUG

  for (i = 0; i < n; i ++)
  {
    gp_list_get_name(flist, i, &name);

    // Skip .xvpics dir...
    if (!strcmp(name, ".xvpics"))
      continue;

    if (strcmp(folder, "/") == 0)
      snprintf(path, sizeof(path), "/%s", name);
    else
      snprintf(path, sizeof(path), "%s/%s", folder, name);

#ifdef DEBUG
    puts(path);
#endif // DEBUG

    get_files(camera, path, list, context);
  }

  gp_list_free(flist);
}
#endif // HAVE_LIBGPHOTO2


//
// 'flphoto::camera_select_new()' - Select new images from the camera.
//

void
flphoto::camera_select_new()
{
  int			i;		// Looping var
  const char		*dstdir;	// Destination directory
  int			dstlen;		// Length of destination directory
  char			dstname[1024],	// Destination filename
			*dstptr;	// Pointer into destination filename
  Fl_Image_Browser::ITEM *item;		// Current item in browser


  // Get the download directory...
  if ((dstdir = camera_download_field_->value()) == NULL)
    dstdir = ".";
  else if (!*dstdir)
    dstdir = ".";

  dstlen = strlen(dstdir) + 1;

  // Loop through the thumbnails and select any that don't exist in
  // the destination directory...
  for (i = 0; i < camera_browser_->count(); i ++)
  {
    item = camera_browser_->value(i);

    snprintf(dstname, sizeof(dstname), "%s/%s", dstdir, item->label);
    for (dstptr = dstname + dstlen; *dstptr; dstptr ++)
      *dstptr = tolower(*dstptr);

    item->selected = access(dstname, 0) != 0;
  }

  // Redraw the browser...
  camera_browser_->damage(FL_DAMAGE_SCROLL);
}


#ifdef HAVE_LIBGPHOTO2
//
// 'progress_start()' - Update the progress widget in the camera window.
//

static unsigned				// O - ID number
progress_start(GPContext  *context,	// I - Current context
               float      target,	// I - Target size
	       const char *format,	// I - Progress text, if any
	       va_list    args,		// I - Pointer to additional args
               void       *data)	// I - Callback data
{
  Fl_Progress	*p = (Fl_Progress *)data;


  p->value(0);
  p->maximum(target);
  Fl::check();
  return (0);
}


//
// 'progress_update()' - Update the progress widget in the camera window.
//

static void
progress_update(GPContext *context,	// I - Current context
        	unsigned  id,		// I - Current progress ID
		float     current,	// I - Current size
        	void      *data)	// I - Callback data
{
  Fl_Progress	*p = (Fl_Progress *)data;


  p->value(0);
  p->value(current);
  Fl::check();
}


//
// 'purge_thumbnails()' - Purge the cached thumbnails...
//

static void
purge_thumbnails(void)
{
  int		i;			// Looping var
  char		thumbdir[1024],		// Thumbnail directory
		xvpicsdir[1024],	// .xvpics directory
		thumbfile[1024];	// Thumbnail filename
  struct dirent **files;		// Thumbnail files
  int		num_files;		// Number of files


  // Clean out all temporary files...
  flphoto::prefs.getUserdataPath(thumbdir, sizeof(thumbdir));

  num_files = fl_filename_list(thumbdir, &files);

  for (i = 0; i < num_files; i ++)
  {
    if (files[i]->d_name[0] != '.')
    {
      // Remove this thumbnail file...
      snprintf(thumbfile, sizeof(thumbfile), "%s/%s", thumbdir,
               files[i]->d_name);
      unlink(thumbfile);
    }

    free(files[i]);
  }

  if (num_files > 0)
    free(files);

  // Then the .xvpics files...
  snprintf(xvpicsdir, sizeof(xvpicsdir), "%s/.xvpics", thumbdir);

  num_files = fl_filename_list(xvpicsdir, &files);

  for (i = 0; i < num_files; i ++)
  {
    if (files[i]->d_name[0] != '.')
    {
      // Remove this thumbnail file...
      snprintf(thumbfile, sizeof(thumbfile), "%s/%s", xvpicsdir,
               files[i]->d_name);
      unlink(thumbfile);
    }

    free(files[i]);
  }

  if (num_files > 0)
    free(files);

  rmdir(xvpicsdir);
}
#endif // HAVE_LIBGPHOTO2


//
// End of "$Id: camera.cxx 442 2007-02-16 03:59:37Z mike $".
//
