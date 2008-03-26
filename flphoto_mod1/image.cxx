//
// "$Id: image.cxx 416 2006-11-18 13:56:11Z mike $"
//
// Image methods.
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
//   flphoto::open_image_cb()             - Open the currently selected image.
//   flphoto::edit_image_cb()             - Edit the current image.
//   flphoto::save_image_cb()             - Save an image.
//   flphoto::save_image_as_cb()          - Save an image with a new filename.
//   flphoto::save_image()                - Save the current image to a file.
//   flphoto::save_jpeg()                 - Save the current image to a JPEG file.
//   flphoto::save_png()                  - Save the current image to a PNG file.
//   flphoto::select_image_cb()           - Select all or no images.
//   flphoto::remove_image_cb()           - Remove the current image.
//   flphoto::purge_image_cb()            - Purge the current image.
//   flphoto::image_progress()            - Show a progress bar for an image operation.
//   flphoto::revert_image_cb()           - Revert the image back to the original.
//   flphoto::auto_adjust_cb()            - Automatically adjust the image.
//   flphoto::adjust_levels_cb()          - Adjust the levels for brightness and contrast.
//   flphoto::redeye_cb()                 - Remove red-eye from the image.
//   flphoto::blur_cb()                   - Blur the image.
//   flphoto::sharpen_cb()                - Sharpen the image.
//   flphoto::crop_cb()                   - Crop an image.
//   flphoto::crop_ok_cb()                - Actually do the cropping of the image.
//   flphoto::crop_update_cb()            - Update the preview image.
//   flphoto::rotate_image()              - Rotate an image.
//   flphoto::rotate_cb()                 - Rotate an image.
//   flphoto::scale_cb()                  - Scale an image.
//   flphoto::scale_ok_cb()               - Actually do the scaling of the image.
//   flphoto::scale_update_cb()           - Update the scaling area.
//   flphoto::blur_image()                - Blur an image.
//   flphoto::copy_image()                - Copy image data.
//   flphoto::props_image_cb()            - Show image properties.
//   flphoto::save_changed_image_cb()     - Save changed images.
//   flphoto::edit_select_image_cb()      - Edit selected images.
//   flphoto::save_selected_image_cb()    - Save selected images.
//   flphoto::revert_selected_image_cb()  - Revert selected images.
//   flphoto::purge_selected_image_cb()   - Purge selected images.
//   flphoto::remove_selected_image_cb()  - Remove selected images.
//   flphoto::auto_adjust_selected_cb()   - Auto-adjust selected images.
//   flphoto::adjust_levels_selected_cb() - Adjust levels in selected images.
//   flphoto::blur_selected_cb()          - Blur selected images.
//   flphoto::sharpen_selected_cb()       - Sharpen selected images.
//   flphoto::crop_selected_cb()          - Crop selected images.
//   flphoto::rotate_selected_cb()        - Rotate selected images.
//   flphoto::scale_selected_cb()         - Scale selected images.
//

#include "flphoto.h"
#include "i18n.h"
#include <stdio.h>
#include <stdlib.h>
#include "flstring.h"
#include <ctype.h>
#include <errno.h>
#include <math.h>
#if defined(WIN32) && !defined(__CYGWIN__)
#  include <io.h>
#else
#  include <unistd.h>
#endif // WIN32 && !__CYGWIN__
#include <FL/fl_draw.H>
#include <FL/filename.H>
#include "Fl_EXIF_Data.H"

#ifdef HAVE_LIBJPEG
// Some releases of the Cygwin JPEG libraries don't have a correctly
// updated header file for the INT32 data type; the following define
// from Shane Hill seems to be a usable workaround...
#  if defined(WIN32) && defined(__CYGWIN__)
#    define XMD_H
#  endif // WIN32 && __CYGWIN__
extern "C" {
#  include <jpeglib.h>
#  include "transupp.h"
};
#endif // HAVE_LIBJPEG

#ifdef HAVE_LIBPNG
#  include <png.h>
#endif // HAVE_LIBPNG


//
// Change bits for images...
//

#define CHANGE_ROTATE_0		0
#define CHANGE_ROTATE_90	1
#define CHANGE_ROTATE_180	2
#define CHANGE_ROTATE_270	3
#define CHANGE_ROTATE_MASK	3
#define CHANGE_CROP		4
#define CHANGE_SCALE		8

#define CHANGE_TOUCHUP		16


//
// 'flphoto::open_image_cb()' - Open the currently selected image.
//

void
flphoto::open_image_cb(int number)		// I - Image number
{
  int				i;		// Looping var
  Fl_Image_Browser::ITEM	*item;		// Current item
  float				zoom;		// Zoom value


  // Load the current image as needed...
  zoom = display_->scale();
  if (number < 0)
  {
    display_->value(browser_->load_item(browser_->selected()));
    image_item_ = browser_->value(browser_->selected());
  }
  else
  {
    browser_->make_visible(number);
    display_->value(browser_->load_item(number));
    image_item_ = browser_->value(number);
  }

  prefs.get("keep_zoom", i, 1);
  if (i)
    display_->scale(zoom);

  // Free up memory used by other images as needed...
  for (i = 0; i < browser_->count(); i ++)
  {
    item = browser_->value(i);

    if (item != image_item_ && item->image && !item->changed)
    {
      item->image->release();
      item->image = 0;
    }
  }

  Fl::check();
}


//
// 'flphoto::edit_image_cb()' - Edit the current image.
//

void
flphoto::edit_image_cb()
{
#ifdef WIN32
  fl_message(_("Sorry, this is not yet implemented!"));
#else
  int	pid;				// Process ID...
  char	command[1024];			// Image editor


  if (image_item_)
  {
    prefs.get("image_editor", command, "gimp", sizeof(command));

    if ((pid = fork()) == 0)
    {
      execlp(command, command, image_item_->filename, NULL);
      exit(errno);
    }
  }
  else
    fl_message(_("Please open an image first."));
#endif // WIN32
}


//
// 'flphoto::save_image_cb()' - Save an image.
//

void
flphoto::save_image_cb()
{
  if (!image_item_ || !image_item_->changed)
    return;

  save_image(image_item_->filename);
}


//
// 'flphoto::save_image_as_cb()' - Save an image with a new filename.
//

void
flphoto::save_image_as_cb()
{
  const char	*newname;			// New image name


  if (!image_item_)
    return;

  if ((newname = fl_file_chooser(_("Save Image As?"),
                                 _("Image Files (*.{jpg,png})"),
                                 image_item_->filename)) != NULL)
  {
    if (!access(newname, 0))
    {
      if (!fl_choice(_("Image file %s already exists!\n\nOK to replace?"),
                     _("Cancel"), _("Replace"), NULL,
		     fl_filename_name(newname)))
	return;
    }

    save_image(newname);
  }
}


//
// 'flphoto::save_image()' - Save the current image to a file.
//

void
flphoto::save_image(const char *filename,	// I - Filename
                    int        batch)		// I - 1 = batch mode
{
  int		i;				// Looping var
  const char	*ext;				// Extension of image
  int		status;				// Status of save


  window_->show();

  ext = fl_filename_ext(filename);
  if (!strcasecmp(ext, ".jpg"))
    status = save_jpeg(filename, batch);
  else if (!strcasecmp(ext, ".png"))
    status = save_png(filename, batch);
  else
  {
    fl_alert(_("Unknown extension \"%s\"; unable to save!"), ext);
    status = 0;
  }

  if (status)
  {
    // Successfully saved image...
    browser_->damage(FL_DAMAGE_SCROLL);

    if (strcmp(image_item_->filename, filename) != 0)
    {
      image_item_->image->release();
      image_item_->image = 0;
      image_item_->thumbnail->release();
      image_item_->thumbnail = Fl_Shared_Image::get(image_item_->thumbname);
      image_item_->changed = 0;

      if ((i = browser_->find(filename)) >= 0)
      {
        browser_->select(i);
        image_item_ = browser_->value(i);

	image_item_->changed = 0;

	if (image_item_->image)
	{
	  image_item_->image->release();
	  image_item_->image = 0;
	}

	image_item_->save_thumbnail(1);
      }
      else
      {
	browser_->add(filename);
        browser_->select(browser_->count() - 1);
        image_item_ = browser_->value(browser_->count() - 1);
	update_stats();
      }

      open_image_cb();
    }
    else
    {
      image_item_->changed = 0;
      image_item_->save_thumbnail();
    }
  }
}


//
// 'flphoto::save_jpeg()' - Save the current image to a JPEG file.
//

int						// O - 0 on failure, 1 on success
flphoto::save_jpeg(const char *filename,	// I - Filename
                   int        batch)		// I - 1 = batch mode
{
  Fl_Shared_Image		*img;		// Image to write...
  uchar				*ptr;		// Pointer to image data
  int				val;		// JPEG value
  int				doxform;	// Just transform image?
  FILE				*cfp;		// File pointer
  struct jpeg_compress_struct	cinfo;		// Compressor info
  struct jpeg_error_mgr		cerr;		// Error handler info
  jvirt_barray_ptr		*carrays;	// Coefficients
  char				dname[1024];	// Original filename
  FILE				*dfp;		// File pointer
  struct jpeg_decompress_struct	dinfo;		// Decompressor info
  struct jpeg_error_mgr		derr;		// Error handler info
  jvirt_barray_ptr		*darrays;	// Coefficients
  jpeg_transform_info		xform;		// Transformation options


  // First see if we are changing anything but the orientation of the image...
  if (!strcmp(filename, image_item_->filename) &&
      (image_item_->changed & ~CHANGE_ROTATE_MASK))
  {
    if (!fl_choice(_("Warning: Re-saving %s will cause a loss of quality!\n"
                     "Do you wish to replace the original JPEG file?"),
                   _("Cancel"), _("Replace"), NULL, filename))
      return (0);
  }

  // Then backup the image if we are saving over the original one...
  if (!strcmp(filename, image_item_->filename))
  {
    snprintf(dname, sizeof(dname), "%s.bck", filename);

    if (rename(filename, dname))
    {
      fl_alert(_("Unable to rename %s:\n\n%s"), fl_filename_name(filename),
               strerror(errno));
      return (0);
    }
  }
  else
    strlcpy(dname, image_item_->filename, sizeof(dname));

  // Do we need to show the JPEG parameter dialog?
  if ((image_item_->changed & ~CHANGE_ROTATE_MASK) ||
      strcasecmp(fl_filename_ext(image_item_->filename), ".jpg"))
  {
    // Not just transforming...
    doxform = 0;

    jpeg_quality_group_->show();
    jpeg_transform_box_->hide();
  }
  else
  {
    // Just transforming...
    doxform = 1;

    jpeg_quality_group_->hide();
    jpeg_transform_box_->show();
  }

  // Load JPEG parameters...
  prefs.get("jpeg_quality", val, 75);
  jpeg_quality_value_->value(val);

  prefs.get("jpeg_progressive", val, 1);
  jpeg_progressive_button_->value(val);

  prefs.get("jpeg_optimize", val, 1);
  jpeg_optimize_button_->value(val);

  prefs.get("jpeg_comments", val, 1);
  jpeg_comments_button_->value(val);

  prefs.get("jpeg_exif", val, 1);
  jpeg_exif_button_->value(val);

  if (!batch)
  {
    // Show the JPEG settings...
    jpeg_window_->hotspot(jpeg_window_);
    jpeg_window_->show();

    while (jpeg_window_->shown())
      Fl::wait();
  }

  // Save JPEG parameters...
  prefs.set("jpeg_quality", (int)jpeg_quality_value_->value());
  prefs.set("jpeg_progressive", jpeg_progressive_button_->value());
  prefs.set("jpeg_optimize", jpeg_optimize_button_->value());
  prefs.set("jpeg_comments", jpeg_comments_button_->value());
  prefs.set("jpeg_exif", jpeg_exif_button_->value());

  // Open the original image as needed..
  if ((jpeg_comments_button_->value() || jpeg_exif_button_->value() || doxform) &&
      !strcasecmp(fl_filename_ext(image_item_->filename), ".jpg"))
  {
    // Try opening the original JPEG file...
    if ((dfp = fopen(dname, "rb")) == NULL)
    {
      fl_alert(_("Unable to open original JPEG image:\n\n%s"), strerror(errno));

      if (!strcmp(image_item_->filename, filename))
	rename(dname, filename);

      return (0);
    }

    // Then read the JPEG header and data...
    dinfo.err = jpeg_std_error(&derr);
    jpeg_create_decompress(&dinfo);
    jpeg_stdio_src(&dinfo, dfp);

    if (jpeg_exif_button_->value())
    {
      for (int m = 0; m < 16; m ++)
        jpeg_save_markers(&dinfo, JPEG_APP0 + m, 0xffff);
    }

    if (jpeg_comments_button_->value())
      jpeg_save_markers(&dinfo, JPEG_COM, 0xffff);

    jpeg_read_header(&dinfo, 1);
  }
  else
    dfp = NULL;

  // Create the output file...
  if ((cfp = fopen(filename, "wb")) == NULL)
  {
    fl_alert(_("Unable to create JPEG image:\n\n%s"), strerror(errno));

    if (dfp)
      fclose(dfp);

    if (!strcmp(image_item_->filename, filename))
      rename(dname, filename);

    return (0);
  }

  cinfo.err = jpeg_std_error(&cerr);
  jpeg_create_compress(&cinfo);
  jpeg_stdio_dest(&cinfo, cfp);

  img = image_item_->image;

  if (doxform)
  {
    // Set transform parameters...
    xform.trim            = 0;
    xform.force_grayscale = 0;

    switch (image_item_->changed & CHANGE_ROTATE_MASK)
    {
      case CHANGE_ROTATE_90 :
          xform.transform = JXFORM_ROT_270;
          break;

      case CHANGE_ROTATE_180 :
          xform.transform = JXFORM_ROT_180;
          break;

      case CHANGE_ROTATE_270 :
          xform.transform = JXFORM_ROT_90;
          break;
    }

    // Any space needed by a transform option must be requested before
    // jpeg_read_coefficients so that memory allocation will be done right.
    jtransform_request_workspace(&dinfo, &xform);

    // Read source file as DCT coefficients
    darrays = jpeg_read_coefficients(&dinfo);

    // Initialize destination compression parameters from source values
    jpeg_copy_critical_parameters(&dinfo, &cinfo);

    // Adjust destination parameters if required by transform options;
    // also find out which set of coefficient arrays will hold the output.
    carrays = jtransform_adjust_parameters(&dinfo, &cinfo, darrays, &xform);
  }
  else
  {
    cinfo.image_width      = img->w();
    cinfo.image_height     = img->h();
    cinfo.input_components = img->d();
    cinfo.in_color_space   = img->d() == 1 ? JCS_GRAYSCALE : JCS_RGB;

    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, (int)jpeg_quality_value_->value(), 1);

    carrays = NULL;
    darrays = NULL;
  }

  if (jpeg_progressive_button_->value())
    jpeg_simple_progression(&cinfo);

  if (jpeg_optimize_button_->value())
    cinfo.optimize_coding = 1;

  // Transform or save...
  if (doxform)
  {
    // Do the transform...
    image_progress(_("Transforming image on disk..."), 0);
    jpeg_write_coefficients(&cinfo, carrays);

    image_progress(_("Transforming image on disk..."), 25);
    jcopy_markers_execute(&dinfo, &cinfo, JCOPYOPT_ALL);

    image_progress(_("Transforming image on disk..."), 50);
    jtransform_execute_transformation(&dinfo, &cinfo, darrays, &xform);
  }
  else
  {
    // Save the image...
    jpeg_start_compress(&cinfo, 1);

    if (dfp)
      jcopy_markers_execute(&dinfo, &cinfo, JCOPYOPT_ALL);

    while (cinfo.next_scanline < cinfo.image_height)
    {
      image_progress(_("Saving image..."), 100 * cinfo.next_scanline / img->h());

      ptr = (uchar *)img->data()[0] +
            cinfo.next_scanline * img->w() * img->d();

      jpeg_write_scanlines(&cinfo, &ptr, 1);
    }
  }

  jpeg_finish_compress(&cinfo);
  jpeg_destroy_compress(&cinfo);

  fclose(cfp);

  if (dfp)
  {
    // Free decompressor memory and close the original file...
    jpeg_destroy_decompress(&dinfo);

    fclose(dfp);
  }

  display_->redraw();

  return (1);
}


//
// 'flphoto::save_png()' - Save the current image to a PNG file.
//

int						// O - 0 on failure, 1 on success
flphoto::save_png(const char *filename,		// I - Filename
                  int        batch)		// I - 1 = batch mode
{
  Fl_Shared_Image		*img;		// Image to write...
  int				y;		// Current row
  const uchar			*ptr;		// Pointer to image data
  FILE				*fp;		// File pointer
  char				bckname[1024];	// Backup filename
  png_structp			pp;		// PNG data
  png_infop			info;		// PNG image info


  // Save the original file...
  if (!strcmp(image_item_->filename, filename))
  {
    snprintf(bckname, sizeof(bckname), "%s.bck", filename);
    rename(filename, bckname);
  }

  // Create the output file...
  if ((fp = fopen(filename, "wb")) == NULL)
  {
    fl_alert(_("Unable to create PNG image:\n\n%s"), strerror(errno));

    if (!strcmp(image_item_->filename, filename))
      rename(bckname, filename);

    return (0);
  }

  // Create the PNG image structures...
  img = image_item_->image;

  pp = png_create_write_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
  if (!pp)
  {
    fclose(fp);

    fl_alert(_("Unable to create PNG data:\n\n%s"), strerror(errno));

    if (!strcmp(image_item_->filename, filename))
      rename(bckname, filename);

    return (0);
  }

  info = png_create_info_struct(pp);
  if (!info)
  {
    fclose(fp);

    png_destroy_write_struct(&pp, 0);

    fl_alert(_("Unable to create PNG image information:\n\n%s"),
             strerror(errno));

    if (!strcmp(image_item_->filename, filename))
      rename(bckname, filename);

    return (0);
  }

  if (setjmp(png_jmpbuf(pp)))
  {
    fclose(fp);

    png_destroy_write_struct(&pp, &info);

    fl_alert(_("Unable to write PNG image:\n\n%s"), strerror(errno));

    if (!strcmp(image_item_->filename, filename))
      rename(bckname, filename);

    return (0);
  }

  png_init_io(pp, fp);

  png_set_compression_level(pp, Z_BEST_COMPRESSION);
  png_set_IHDR(pp, info, img->w(), img->h(), 8,
               img->d() == 1 ? PNG_COLOR_TYPE_GRAY : PNG_COLOR_TYPE_RGB,
	       PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
	       PNG_FILTER_TYPE_DEFAULT);
  png_set_sRGB(pp, info, PNG_sRGB_INTENT_PERCEPTUAL);
  png_set_sRGB_gAMA_and_cHRM(pp, info, PNG_INFO_sRGB);

  png_write_info(pp, info);

  for (y = 0, ptr = (const uchar *)img->data()[0];
       y < img->h();
       y ++, ptr += img->w() * img->d())
  {
    if (!(y & 15))
      image_progress(_("Saving image..."), 100 * y / img->h());

    png_write_row(pp, (png_byte *)ptr);
  }

  png_write_end(pp, info);
  png_destroy_write_struct(&pp, 0);

  fclose(fp);

  display_->redraw();

  return (1);
}


//
// 'flphoto::select_image_cb()' - Select all or no images.
//

void
flphoto::select_image_cb(int v)			// I - Select value
{
  int			i;			// Looping var
  Fl_Image_Browser::ITEM *item;			// Current item


  for (i = 0; i < browser_->count(); i ++)
  {
    item = browser_->value(i);
    item->selected = v;
  }

  browser_->redraw();
}


//
// 'flphoto::remove_image_cb()' - Remove the current image.
//

void
flphoto::remove_image_cb()
{
  int	i;					// Looping var


  if (!image_item_)
    return;

  if (image_item_->changed)
  {
    if (!fl_choice(_("%s has been modified, really delete?"),
                   _("Cancel"), _("Delete"), NULL, image_item_->label))
      return;
  }

  for (i = 0; i < browser_->count(); i ++)
    if (browser_->value(i) == image_item_)
    {
      display_->value(0);
      image_item_ = 0;

      browser_->remove(i);
      update_stats();
      i --;

      album_changed_ = 1;
      break;
    }

  browser_->redraw();
  update_title();
}


//
// 'flphoto::purge_image_cb()' - Purge the current image.
//

void
flphoto::purge_image_cb()
{
  int	i;					// Looping var


  if (!image_item_)
    return;

  if (!fl_choice(_("Purging %s will delete the file from disk.\n\nReally delete?"),
                 _("Cancel"), _("Delete"), NULL, image_item_->label))
    return;

  unlink(image_item_->filename);
  unlink(image_item_->thumbname);

  for (i = 0; i < browser_->count(); i ++)
    if (browser_->value(i) == image_item_)
    {
      display_->value(0);
      image_item_ = 0;

      browser_->remove(i);
      update_stats();
      i --;

      album_changed_ = 1;
      break;
    }

  browser_->redraw();
  update_title();
}


//
// 'flphoto::image_progress()' - Show a progress bar for an image operation.
//

void
flphoto::image_progress(const char *label,	// I - Label string
                        int        percent)	// I - Percent complete
{
  // Handle any pending redraws...
  Fl::check();

  // Make this window current...
  window_->make_current();

  // Draw a progress bar in the middle of the image...
  fl_font(FL_HELVETICA, FL_NORMAL_SIZE);

  int ww = display_->w() / 2;
  int hh = FL_NORMAL_SIZE + 20;
  int xx = display_->x() + display_->w() / 4;
  int yy = display_->y() + (display_->h() - hh) / 2;

  if (percent > 0)
  {
    fl_push_clip(xx, yy, ww * percent / 100, hh);
    fl_draw_box(FL_UP_BOX, xx, yy, ww, hh, FL_SELECTION_COLOR);
    fl_color(fl_contrast(FL_BLACK, FL_SELECTION_COLOR));
    fl_draw(label, xx, yy, ww, hh, FL_ALIGN_CENTER);
    fl_pop_clip();
  }

  if (percent < 100)
  {
    fl_push_clip(xx + ww * percent / 100, yy,
	         ww - ww * percent / 100, hh);
    fl_draw_box(FL_UP_BOX, xx, yy, ww, hh, display_->color());
    fl_color(fl_contrast(FL_BLACK, display_->color()));
    fl_draw(label, xx, yy, ww, hh, FL_ALIGN_CENTER);
    fl_pop_clip();
  }

  // Flush all drawing commands...
  Fl::flush();
}


//
// 'flphoto::revert_image_cb()' - Revert the image back to the original.
//

void
flphoto::revert_image_cb()
{
  if (image_item_->image)
  {
    float factor;

    image_item_->image->release();
    image_item_->image = Fl_Shared_Image::get(image_item_->filename);
    image_item_->thumbnail->release();
    image_item_->thumbnail = Fl_Shared_Image::get(image_item_->thumbname);
    image_item_->changed = 0;
    factor = display_->scale();
    display_->value(image_item_->image);
    display_->scale(factor);
    browser_->damage(FL_DAMAGE_SCROLL);
  }
}


//
// 'flphoto::auto_adjust_cb()' - Automatically adjust the image.
//

void
flphoto::auto_adjust_cb()
{
  sharpen_cb();
  adjust_levels_cb();
}


//
// 'flphoto::adjust_levels_cb()' - Adjust the levels for brightness and contrast.
//

void
flphoto::adjust_levels_cb()
{
  int			i, y,			// Looping vars
			temp,			// Temporary value
			hist[16];		// Histogram
  int			mingray,		// Minimum gray value
			maxgray,		// Maximum gray value
			gray;			// Current gray value
  uchar			*rgb,			// Pointer to image data
			lut[256];		// Lookup table
  Fl_Shared_Image	*img;			// Current image


  // Get the image...
  img = display_->value();
  if (!img)
    return;

  // Figure out the min and max gray values...
  mingray = 255;
  maxgray = 0;

  memset(hist, 0, sizeof(hist));

  for (y = img->h(), rgb = (uchar *)img->data()[0]; y > 0; y --)
  {
    if (!(y & 15))
      image_progress(_("Histogramming Image..."),
                     100 * (img->h() - y) / img->h());

    for (i = img->w(); i > 0; i --, rgb += img->d())
    {
      if (img->d() == 1)
	gray = rgb[0];
      else
	gray = (rgb[0] * 31 + rgb[1] * 61 + rgb[2] * 8) / 100;

      hist[gray / 16] ++;
    }
  }

  for (i = 0, temp = 0; i < 16; i ++)
  {
    if (hist[i] > temp)
      temp = hist[i];

#ifdef DEBUG
    printf("hist[%d] = %d\n", i, hist[i]);
#endif // DEBUG
  }

  temp /= 16;

#ifdef DEBUG
  printf("histogram limit = %d\n", temp);
#endif // DEBUG

  for (i = 0; i < 8; i ++)
    if (hist[i] > temp)
      break;

  mingray = i * 16 - 8;

  if (hist[15] > temp && hist[14] < temp)
    hist[15] = 0;

  for (i = 15; i > 8; i --)
    if (hist[i] > temp)
      break;

  maxgray = i * 16 + 23;

#ifdef DEBUG
  printf("mingray=%d, maxgray=%d\n", mingray, maxgray);
#endif // DEBUG

  // See if there is a big change...
  gray = maxgray - mingray;
  if (gray < 2 || gray > 254)
  {
    display_->redraw();
    return;
  }

  if (gray < 192)
    gray = 192;

  // Build a LUT...
  for (i = 0; i < 256; i ++)
  {
    temp = 255 * (i - mingray) / gray;
    if (temp < 0)
      lut[i] = 0;
    else if (temp > 255)
      lut[i] = 255;
    else
      lut[i] = temp;
  }

  // Adjust things...
  for (y = img->h(), rgb = (uchar *)img->data()[0]; y > 0; y --)
  {
    if (!(y & 15))
      image_progress(_("Adjusting Image..."),
                     100 * (img->h() - y) / img->h());

    for (i = img->w() * img->d(); i > 0; i --, rgb ++)
      *rgb = lut[*rgb];
  }

  // Mark the image as changed and redraw...
  image_item_->changed |= CHANGE_TOUCHUP;
  display_->redraw();
  browser_->redraw();
}


//
// 'flphoto::redeye_cb()' - Remove red-eye from the image.
//

void
flphoto::redeye_cb()
{
  uchar			*data;			// Image data
  int			x, y,			// Looping vars
			w,			// Byte width of line
			sx, sy,			// Start X,Y
			ex, ey,			// End X,Y
			r, g, b;		// RGB values
  Fl_Shared_Image	*img;			// Current image


  // Get the current image...
  img = display_->value();
  if (!img)
    return;

  // Can't remove red-eye in grayscale images...
  if (img->d() != 3)
    return;

  // Scan the image looking for obviously red pixels, and then adjust
  // them along with surrounding pixels to be grayscale...
  w = img->w() * 3;

  if (display_->start_x() < display_->mouse_x())
  {
    sx = display_->start_x();
    ex = display_->mouse_x();
  }
  else
  {
    sx = display_->mouse_x();
    ex = display_->start_x();
  }

  if (display_->start_y() < display_->mouse_y())
  {
    sy = display_->start_y();
    ey = display_->mouse_y();
  }
  else
  {
    sy = display_->mouse_y();
    ey = display_->start_y();
  }

  if ((ex - sx) < 2 && (ey - sy) < 2)
  {
    sx -= 10;
    ex += 10;
    sy -= 10;
    ey += 10;
  }

  if (sx < 0)
    sx = 0;
  if (sy < 0)
    sy = 0;
  if (ex >= img->w())
    ex = img->w() - 1;
  if (ey >= img->h())
    ey = img->h() - 1;

  // Remove red/cyan pixels inside this area...
  for (y = sy; y <= ey; y ++)
    for (x = sx, data = (uchar *)img->data()[0] + w * y + x * 3;
         x <= ex;
	 x ++)
    {
      r = *data++;
      g = *data++;
      b = *data++;

      if ((r > (3 * g / 2) && r > (3 * b / 2)) || (g > r && b > r))
      {
	// Mark the image as changed...
	image_item_->changed |= CHANGE_TOUCHUP;

	// Convert red/cyan-eye to grayscale...
	memset(data - 3, (r * 31 + g * 61 + b * 8) / 100, 3);
      }
    }

  // Redraw...
  display_->redraw();
  browser_->redraw();
}


//
// 'flphoto::blur_cb()' - Blur the image.
//

void
flphoto::blur_cb()
{
  Fl_Shared_Image	*img;			// Current image...


  // Get the current image...
  img = display_->value();
  if (!img)
    return;

  // Blur the image...
  blur_image((uchar *)img->data()[0], img->w(), img->h(), img->d(), 5, 0.5);

  // Mark the image as changed and redraw...
  image_item_->changed |= CHANGE_TOUCHUP;
  display_->redraw();
  browser_->redraw();
}


//
// 'flphoto::sharpen_cb()' - Sharpen the image.
//

void
flphoto::sharpen_cb()
{
  Fl_Shared_Image	*img;			// Current image
  int			x, y;			// Looping vars
  uchar			*blur,			// Blurred image 
			*blurptr,		// Pointer into blurred image
			*dataptr;		// Pointer into original image
  int			val;			// New image value


  // Get the current image...
  img = display_->value();
  if (!img)
    return;

  // Create a blurred image...
  blur = copy_image(img);
  blur_image(blur, img->w(), img->h(), img->d(), 5, 0.5);

  // Then sharpen against the blurred image...
  for (y = 0, blurptr = blur, dataptr = (uchar *)img->data()[0];
       y < img->h();
       y ++)
  {
    if (!(y & 15))
      image_progress(_("Sharpening Image..."), 100 * y / img->h());

    for (x = img->w() * img->d(); x > 0; x --)
    {
      val = (*dataptr - *blurptr++) / 2 + *dataptr;
      if (val < 0)
        *dataptr++ = 0;
      else if (val > 255)
        *dataptr++ = 255;
      else
        *dataptr++ = val;
    }
  }

  // Delete the blurred image...
  delete[] blur;

  // Mark the image as changed and redraw...
  image_item_->changed |= CHANGE_TOUCHUP;
  display_->redraw();
  browser_->redraw();
}


//
// 'flphoto::crop_cb()' - Crop an image.
//

void
flphoto::crop_cb()
{
  int			W, H;			// Preview width and height
  Fl_Shared_Image	*img;			// Current image


  // Get the current image...
  img = display_->value();
  if (!img)
    return;

  // Setup the maximums for the fields...
  crop_left_value_->range(0.0, img->w());
  crop_left_value_->value(0.0);
  crop_right_value_->range(0.0, img->w());
  crop_right_value_->value(0.0);
  crop_top_value_->range(0.0, img->h());
  crop_top_value_->value(0.0);
  crop_bottom_value_->range(0.0, img->h());
  crop_bottom_value_->value(0.0);

  // Make the preview image...
  W = 160;
  H = W * img->h() / img->w();

  if (H > 160)
  {
    H = 160;
    W = H * img->w() / img->h();
  }

  crop_image_ = (Fl_Shared_Image *)img->copy(W, H);
  crop_data_  = new uchar[W * H * img->d()];

  memcpy(crop_data_, crop_image_->data()[0], W * H * img->d());

  crop_preview_box_->image(crop_image_);

  // Show the window...
  crop_window_->hotspot(crop_window_);
  crop_window_->show();

  crop_update_cb(crop_aspect_chooser_);
}


//
// 'flphoto::crop_ok_cb()' - Actually do the cropping of the image.
//

void
flphoto::crop_ok_cb()
{
  Fl_Shared_Image	*src,			// Source image
			*dst;			// Destination image
  int			left, right,		// Left and right margins
			top, bottom;		// Bottom and top margins
  int			y;			// Current row
  const uchar		*srcptr;		// Pointer into source image
  uchar			*dstptr;		// Pointer into destination image
  int			srcbytes,		// Number of bytes in source row
			dstbytes;		// Number of bytes in destination row


  // Don't need the preview image anymore...
  crop_preview_box_->image(0);
  crop_window_->hide();
  window_->cursor(FL_CURSOR_WAIT);
  Fl::check();

  // Compute the crop area...
  src      = image_item_->image;
  left     = (int)crop_left_value_->value();
  right    = src->w() - (int)crop_right_value_->value();
  top      = (int)crop_top_value_->value();
  bottom   = src->h() - (int)crop_bottom_value_->value();

  // Make a new image of the proper size...
  dst      = (Fl_Shared_Image *)src->copy(right - left, bottom - top);

  if (dst != src)
  {
    // Copy the pixel data...
    srcptr   = (uchar *)src->data()[0] + (top * src->w() + left) * src->d();
    srcbytes = src->w() * src->d();
    dstptr   = (uchar *)dst->data()[0];
    dstbytes = dst->w() * dst->d();

    for (y = dst->h(); y > 0; y --, srcptr += srcbytes, dstptr += dstbytes)
      memcpy(dstptr, srcptr, dstbytes);

    // Reset the display...
    display_->value(dst);

    image_item_->changed |= CHANGE_CROP;
    image_item_->image->release();
    image_item_->image = dst;
    image_item_->make_thumbnail();
    browser_->damage(FL_DAMAGE_SCROLL);
  }

  // Free the preview images...
  crop_image_->release();
  delete[] crop_data_;

  window_->cursor(FL_CURSOR_DEFAULT);
}


//
// 'flphoto::crop_update_cb()' - Update the preview image.
//

void
flphoto::crop_update_cb(Fl_Widget *widget)
					// I - Preview widget
{
  int			left, right,	// New left/right values
			bottom, top;	// New bottom/top values
  int			x, y, z;	// Looping vars
  int			w, h;		// Crop aspect width/height
  uchar			*src,		// Pointer into source image
			*dst;		// Pointer into destination image
  Fl_Shared_Image	*img;		// Current image
  int			i;		// Looping var


  // Get the current image...
  img = display_->value();
  if (!img)
    return;

  if (widget == crop_preview_box_)
  {
    // Update the crop box...
    x = Fl::event_x() - crop_preview_box_->x() -
        (crop_preview_box_->w() - crop_image_->w()) / 2;
    if (x < 0)
      x = 0;
    else if (x > crop_image_->w())
      x = crop_image_->w();

    y = Fl::event_y() - crop_preview_box_->y() -
        (crop_preview_box_->h() - crop_image_->h()) / 2;
    if (y < 0)
      y = 0;
    else if (y > crop_image_->h())
      y = crop_image_->h();

    x = img->w() * x / crop_image_->w();
    y = img->h() * y / crop_image_->h();

#ifdef DEBUG
    printf("x=%d, y=%d\n", x, y);
#endif // DEBUG

    switch (Fl::event())
    {
      case FL_PUSH :
          crop_left_value_->value(x);
          crop_top_value_->value(y);
          crop_right_value_->value(0);
          crop_bottom_value_->value(0);
	  break;

      case FL_DRAG :
          crop_right_value_->value(img->w() - x);
          crop_bottom_value_->value(img->h() - y);
	  break;

      case FL_RELEASE :
	  if (x < crop_left_value_->value())
	  {
	    crop_right_value_->value(img->w() - crop_left_value_->value());
            crop_left_value_->value(x);
	  }
	  else
            crop_right_value_->value(img->w() - x);

	  if (y < crop_top_value_->value())
	  {
	    crop_bottom_value_->value(img->h() - crop_top_value_->value());
            crop_top_value_->value(y);
	  }
	  else
            crop_bottom_value_->value(img->h() - y);
          break;
    }
  }

  // Maintain aspect ratio as needed...
  if (crop_aspect_chooser_->value())
  {
    if (crop_aspect_chooser_->value() == 1)
    {
      // Crop using image's aspect ratio...
      w = img->w();
      h = img->h();

      for (i = 2; i < w && i < h;)
      {
        if (w == ((w / i) * i) && h == ((h / i) * i) &&
	    (w / i) > 2 && (h / i) > 2)
	{
	  w /= i;
	  h /= i;
	}
	else
	  i ++;
      }
    }
    else if (crop_aspect_chooser_->value() == 2)
    {
      w = (int)crop_aspect_x_->value();
      if (w == 0) w = 1; // Avoid division by zero
      h = (int)crop_aspect_y_->value();
      if (h == 0) h = 1; // Avoid division by zero
    }
    else
    {
      // Crop using custom aspect ratio...
      sscanf(crop_aspect_chooser_->text(crop_aspect_chooser_->value()),
             "%d:%d", &w, &h);
    }

    if (widget == crop_aspect_chooser_)
    {
      crop_aspect_x_->value(w);
      crop_aspect_y_->value(h);

      if (crop_aspect_chooser_->value() == 2)
        crop_aspect_custom_->activate();
      else
        crop_aspect_custom_->deactivate();
    }

    x = img->w() - (int)crop_left_value_->value() -
	(int)crop_right_value_->value();
    y = img->h() - (int)crop_top_value_->value() -
	(int)crop_bottom_value_->value();

    if ((x * h) != (y * w))
    {
      y = x * h / w;

      if ((y + crop_top_value_->value()) > img->h())
      {
        y = img->h() - (int)crop_top_value_->value();
	x = y * w / h;

	crop_right_value_->value(img->w() - x - crop_left_value_->value());
	crop_bottom_value_->value(0);
      }
      else
	crop_bottom_value_->value(img->h() - y - crop_top_value_->value());
    }
  }
  else
  {
    crop_aspect_x_->value(1);
    crop_aspect_y_->value(1);
  }

  // Display the crop box...
  left   = (int)(crop_image_->w() * crop_left_value_->value() /
                 crop_left_value_->maximum());
  right  = crop_image_->w() -
           (int)(crop_image_->w() * crop_right_value_->value() /
                 crop_right_value_->maximum());
  top    = (int)(crop_image_->h() * crop_top_value_->value() /
                 crop_top_value_->maximum());
  bottom = crop_image_->h() -
           (int)(crop_image_->h() * crop_bottom_value_->value() /
                 crop_bottom_value_->maximum());

  if (left > right)
  {
    x     = left;
    left  = right;
    right = x;
  }

  if (top > bottom)
  {
    y      = top;
    top    = bottom;
    bottom = y;
  }

  snprintf(crop_dimensions_label_, sizeof(crop_dimensions_label_),
           _("%d x %d pixels"),
	   (int)(crop_left_value_->maximum() - crop_left_value_->value() -
	         crop_right_value_->value()),
	   (int)(crop_bottom_value_->maximum() - crop_bottom_value_->value() -
	         crop_top_value_->value()));
  crop_dimensions_box_->label(crop_dimensions_label_);
  crop_dimensions_box_->redraw();

#ifdef DEBUG
  printf("left=%d, top=%d, right=%d, bottom=%d\n", left, top, right, bottom);
#endif // DEBUG

  for (src = crop_data_, dst = (uchar *)crop_image_->data()[0], y = 0;
       y < crop_image_->h();
       y ++)
    for (x = 0; x < crop_image_->w(); x ++)
      for (z = 0; z < crop_image_->d(); z ++)
      {
        if (y < top || y >= bottom || x < left || x >= right)
	  *dst++ = *src++ / 4 + 192;
	else
	  *dst++ = *src++;
      }

  // Force the crop image to update...
  crop_image_->uncache();
  crop_preview_box_->redraw();
}


//
// 'flphoto::rotate_image()' - Rotate an image.
//

Fl_Shared_Image *				// O - Rotated image
flphoto::rotate_image(Fl_Shared_Image *img,	// I - Original image
                      int             angle)	// I - Angle to rotate
{
  Fl_Shared_Image	*rot;			// Rotated image
  int			x, y,			// Looping vars
			xstep, ystep;		// Direction in buffers
  const uchar		*imgptr;		// Pointer to source data
  uchar			*rotptr,		// Pointer to destination data
			*rotbase,		// Beginning of image data
			*rotend;		// End of image data


  rot     = (Fl_Shared_Image *)img->copy(img->h(), img->w());
  rotbase = (uchar *)rot->data()[0];
  rotend  = rotbase + rot->w() * rot->h() * rot->d();

  if (angle == 90)
  {
    // Rotate 90 clockwise...
    rotptr = rotbase + (rot->h() - 1) * rot->w() * rot->d();
    xstep  = -rot->w() * rot->d() - rot->d();
    ystep  = (rot->w() * rot->h() + 1) * rot->d();
  }
  else
  {
    // Rotate 90 counter-clockwise...
    rotptr = rotbase + (rot->w() - 1) * rot->d();
    xstep  = rot->w() * rot->d() - rot->d();
    ystep  = -(rot->w() * rot->h() + 1) * rot->d();
  }

  if (img->d() == 1)
  {
    // Rotate grayscale image...
    for (y = 0, imgptr = (const uchar *)img->data()[0];
         y < img->h();
	 y ++, rotptr += ystep)
    {
      if (!(y & 15))
        image_progress(_("Rotating image..."), 100 * y / img->h());

      for (x = img->w(); x > 0; x --, rotptr += xstep)
	if (rotptr < rotbase || rotptr >= rotend)
        {
#ifdef DEBUG
	  printf("rotptr=%p, rotbase=%p, rotend=%p, w=%d, h=%d\n"
	         "    x=%d, y=%d, xstep=%d, ystep=%d\n",
		 rotptr, rotbase, rotend, rot->w(), rot->h(),
		 x, y, xstep, ystep);
#endif // DEBUG
	  y = 1;
	  break;
	}
	else
          *rotptr++ = *imgptr++;
    }
  }
  else
  {
    // Rotate color image...
    for (y = 0, imgptr = (const uchar *)img->data()[0];
         y < img->h();
	 y ++, rotptr += ystep)
    {
      if (!(y & 15))
        image_progress(_("Rotating image..."), 100 * y / img->h());

      for (x = img->w(); x > 0; x --, rotptr += xstep)
	if (rotptr < rotbase || rotptr >= rotend)
        {
#ifdef DEBUG
	  printf("rotptr=%p, rotbase=%p, rotend=%p, w=%d, h=%d\n"
	         "    x=%d, y=%d, xstep=%d, ystep=%d\n",
		 rotptr, rotbase, rotend, rot->w(), rot->h(),
		 x, y, xstep, ystep);
#endif // DEBUG
	  y = 1;
	  break;
	}
	else
	{
          *rotptr++ = *imgptr++;
          *rotptr++ = *imgptr++;
          *rotptr++ = *imgptr++;
	}
    }
  }

  return (rot);
}


//
// 'flphoto::rotate_cb()' - Rotate an image.
//

void
flphoto::rotate_cb(int angle)			// I - Angle to rotate image
{
  Fl_Shared_Image	*rot;			// Rotated image
  int			touchup;		// Change bits


  // Don't do anything if we don't have an image displayed...
  if (!display_->value())
    return;

  window_->cursor(FL_CURSOR_WAIT);
  Fl::check();

  // Create the rotated image...
  rot = rotate_image(image_item_->image, angle);

  display_->value(rot);

  image_item_->image->release();
  image_item_->image = rot;

  // Do the thumbnail, too...
  rot = rotate_image(image_item_->thumbnail, angle);
  image_item_->thumbnail->release();
  image_item_->thumbnail = rot;

  // Redraw the browser and window...
  browser_->redraw();
  display_->redraw();

  // Update the rotation bits without affecting the other change bits...
  touchup = image_item_->changed & ~CHANGE_ROTATE_MASK;

  if (angle == 90)
    image_item_->changed = ((image_item_->changed + 1) & CHANGE_ROTATE_MASK) |
                           touchup;
  else
    image_item_->changed = ((image_item_->changed + 3) & CHANGE_ROTATE_MASK) |
                           touchup;

  window_->cursor(FL_CURSOR_DEFAULT);
}


//
// 'flphoto::scale_cb()' - Scale an image.
//

void
flphoto::scale_cb()
{
  Fl_Shared_Image	*img;			// Current image


  // Get the current image...
  img = display_->value();
  if (!img)
    return;

  // Setup the maximums for the fields...
  scale_xsize_value_->maximum(img->w());
  scale_xsize_value_->value(img->w());
  scale_ysize_value_->maximum(img->h());
  scale_ysize_value_->value(img->h());
  scale_xratio_value_->value(1.0);
  scale_yratio_value_->value(1.0);

  // Show the window...
  scale_window_->hotspot(scale_window_);
  scale_window_->show();
}


//
// 'flphoto::scale_ok_cb()' - Actually do the scaling of the image.
//

void
flphoto::scale_ok_cb()
{
  Fl_Shared_Image	*dst;			// New image


  // Hide the scale window...
  scale_window_->hide();
  window_->cursor(FL_CURSOR_WAIT);
  Fl::check();

  // Make a new image of the proper size...
  dst = (Fl_Shared_Image *)image_item_->image->copy(
            (int)scale_xsize_value_->value(),
            (int)scale_ysize_value_->value());

  if (dst != image_item_->image)
  {
    // Reset the display...
    display_->value(dst);

    image_item_->changed |= CHANGE_SCALE;
    image_item_->image->release();
    image_item_->image = dst;
    image_item_->make_thumbnail();
    browser_->damage(FL_DAMAGE_SCROLL);
  }

  window_->cursor(FL_CURSOR_DEFAULT);
}


//
// 'flphoto::scale_update_cb()' - Update the scaling area.
//

void
flphoto::scale_update_cb(Fl_Widget *w)
{
  Fl_Shared_Image	*img;			// Current image


  img = image_item_->image;

  if (scale_aspect_button_->value())
  {
    // Make sure the new width/height matches..
    if (w == scale_xratio_value_)
      scale_yratio_value_->value(scale_xratio_value_->value());
    else if (w == scale_yratio_value_)
      scale_xratio_value_->value(scale_yratio_value_->value());
    else if (w == scale_ysize_value_)
      scale_xsize_value_->value(scale_ysize_value_->value() * img->w() / img->h());
    else
      scale_ysize_value_->value(scale_xsize_value_->value() * img->h() / img->w());
  }

  if (w == scale_xratio_value_ || w == scale_yratio_value_)
  {
    scale_xsize_value_->value((int)(img->w() * scale_xratio_value_->value()));
    scale_ysize_value_->value((int)(img->h() * scale_yratio_value_->value()));
  }
  else
  {
    scale_xratio_value_->value(scale_xsize_value_->value() / img->w());
    scale_yratio_value_->value(scale_ysize_value_->value() / img->h());
  }
}


//
// 'flphoto::blur_image()' - Blur an image.
//

void
flphoto::blur_image(uchar *data,		// I - Image data
                    int   w,			// I - Width of image
		    int   h,			// I - Height of image
		    int   d,			// I - Depth of image
		    int   radius,		// I - Blur radius
		    float amount)		// I - Amount to blur
{
  uchar	*temp,					// Temporary row
	*tempptr,				// Pointer into row
	*dataptr,				// Pointer into image
	*datastart,				// Start of image data
	*rptr, *rend;				// Blur pointers
  int	x, y, z,				// Looping vars
	r, rmax,				// Current/max radius
	val;					// Current value
  float	a;					// Blur amount
  int	*alut;					// Amount lookup table


  // Prescale the blur amount and build a lookup table...
  for (a = amount, r = radius; r > 0; a += amount * r / (radius + 1), r --);

#ifdef DEBUG
  printf("radius=%d, amount=%.3f, a=%.3f, new amount=%.3f\n",
         radius, amount, a, amount / a / 2);
#endif // DEBUG

  amount /= a * 2;
  rmax   = 2 * radius + 1;
  alut   = new int[rmax];
  for (r = 0; r < rmax; r ++)
    if (r < radius)
      alut[r] = (int)(256 * amount * (r + 1) / (radius + 1));
    else if (r == radius)
      alut[r] = (int)(256 * amount);
    else
      alut[r] = (int)(256 * amount * (rmax - r + 1) / radius);

  // Allocate a row for temporary data...
  if (w > h)
    temp = new uchar[w * d];
  else
    temp = new uchar[h * d];

  // Blur horizontally...
  for (y = 0, datastart = data; y < h; y ++, datastart += w * d)
  {
    if (!(y & 15))
      image_progress(_("Blurring Image..."), 50 * y / h);

    for (dataptr = datastart, tempptr = temp, x = 0; x < w; x ++)
    {
      for (z = 0; z < d; z ++, dataptr ++, tempptr++)
      {
	if (x < radius)
	{
	  rptr = datastart + z;
	  r    = radius - x;
	}
	else
	{
	  rptr = dataptr - radius * d;
	  r    = 0;
	}

	if (x > (w - radius))
	  rend = datastart + w * d;
	else
	  rend = dataptr + radius * d;

        for (val = 0; rptr < rend; rptr += d, r ++)
	  val += *rptr * alut[r];

        if (val < 0)
	  *tempptr = 0;
	else if (val > 0xff00)
	  *tempptr = 255;
	else
	  *tempptr = (unsigned)val >> 8;
      }
    }

    memcpy(datastart, temp, w * d);
  }

  // Blur vertically...
  for (x = 0, datastart = data; x < w; x ++, datastart += d)
  {
    if (!(x & 15))
      image_progress(_("Blurring Image..."), 50 + 50 * x / w);

    for (dataptr = datastart, tempptr = temp, y = 0; y < h; y ++, dataptr += w * d - d)
    {
      for (z = 0; z < d; z ++, dataptr ++, tempptr++)
      {
	if (y < radius)
	{
	  rptr = datastart + z;
	  r    = radius - y;
	}
	else
	{
	  rptr = dataptr - radius * w * d;
	  r    = 0;
	}

	if (y > (h - radius))
	  rend = datastart + h * w * d;
	else
	  rend = dataptr + radius * w * d;

        for (val = 0; rptr < rend; rptr += w * d, r ++)
	  val += *rptr * alut[r];

        if (val < 0)
	  *tempptr = 0;
	else if (val > 0xff00)
	  *tempptr = 255;
	else
	  *tempptr = (unsigned)val >> 8;
      }
    }

    if (d == 1)
    {
      for (y = 0, dataptr = datastart, tempptr = temp; y < h; y ++, dataptr += w - 1)
        *dataptr++ = *tempptr++;
    }
    else
    {
      for (y = 0, dataptr = datastart, tempptr = temp; y < h; y ++, dataptr += w * d - d)
      {
        *dataptr++ = *tempptr++;
        *dataptr++ = *tempptr++;
        *dataptr++ = *tempptr++;
      }
    }
  }

  // Free temporary data...
  delete[] temp;
  delete[] alut;
}


//
// 'flphoto::copy_image()' - Copy image data.
//

uchar *						// O - Image data
flphoto::copy_image(Fl_Shared_Image *img)	// I - Image
{
  uchar	*temp;					// Image data


  // Allocate a temporary buffer and copy the image data...
  temp = new uchar[img->w() * img->h() * img->d()];
  memcpy(temp, img->data()[0], img->w() * img->h() * img->d());
  return (temp);
}


//
// 'flphoto::props_image_cb()' - Show image properties.
//

void
flphoto::props_image_cb()
{
  char		text[1024];			// EXIF data text
  const char	*date_time,			// Date/time
		*make_model,			// Make/model
		*flash;				// Flash mode
  double	exposure,			// Exposure
		f_number,			// F stop
		focal_length,			// Focal length
                distance_value;			// Distance value
  int		iso_speed;			// Sensitivity
  char		distance[255];			// Distance
  Fl_EXIF_Data	*data;				// EXIF data


  // Don't show props unless we have an image open...1
  if (!image_item_)
    return;

  // Show both the comments and EXIF data...
  props_comments_field_->resize(10, 25, 285, 110);
  props_comments_field_->value(image_item_->comments ? image_item_->comments : "");
  props_exif_field_->show();

  // Load EXIF data as needed...
  if (!strcasecmp(fl_filename_ext(image_item_->filename), ".jpg"))
  {
    if ((data = new Fl_EXIF_Data(image_item_->filename)) != NULL)
    {
      if ((date_time = data->get_ascii(Fl_EXIF_Data::TAG_DATE_TIME)) == NULL)
        date_time = data->get_ascii(Fl_EXIF_Data::TAG_KODAK_DATE_TIME_ORIGINAL);

      make_model     = data->get_ascii(Fl_EXIF_Data::TAG_MODEL);
      exposure       = data->get_rational(Fl_EXIF_Data::TAG_EXPOSURE_TIME);
      f_number       = data->get_rational(Fl_EXIF_Data::TAG_F_NUMBER);
      iso_speed      = data->get_integer(Fl_EXIF_Data::TAG_ISO_SPEED_RATINGS);
      distance_value = data->get_rational(Fl_EXIF_Data::TAG_SUBJECT_DISTANCE);
      focal_length   = data->get_rational(Fl_EXIF_Data::TAG_FOCAL_LENGTH);

      if (distance_value == 0.0f)
        strcpy(distance, "unknown");
      else if (distance_value < 0.0f)
        strcpy(distance, "infinity");
      else
        snprintf(distance, sizeof(distance), "%.2fm", distance_value);

      if (data->get_integer(Fl_EXIF_Data::TAG_FLASH) > 0)
        flash = "Flash";
      else
        flash = "No Flash";

      if (exposure < 1.0)
	snprintf(text, sizeof(text),
	         "%s\n%s\n1/%.0fs  F%.1f  ISO %d\n%.0fmm @ %s\n",
		 date_time ? date_time : "NO DATE INFO",
		 make_model ? make_model : "NO MAKE MODEL",
		 1.0 / exposure, f_number, iso_speed, focal_length, distance);
      else
	snprintf(text, sizeof(text),
	         "%s\n%s\n%.1fs  F%.1f  ISO %d\n%.0fmm @ %s\n",
		 date_time ? date_time : "NO DATE INFO",
		 make_model ? make_model : "NO MAKE MODEL",
		 exposure, f_number, iso_speed, focal_length, distance);

      props_exif_field_->value(text);

      delete data;
    }
    else
      props_exif_field_->value(_("No EXIF information available."));
  }
  else
    props_exif_field_->value(_("No EXIF information available."));

  props_window_->hotspot(props_window_);
  props_window_->show();
}


//
// 'flphoto::save_changed_image_cb()' - Save changed images.
//

void
flphoto::save_changed_image_cb()
{
  int			i;			// Looping var
  int			batch;			// Batch mode
  Fl_Image_Browser::ITEM *item;			// Current item


  for (i = 0, batch = 0; i < browser_->count(); i ++)
  {
    item = browser_->value(i);

    if (item->changed)
    {
      // "Open" the image and save it...
      open_image_cb(i);
      save_image(item->filename, batch);

      // Don't show the save dialog for the rest of the images...
      batch = 1;
    }
  }
}




//
// Selected image operations...
//

//
// 'flphoto::edit_selected_image_cb()' - Edit selected images.
//

void
flphoto::edit_selected_image_cb()
{
#ifdef WIN32
  fl_message(_("Sorry, this is not yet implemented!"));
#else
  int			i;			// Looping var...
  Fl_Image_Browser::ITEM *item;			// Current item
  int			argc;			// Number of arguments
  char			**argv;			// Argument array
  int			pid;			// Process ID...
  char			command[1024];		// Image editor


  for (i = 0, argc = 1; i < browser_->count(); i ++)
  {
    item = browser_->value(i);

    if (item->selected)
      argc ++;
  }

  if (argc == 1)
  {
    fl_message(_("Please select one or more images to edit."));
    return;
  }

  if (!fl_choice(_("Are you sure you want to edit all of the selected images?"),
                 _("Cancel"), _("Edit"), NULL))
    return;

  argv = new char *[argc + 1];

  prefs.get("image_editor", command, "gimp", sizeof(command));

  argv[0] = command;

  for (i = 0, argc = 1; i < browser_->count(); i ++)
  {
    item = browser_->value(i);

    if (item->selected)
    {
      argv[argc] = item->filename;
      argc ++;
    }
  }

  argv[argc] = NULL;

  if ((pid = fork()) == 0)
  {
    execvp(command, argv);
    exit(errno);
  }

  delete[] argv;
#endif // WIN32
}


//
// 'flphoto::save_selected_image_cb()' - Save selected images.
//

void
flphoto::save_selected_image_cb()
{
  int			i;			// Looping var
  int			batch;			// Batch mode
  Fl_Image_Browser::ITEM *item;			// Current item


  if (!fl_choice(_("Are you sure you want to save all of the selected images?"),
                 _("Cancel"), _("Save All"), NULL))
    return;

  for (i = 0, batch = 0; i < browser_->count(); i ++)
  {
    item = browser_->value(i);

    if (item->selected && item->changed)
    {
      // "Open" the image and save it...
      open_image_cb(i);
      save_image(item->filename, batch);

      // Don't show the save dialog for the rest of the images...
      batch = 1;
    }
  }
}


//
// 'flphoto::revert_selected_image_cb()' - Revert selected images.
//

void
flphoto::revert_selected_image_cb()
{
  int			i;			// Looping var
  Fl_Image_Browser::ITEM *item;			// Current item


  if (!fl_choice(_("Are you sure you want to revert all of the selected images?"),
                 _("Cancel"), _("Revert"), NULL))
    return;

  for (i = 0; i < browser_->count(); i ++)
  {
    item = browser_->value(i);

    if (item->selected && item->changed)
    {
      // "Open" the image and revert it...
      open_image_cb(i);
      revert_image_cb();
    }
  }
}


//
// 'flphoto::purge_selected_image_cb()' - Purge selected images.
//

void
flphoto::purge_selected_image_cb()
{
  int			i;			// Looping var
  Fl_Image_Browser::ITEM *item;			// Current item
  int			yes_to_all;		// Yes to all


  yes_to_all = 0;
  for (i = 0; i < browser_->count(); i ++)
    if (browser_->selected(i))
    {
      item = browser_->value(i);

      if (!yes_to_all)
      {
	switch (fl_choice(_("Purging %s will delete the file from disk.\n\nReally delete?"),
                	  _("Keep Image File"), _("Delete Image File"),
			  _("Delete All Selected Files"), item->label))
	{
	  case 0 : // Keep
	      return;

	  case 2 : // Delete all
              yes_to_all = 1;
              break;
	}
      }

      unlink(item->filename);
      unlink(item->thumbname);

      if (item == image_item_)
      {
	display_->value(0);
	image_item_ = 0;
      }

      browser_->remove(i);
      update_stats();
      i --;

      album_changed_ = 1;
    }

  browser_->redraw();
  update_title();
}


//
// 'flphoto::remove_selected_image_cb()' - Remove selected images.
//

void
flphoto::remove_selected_image_cb()
{
  int			i;			// Looping var
  Fl_Image_Browser::ITEM *item;			// Current item


  if (!fl_choice(_("Are you sure you want to remove all of the selected images?"),
                 _("Cancel"), _("Remove Image"), NULL))
    return;

  for (i = 0; i < browser_->count(); i ++)
    if (browser_->selected(i))
    {
      item = browser_->value(i);

      if (item->changed)
      {
        browser_->make_visible(i);

        if (!fl_choice(_("%s has been modified, really delete?"),
                       _("Cancel"), _("Delete Image"), NULL, item->label))
	  continue;
      }

      if (item == image_item_)
      {
	display_->value(0);
	image_item_ = 0;
      }

      browser_->remove(i);
      update_stats();
      i --;

      album_changed_ = 1;
    }

  browser_->redraw();
  update_title();
}


//
// 'flphoto::auto_adjust_selected_cb()' - Auto-adjust selected images.
//

void
flphoto::auto_adjust_selected_cb()
{
  int			i;			// Looping var
  Fl_Image_Browser::ITEM *item;			// Current item
  int			batch;			// Batch flag


  if (!fl_choice(_("Are you sure you want to auto-adjust and save all of the selected images?"),
                 _("Cancel"), _("Adjust Images and Save"), NULL))
    return;

  for (i = 0, batch = 0; i < browser_->count(); i ++)
    if (browser_->selected(i))
    {
      // "Open" the image and auto adjust it...
      item = browser_->value(i);

      open_image_cb(i);
      auto_adjust_cb();

      // Save the image...
      save_image(item->filename, batch);
      batch = 1;
    }
}


//
// 'flphoto::adjust_levels_selected_cb()' - Adjust levels in selected images.
//

void
flphoto::adjust_levels_selected_cb()
{
  int			i;			// Looping var
  Fl_Image_Browser::ITEM *item;			// Current item
  int			batch;			// Batch flag


  if (!fl_choice(_("Are you sure you want to adjust the brightness and contrast\nand save all of the selected images?"),
                 _("Cancel"), _("Adjust Images and Save"), NULL))
    return;

  for (i = 0, batch = 0; i < browser_->count(); i ++)
    if (browser_->selected(i))
    {
      // "Open" the image and adjust levels...
      item = browser_->value(i);

      open_image_cb(i);
      adjust_levels_cb();

      // Save the image...
      save_image(item->filename, batch);
      batch = 1;
    }
}


//
// 'flphoto::blur_selected_cb()' - Blur selected images.
//

void
flphoto::blur_selected_cb()
{
  int			i;			// Looping var
  Fl_Image_Browser::ITEM *item;			// Current item
  int			batch;			// Batch flag


  if (!fl_choice(_("Are you sure you want to blur and save all of the selected images?"),
                 _("Cancel"), _("Blur Images and Save"), NULL))
    return;

  for (i = 0, batch = 0; i < browser_->count(); i ++)
    if (browser_->selected(i))
    {
      // "Open" the image and blur it...
      item = browser_->value(i);

      open_image_cb(i);
      blur_cb();

      // Save the image...
      save_image(item->filename, batch);
      batch = 1;
    }
}


//
// 'flphoto::sharpen_selected_cb()' - Sharpen selected images.
//

void
flphoto::sharpen_selected_cb()
{
  int			i;			// Looping var
  Fl_Image_Browser::ITEM *item;			// Current item
  int			batch;			// Batch flag


  if (!fl_choice(_("Are you sure you want to sharpen and save all of the selected images?"),
                 _("Cancel"), _("Sharpen Images and Save"), NULL))
    return;

  for (i = 0, batch = 0; i < browser_->count(); i ++)
    if (browser_->selected(i))
    {
      // "Open" the image and sharpen it...
      item = browser_->value(i);

      open_image_cb(i);
      sharpen_cb();

      // Save the image...
      save_image(item->filename, batch);
      batch = 1;
    }
}


//
// 'flphoto::crop_selected_cb()' - Crop selected images.
//

void
flphoto::crop_selected_cb()
{
  int			i;			// Looping var
  Fl_Image_Browser::ITEM *item;			// Current item
  int			batch;			// Batch flag


  if (!fl_choice(_("Are you sure you want to crop and save all of the selected images?"),
                 _("Cancel"), _("Crop Images and Save"), NULL))
    return;

  for (i = 0, batch = 0; i < browser_->count(); i ++)
    if (browser_->selected(i))
    {
      // "Open" the image and crop it...
      item = browser_->value(i);

      open_image_cb(i);
      crop_cb();

      // Save the image...
      save_image(item->filename, batch);
      batch = 1;
    }
}


//
// 'flphoto::rotate_selected_cb()' - Rotate selected images.
//

void
flphoto::rotate_selected_cb(int angle)		// I - Angle of rotation
{
  int			i;			// Looping var
  Fl_Image_Browser::ITEM *item;			// Current item
  int			batch;			// Batch flag


  if (!fl_choice(_("Are you sure you want to rotate and save all of the selected images?"),
                 _("Cancel"), _("Rotate Images and Save"), NULL))
    return;

  for (i = 0, batch = 0; i < browser_->count(); i ++)
    if (browser_->selected(i))
    {
      // "Open" the image and rotate it...
      item = browser_->value(i);

      open_image_cb(i);
      rotate_cb(angle);

      // Save the image...
      save_image(item->filename, batch);
      batch = 1;
    }
}


//
// 'flphoto::scale_selected_cb()' - Scale selected images.
//

void
flphoto::scale_selected_cb()
{
  int			i;			// Looping var
  Fl_Image_Browser::ITEM *item;			// Current item
  int			batch;			// Batch flag


  if (!fl_choice(_("Are you sure you want to scale and save all of the selected images?"),
                 _("Cancel"), _("Scale Images and Save"), NULL))
    return;

  for (i = 0, batch = 0; i < browser_->count(); i ++)
    if (browser_->selected(i))
    {
      // "Open" the image and scale it...
      item = browser_->value(i);

      open_image_cb(i);
      scale_cb();

      // Save the image...
      save_image(item->filename, batch);
      batch = 1;
    }
}


//
// End of "$Id: image.cxx 416 2006-11-18 13:56:11Z mike $".
//
