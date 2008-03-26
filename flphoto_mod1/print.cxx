//
// "$Id: print.cxx 441 2006-12-21 16:46:09Z mike $"
//
// Print methods.
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
//   flphoto::print_album_cb() - Print an album.
//   flphoto::print_image_cb() - Print an image.
//   flphoto::print_cb()       - Generate the print file.
//   get_moon_phase()          - Get the phase of the moon.
//   make_optimal_image()      - Make an optimally-sized copy of an image.
//   write_ascii85()           - Print binary data as a series of base-85 numbers.
//   write_asciihex()          - Print binary data as a series of hex numbers.
//   write_calendar()          - Write a calendar.
//   write_end_page()          - Show the current page.
//   write_image()             - Write an image at the specified location, mode, and
//   write_matting()           - Write the matting around an image...
//   write_prolog()            - Write the PostScript prolog...
//   write_ps_string()         - Write a PostScript string...
//   write_start_page()        - Start a page.
//   write_trailer()           - Write the PostScript trailer...
//   fl_datefile::load()       - Load datefile.
//   fl_datefile::lookup()     - Lookup date.
//

#include "flphoto.h"
#include "i18n.h"
#include <stdio.h>
#include <stdlib.h>
#include "flstring.h"
#if defined(WIN32) && !defined(__CYGWIN__)
#  include <io.h>
#else
#  include <unistd.h>
#endif // WIN32 && !__CYGWIN__
#include <errno.h>
#include <time.h>
#include "Fl_Print_Dialog.H"
#include <FL/Fl_File_Chooser.H>
#include <math.h>


//
// Local structure...
//

struct fl_datefile
{
  int	year,				// Year
	month,				// Month
	day,				// Day
	repeat;				// Repeat every year?
  char	*message;			// Message

  static fl_datefile	*load(const char *filename, int *num_dates);
  static fl_datefile	*lookup(int y, int m, int d, int num_dates,
			        fl_datefile *dates, int &first);
};


//
// Moon data...
//

#define MOON_PERIOD (int)(29.53058867 * 86400)

enum MoonPhase
{
  MOON_TRANSITION,
  MOON_NEW,
  MOON_FIRSTQ,
  MOON_FULL,
  MOON_LASTQ
};


//
// Local globals...
//

static Fl_Print_Dialog	*print_dialog = 0;


//
// Local functions...
//

static MoonPhase	get_moon_phase(int year, int month, int day);
static Fl_Shared_Image	*make_optimal_image(Fl_Shared_Image *img, int width,
			                    int length);
static void		write_ascii85(FILE *fp, uchar *data, int length);
static void		write_asciihex(FILE *fp, uchar *data, int length);
static void		write_calendar(FILE *fp, int firstday, int month,
			               int year, int left, int bottom,
				       int right, int top, int num_dates,
				       fl_datefile *dates);
static void		write_end_page(FILE *fp);
static float		write_image(FILE *fp, Fl_Shared_Image *img,
			            int mode, int quality, int left, int bottom,
				    int right, int top, int autorotate);
static void		write_matting(FILE *fp, uchar *data, int mode, int type,
			              int width, int left, int bottom,
				      int right, int top);
static void		write_prolog(FILE *fp, int mode, int left, int bottom,
				     int right, int top);
static void		write_ps_string(FILE *fp, const char *s);
static void		write_start_page(FILE *fp, int page, int width,
			                 int length, int commands);
static void		write_trailer(FILE *fp, int pages);


//
// 'flphoto::print_album_cb()' - Print an album.
//

void
flphoto::print_album_cb()
{
  int	format;				// Print format


  if (!print_dialog)
  {
    print_dialog = new Fl_Print_Dialog();
    print_dialog->callback(print_cb, this);
  }

  flphoto::prefs.get("print_album_format", format, Fl_Print_Dialog::PRINT_INDEX);

  print_dialog->format(format);

  print_dialog->which(Fl_Print_Dialog::PRINT_ALL);

  print_dialog->matcolors(browser_);

  print_dialog->show();

  while (print_dialog->shown())
    Fl::wait();

  flphoto::prefs.set("print_album_format", print_dialog->format());
}


//
// 'flphoto::print_image_cb()' - Print an image.
//

void
flphoto::print_image_cb()
{
  int	i;				// Looping var
  int	format;				// Print format


  if (!print_dialog)
  {
    print_dialog = new Fl_Print_Dialog();
    print_dialog->callback(print_cb, this);
  }

  flphoto::prefs.get("print_image_format", format, Fl_Print_Dialog::PRINT_1UP);

  print_dialog->format(format);

  print_dialog->which(Fl_Print_Dialog::PRINT_CURRENT);

  for (i = 0; i < browser_->count(); i ++)
    if (browser_->selected(i) && browser_->value(i) != image_item_)
    {
      print_dialog->which(Fl_Print_Dialog::PRINT_SELECTED);
      break;
    }

  print_dialog->matcolors(browser_);

  print_dialog->show();

  while (print_dialog->shown())
    Fl::wait();

  flphoto::prefs.set("print_image_format", print_dialog->format());
}


//
// 'flphoto::print_cb()' - Generate the print file.
//

const char *				// O - Print file
flphoto::print_cb(Fl_Print_Dialog *pd,	// I - Print dialog
                  void            *d)	// I - Album
{
  flphoto		*album = (flphoto *)d;
					// Album
#if !defined(WIN32) || defined(__CYGWIN__)
  int			fd;		// Temporary file
#endif // !WIN32 || __CYGWIN__
  FILE			*fp;		// Print file
  char			tmpdir[1024];	// Temporary directory
  int			page;		// Current page
  int			left, bottom,	// Lower left corner
			right, top,	// Upper right corner
			width, length,	// Page size
			pagew, pagel, pageo,
					// Page size and orientation
			loff, toff,	// Offset for margins
			ltemp, btemp,	// Offset for images
			imgw, imgh, imgo,
					// Image size and orientation
			mattype,	// Type of matting to use
			matcomments;	// Show comments in matting?
  uchar			matrgb[3],	// Matting color
			matdata[64 * 64 * 3];
					// Matting image data
  float			matwidth,	// Width of matting
			imgwidth,	// Width of image
			imgheight,	// Height of image
			imgy;		// Bottom of image
  int			imgcols,	// Number of columns of images
			imgrows;	// Number of rows of images
  int			i, x, y, copy, progval, progmax;
					// Looping vars
  int			day, month, year, orient, bound;
					// Calendar settings
  char			datefile[1024];	// Date file
  int			num_dates;	// Number of dates
  fl_datefile		*dates;		// Dates
  int			num_pages,	// Number of pages
			num_images;	// Number of images
  int			images[1000];	// Images to print
  Fl_Shared_Image	*img;		// Current image
  Fl_Image_Browser::ITEM *item;		// Current item in album
  const char		*label;		// Label in album
  int			calpage;	// Calendar page


  if (!pd->print_to_file())
  {
    // First create a temporary file for the print data...
    flphoto::prefs.getUserdataPath(tmpdir, sizeof(tmpdir));

#if defined(WIN32) && !defined(__CYGWIN__)
    // NOTE: this is just a temporary fix...
    snprintf(album->album_printname_, sizeof(album->album_printname_),
             "%s/flphoto%06d", tmpdir, GetCurrentProcessId);

    if ((fp = fopen(album->album_printname_, "wb")) == NULL)
    {
      unlink(album->album_printname_);
      return (0);
    }
#else
    snprintf(album->album_printname_, sizeof(album->album_printname_),
             "%s/flphotoXXXXXX", tmpdir);

    if ((fd = mkstemp(album->album_printname_)) < 0)
      return (0);

    if ((fp = fdopen(fd, "wb")) == NULL)
    {
      close(fd);
      unlink(album->album_printname_);
      return (0);
    }
#endif // WIN32 && !__CYGWIN__
  }
  else
  {
    const char	*f;


    if ((f = fl_file_chooser(_("Print To File?"),
                             _("PostScript Files (*.ps)"), 0, 1)) == NULL)
      return (0);

    if (!access(f, 0))
      if (!fl_choice(_("Print file %s already exists.\nOK to replace?"),
                     _("Cancel"), _("Replace"), NULL, f))
        return (0);

    strlcpy(album->album_printname_, f, sizeof(album->album_printname_));

    if ((fp = fopen(f, "wb")) == NULL)
    {
      fl_alert(_("Unable to create %s:\n\n%s"), f, strerror(errno));
      return (0);
    }
  }

  // Get the media dimensions...
  pd->imageable_area(left, bottom, right, top);
  pd->paper_dimension(width, length);

  // Figure out how many images are getting printed...
  switch (pd->which())
  {
    case Fl_Print_Dialog::PRINT_CURRENT :
	num_images = 1;
	images[0]  = 0;

	for (i = 0; i < album->browser_->count(); i ++)
	  if (album->browser_->value(i) == album->image_item_)
	  {
	    images[0] = i;
	    break;
	  }
        break;

    case Fl_Print_Dialog::PRINT_SELECTED :
        for (i = 0, num_images = 0; i < album->browser_->count(); i ++)
	  if (album->browser_->selected(i))
	  {
	    images[num_images] = i;
	    num_images ++;

	    if (num_images >= (int)(sizeof(images) / sizeof(images[0])))
	      break;
	  }

        if (num_images > 0)
          break;

	pd->which(Fl_Print_Dialog::PRINT_ALL);

    default :
	num_images = album->browser_->count();

	if (num_images > (int)(sizeof(images) / sizeof(images[0])))
	  num_images = (int)(sizeof(images) / sizeof(images[0]));

        for (i = 0; i < num_images; i ++)
	  images[i] = i;
        break;
  }

  // Figure out the total number of pages...
  switch (pd->format())
  {
    case Fl_Print_Dialog::PRINT_CALENDAR :
	pd->calendar(day, month, year, orient, bound, datefile, sizeof(datefile));

        if (bound)
	{
	  if (bound > Fl_Print_Dialog::CALENDAR_FOLDED_ALL_PAGES)
	  {
	    // Bound on facing pages...
	    progmax = num_images * pd->copies();

	    if (bound == Fl_Print_Dialog::CALENDAR_BOUND_ALL_PAGES)
	      progmax *= 2;
	  }
	  else
	  {
	    // Folded pages...
	    progmax = ((num_images + 1) / 2) * pd->copies();

	    if (bound == Fl_Print_Dialog::CALENDAR_FOLDED_ALL_PAGES)
	      progmax *= 2;
	  }

          break;
	}

    default :
        progmax = num_images * pd->copies();
	break;
  }

  // Then write the standard PostScript prolog...
  write_prolog(fp, pd->mode(), left, bottom, right, top);
  page = 0;

  switch (pd->format())
  {
    case Fl_Print_Dialog::PRINT_INDEX :
	for (copy = 0, progval = 0; copy < pd->copies(); copy ++)
	{
          for (i = 0, x = -1, y = -1; i < num_images; i ++)
	  {
	    if (x < 0 || y < 0)
	    {
	      page ++;
	      write_start_page(fp, page, width, length, !pd->have_ppd());

	      fprintf(fp, "(Page %d) %d %d L\n",
	              page, (left + right) / 2, bottom);

	      x = left;
	      y = top;
	    }

            progval ++;
	    pd->progress_show(100 * progval / progmax,
	                      _("Printing page %d..."), page);

            item = album->browser_->value(images[i]);
	    switch (pd->quality())
	    {
	      case 0 : // Draft
	      case 1 : // Normal
        	  img = item->thumbnail;
	          break;
              default : // Best
	          img = make_optimal_image(album->browser_->load_item(images[i]),
		                           54, 54);
		  break;
	    }

	    write_image(fp, img, pd->mode(), pd->quality(), x, y - 54,
	                x + 54, y, 0);

            if (img != item->image && img != item->thumbnail)
	      img->release();

            if ((label = strrchr(item->filename, '/')) != NULL)
	      label ++;
	    else
	      label = item->filename;

            fprintf(fp, "(%s) %d %d L\n", label, x + 27, y - 63);

	    if (item->image && item != album->image_item_ && !item->changed)
	    {
	      item->image->release();
	      item->image = 0;
	    }

	    x += 72;

	    if ((x + 54) > right)
	    {
	      x = left;
	      y -= 72;

	      if ((y - 63) < (bottom + 18))
	      {
		write_end_page(fp);
		x = y = -1;
	      }
	    }
	  }

	  if (x >= 0 || y >= 0)
	    write_end_page(fp);
	}
        break;

    case Fl_Print_Dialog::PRINT_1UP :
	for (copy = 0, progval = 0; copy < pd->copies(); copy ++)
	{
          for (i = 0; i < num_images; i ++)
	  {
	    page ++;
	    progval ++;
	    pd->progress_show(100 * progval / progmax,
	                      _("Printing page %d..."), page);
	    write_start_page(fp, page, width, length, !pd->have_ppd());

            item = album->browser_->value(images[i]);

	    switch (pd->quality())
	    {
	      case 0 : // Draft
	          img = make_optimal_image(album->browser_->load_item(images[i]),
		                           (right - left) / 4,
					   (top - bottom) / 4);
	          break;
	      case 1 : // Normal
	          img = make_optimal_image(album->browser_->load_item(images[i]),
		                           right - left, top - bottom);
	          break;
              default : // Best
                  img = album->browser_->load_item(images[i]);
		  break;
	    }

	    write_image(fp, img, pd->mode(), pd->quality(), left, bottom,
	        	right, top, 1);

            if (img != item->image && img != item->thumbnail)
	      img->release();

	    if (item->image && item != album->image_item_ && !item->changed)
	    {
	      item->image->release();
	      item->image = 0;
	    }

            write_end_page(fp);
	  }
	}
        break;

    case Fl_Print_Dialog::PRINT_2UP :
	for (copy = 0, y = -1, progval = 0; copy < pd->copies(); copy ++)
	{
          for (i = 0; i < num_images; i ++)
	  {
            if (y < 0)
	    {
	      page ++;
	      write_start_page(fp, page, width, length, !pd->have_ppd());
	      y = top;
	    }

            progval ++;
	    pd->progress_show(100 * progval / progmax,
	                      _("Printing page %d..."), page);

            item = album->browser_->value(images[i]);

	    switch (pd->quality())
	    {
	      case 0 : // Draft
	          img = make_optimal_image(album->browser_->load_item(images[i]),
		                           (right - left) / 4,
					   (top - bottom) / 8);
	          break;
	      case 1 : // Normal
	          img = make_optimal_image(album->browser_->load_item(images[i]),
		                           right - left, (top - bottom) / 2);
	          break;
              default : // Best
                  img = album->browser_->load_item(images[i]);
		  break;
	    }

	    write_image(fp, img, pd->mode(), pd->quality(), left,
	                y - (top - bottom) / 2 + 9, right, y, 1);

            if (img != item->image && img != item->thumbnail)
	      img->release();

	    if (item->image && item != album->image_item_ && !item->changed)
	    {
	      item->image->release();
	      item->image = 0;
	    }

            y -= (top - bottom) / 2 + 9;
	    if (y < bottom)
	    {
              write_end_page(fp);
	      y = -1;
	    }
	  }
	}

	if (y >= 0)
	  write_end_page(fp);
        break;

    case Fl_Print_Dialog::PRINT_4UP :
	for (copy = 0, x = -1, y = -1, progval = 0; copy < pd->copies(); copy ++)
	{
          for (i = 0; i < num_images; i ++)
	  {
            if (x < 0 || y < 0)
	    {
	      page ++;
	      write_start_page(fp, page, width, length, !pd->have_ppd());
	      x = left;
	      y = top;
	    }

            progval ++;
	    pd->progress_show(100 * progval / progmax,
	                      _("Printing page %d..."), page);

            item = album->browser_->value(images[i]);

	    switch (pd->quality())
	    {
	      case 0 : // Draft
	          img = make_optimal_image(album->browser_->load_item(images[i]),
		                           (right - left) / 8,
					   (top - bottom) / 8);
	          break;
	      case 1 : // Normal
	          img = make_optimal_image(album->browser_->load_item(images[i]),
		                           (right - left) / 2,
					   (top - bottom) / 2);
	          break;
              default : // Best
                  img = album->browser_->load_item(images[i]);
		  break;
	    }

	    write_image(fp, img, pd->mode(), pd->quality(), x,
	                y - (top - bottom) / 2 + 9,
			x + (right - left) / 2 - 9, y, 1);

            if (img != item->image && img != item->thumbnail)
	      img->release();

	    if (item->image && item != album->image_item_ && !item->changed)
	    {
	      item->image->release();
	      item->image = 0;
	    }

            x += (right - left) / 2 + 9;
	    if (x >= right)
	    {
	      x = left;
              y -= (top - bottom) / 2 + 9;
	      if (y < bottom)
	      {
        	write_end_page(fp);
		x = y = -1;
	      }
	    }
	  }
	}

	if (x >= 0 || y >= 0)
	  write_end_page(fp);
        break;

    case Fl_Print_Dialog::PRINT_PORTRAIT :
        progmax *= 8;

	for (copy = 0, progval = 0; copy < pd->copies(); copy ++)
	{
          for (i = 0; i < num_images; i ++)
	  {
	    page ++;
            progval ++;
	    pd->progress_show(100 * progval / progmax,
	                      _("Printing page %d..."), page);
	    write_start_page(fp, page, width, length, !pd->have_ppd());

            item = album->browser_->value(images[i]);

            int hh = (top - bottom) / 4 - 9;
            int hm = 2 * (top - bottom) / 3;
            int hs = top - bottom - hm - 9;
            int hp = (top - bottom) / 6 - 9;
            int ww = right - left - hh - 18;

            // Main portrait
	    switch (pd->quality())
	    {
	      case 0 : // Draft
	          img = make_optimal_image(album->browser_->load_item(images[i]),
					   ww / 4, (top - bottom - hh - 9) / 4);
	          break;
	      case 1 : // Normal
	          img = make_optimal_image(album->browser_->load_item(images[i]),
		                           ww, top - bottom - hh - 9);
	          break;
              default : // Best
                  img = album->browser_->load_item(images[i]);
		  break;
	    }

            fputs("gsave\n", fp);
	    fprintf(fp, "%d %d %d %d rectclip\n", left, top - hm, ww, hm);
	    write_image(fp, img, pd->mode(), pd->quality(),
	                left, bottom + (hh + hs) / 2 + 9,
			left + ww, top + (hs - hh) / 2, 1);
            fputs("grestore\n", fp);

            progval ++;
	    pd->progress_show(100 * progval / progmax,
	                      _("Printing page %d..."), page);

            if (img != item->image && img != item->thumbnail)
	      img->release();

            // Smaller portrait underneath...
	    switch (pd->quality())
	    {
	      case 0 : // Draft
		  img = make_optimal_image(album->browser_->load_item(images[i]),
		                           ww / 4, hs / 4 + 2);
	          break;
	      case 1 : // Normal
		  img = make_optimal_image(album->browser_->load_item(images[i]),
		                           ww, hs + 8);
	          break;
              default : // Best
                  img = album->browser_->load_item(images[i]);
		  break;
	    }

            fputs("gsave\n", fp);
	    fprintf(fp, "%d %d %d %d rectclip\n", left, bottom, ww, hs);
	    write_image(fp, img, pd->mode(), pd->quality(),
	                left, bottom - 4, left + ww, bottom + hs + 4, 1);
            fputs("grestore\n", fp);

            progval ++;
	    pd->progress_show(100 * progval / progmax,
	                      _("Printing page %d..."), page);

            if (img != item->image && img != item->thumbnail)
	      img->release();

	    switch (pd->quality())
	    {
	      case 0 : // Draft
		  img = make_optimal_image(album->browser_->load_item(images[i]),
		                           hh / 4, hp / 4);
	          break;
	      case 1 : // Normal
		  img = make_optimal_image(album->browser_->load_item(images[i]),
		                           hh, hp);
	          break;
              default : // Best
		  img = make_optimal_image(album->browser_->load_item(images[i]),
		                           hh * 2, hp * 2);
	          break;
	    }

            // Wallet-size portraits to the right...
            for (y = top; y > bottom; y -= hp + 11)
	    {
              progval ++;
	      pd->progress_show(100 * progval / progmax,
	                        _("Printing page %d..."), page);
	      write_image(fp, img, pd->mode(), pd->quality(),
		          right - hh, y - hp, right, y, 1);
            }

            if (img != item->image && img != item->thumbnail)
	      img->release();

	    if (item->image && item != album->image_item_ && !item->changed)
	    {
	      item->image->release();
	      item->image = 0;
	    }

            write_end_page(fp);
	  }
	}
        break;

    case Fl_Print_Dialog::PRINT_CALENDAR :
        dates = fl_datefile::load(datefile, &num_dates);

	for (copy = 0, progval = 0; copy < pd->copies(); copy ++)
	{
	  pd->calendar(day, month, year, orient, bound, datefile,
	               sizeof(datefile));

          switch (bound)
	  {
	    case Fl_Print_Dialog::CALENDAR_FOLDED_FRONT_PAGES :
	    case Fl_Print_Dialog::CALENDAR_FOLDED_BACK_PAGES :
	    case Fl_Print_Dialog::CALENDAR_FOLDED_ALL_PAGES :
		// Print calendars on folded pages...
        	//
		// N month folded calendar: (N+1)/2 pages folded, N must be odd
		// (T/L = top or left, B/R = bottom or right)
		//
		//   Page     T/L Front    B/R Front    T/L Back      B/R Back
		//   -------- ------------ -----------  ------------  -----------
		//   1        Credits      Cover image  m1 image      mN cal
        	//   2        mN image     m1 cal       m2 image      mN-1 cal
        	//   ...
		//   (N+1)/2  m8 image     m6 cal       m7 image      m7 cal
		//

        	// If "auto" orientation is selected, assume "top", since
		// that is the most common...
		if (orient == Fl_Print_Dialog::CALENDAR_AUTO)
		  orient = Fl_Print_Dialog::CALENDAR_TOP;

                // Force an odd number of months...
                num_pages = num_images;

                if (num_images & 1)
		  num_pages ++;

                // Force margins to be the same all around...
                if (left > (width - right))
		  right = width - left;
		else
		  left  = width - right;

                if (bottom > (length - top))
		  top = length - bottom;
		else
		  bottom = length - top;

                // Now print all of the pages...
        	for (i = 0; i < num_pages; i ++)
		{
		  int temp_month, temp_year, temp_image;
					// Temporary month, year, and image


		  // Skip even/odd pages as needed...
		  if (((i & 1) == 0 &&
		       bound == Fl_Print_Dialog::CALENDAR_FOLDED_BACK_PAGES) ||
		      ((i & 1) == 1 &&
		       bound == Fl_Print_Dialog::CALENDAR_FOLDED_FRONT_PAGES))
                    continue;

		  // Figure out the month and year for this page...
                  temp_year = year;

                  if (i == 0)
		  {
		    temp_month = 0;
		    temp_image = images[0];
		  }
		  else if (i & 1)
		  {
		    temp_month = month + num_pages - i / 2 - 2;
		    temp_image = images[i / 2 + 1];
		  }
		  else
		  {
		    temp_month = month + i / 2 - 1;
		    temp_image = num_pages - i / 2;

		    if (temp_image >= num_images || temp_image < 0)
		      temp_image = images[0];
		    else
		      temp_image = images[temp_image];
		  }

                  while (temp_month >= 12)
		  {
		    temp_year ++;
		    temp_month -= 12;
		  }

		  // Start the page...
		  page ++;
		  progval ++;
		  pd->progress_show(100 * progval / progmax,
		                    _("Printing page %d..."), page);
		  write_start_page(fp, page, width, length, !pd->have_ppd());

        	  item = album->browser_->value(temp_image);
        	  img  = album->browser_->load_item(temp_image);

                  if (!img)
		  {
		    printf("Unable to load image #%d!\n", temp_image);
		  }
		  else
        	  if (orient == Fl_Print_Dialog::CALENDAR_LEFT)
        	  {
		    // Show image in landscape orientation...
		    pagew = (top - bottom) / 2;
		    pagel = right - left - 9;
		    pageo = 1;

        	    imgh = pagel;
		    imgw = imgh * img->w() / img->h();

        	    if (imgw > (pagew - 18))
		    {
                      imgw = pagew - 18;
	              imgh = imgw * img->h() / img->w();
		    }

        	    fprintf(fp, "%d 0 translate 90 rotate\n", width);
		    fprintf(fp, "%d %d translate\n", bottom, width - right);
		  }
		  else
		  {
		    // Show image in portrait orientation...
		    pagew = right - left;
		    pagel = (top - bottom) / 2 - 9;
		    pageo = 0;

                    imgw = pagew;
	            imgh = imgw * img->h() / img->w();

        	    if (imgh > (pagel - 18))
		    {
        	      imgh = pagel - 18;
		      imgw = imgh * img->w() / img->h();
		    }

		    fprintf(fp, "%d %d translate\n", left, bottom);
		  }

		  switch (pd->quality())
		  {
		    case 0 : // Draft
	        	img = make_optimal_image(
			          album->browser_->load_item(temp_image), imgw / 4,
				  imgh / 4);
	        	break;
		    case 1 : // Normal
	        	img = make_optimal_image(
			          album->browser_->load_item(temp_image), imgw, imgh);
	        	break;
        	    default : // Best
                	img = album->browser_->load_item(temp_image);
			break;
		  }

                  if (!img)
		  {
        	    write_end_page(fp);
		    continue;
		  }

                  if (i == 0)
		  {
		    // Show credits and cover image...
		    if (pageo)
		    {
		      // Put image on the right side...
		      imgy = write_image(fp, img, pd->mode(), pd->quality(),
	               			 pagew + 18, 9, 2 * pagew, pagel, 0);

                      if (item->comments)
		      {
                	write_ps_string(fp, item->comments);
			fprintf(fp, " %.1f %.1f L\n", 0.5 * (3 * pagew + 18),
			        imgy - 9.0);
		      }

                      // Put credits on the left...
        	      fprintf(fp, _("(Created using flPhoto) %d %d L\n"),
		              pagew / 2, pagel / 2 + 6);
		      fprintf(fp, _("(flPhoto Copyright 2002-2006 by Michael Sweet) %d %d L\n"),
		              pagew / 2, pagel / 2 - 6);
        	    }
		    else
		    {
		      // Put image on the bottom...
		      imgy = write_image(fp, img, pd->mode(), pd->quality(),
	                	         0, 9, pagew, pagel - 18, 0);

                      if (item->comments)
		      {
                	write_ps_string(fp, item->comments);
			fprintf(fp, " %.1f %.1f P\n", 0.5 * pagew, imgy - 9.0);
		      }

                      // Put credits on the top...
		      fprintf(fp, "%d %d translate 180 rotate\n",
		              pagew, 2 * pagel);
        	      fprintf(fp, _("(Created using flPhoto) %d %d L\n"),
		              pagew / 2, pagel / 2 + 6);
		      fprintf(fp, _("(flPhoto Copyright 2002-2006 by Michael Sweet) %d %d L\n"),
		              pagew / 2, pagel / 2 - 6);
        	    }
		  }
		  else
		  {
		    if (pageo)
		    {
		      // Put image on the left side...
		      imgy = write_image(fp, img, pd->mode(), pd->quality(),
	                		 0, 9, pagew - 18, pagel, 0);

                      if (item->comments)
		      {
                	write_ps_string(fp, item->comments);
			fprintf(fp, " %.1f %.1f L\n", 0.5 * (pagew - 18),
			        imgy - 9.0);
		      }

		      // Put calendar on the right side...
        	      write_calendar(fp, day, temp_month, temp_year,
		                     pagew + 18, 0, 2 * pagew, pagel,
		                     num_dates, dates);
        	    }
		    else
		    {
		      // Put image on the top...
		      imgy = write_image(fp, img, pd->mode(), pd->quality(),
	                	         0, pagel + 27, pagew, 2 * pagel, 0);

                      if (item->comments)
		      {
                	write_ps_string(fp, item->comments);
			fprintf(fp, " %.1f %.1f L\n", 0.5 * pagew, imgy - 9.0);
		      }

		      // Put calendar at the bottom...
        	      write_calendar(fp, day, temp_month, temp_year,
		                     0, 0, pagew, pagel - 18,
		                     num_dates, dates);
        	    }
                  }

        	  if (img != item->image && img != item->thumbnail)
		    img->release();

		  if (item->image && item != album->image_item_ && !item->changed)
		  {
		    item->image->release();
		    item->image = 0;
		  }

        	  write_end_page(fp);
	        }
		break;

	    case Fl_Print_Dialog::CALENDAR_BOUND_FRONT_PAGES :
	    case Fl_Print_Dialog::CALENDAR_BOUND_BACK_PAGES :
	    case Fl_Print_Dialog::CALENDAR_BOUND_ALL_PAGES :
		// Print calendars on bound pages...
        	//
		// N month separate bound calendar: N+1 pages double-sided
		// long-edge bound
		//
		//   Page     Front        Back
		//   -------- -----------  -----------
		//   1        Cover image  m1 image
		//   2        m1 cal       m2 image
		//   ...
		//   N+1      mN cal       Credits
		//

                // Force margins to be the same all around...
                if (left > (width - right))
		  right = width - left;
		else
		  left  = width - right;

                if (bottom > (length - top))
		  top = length - bottom;
		else
		  bottom = length - top;

        	// If "auto" orientation is selected, assume "top", since
		// that is the most common...
		if (orient == Fl_Print_Dialog::CALENDAR_AUTO)
		  orient = Fl_Print_Dialog::CALENDAR_TOP;

        	if (orient == Fl_Print_Dialog::CALENDAR_LEFT)
        	{
		  // Show image in portrait orientation...
		  pagew = right - left;
		  pagel = top - bottom - 18;
		  pageo = 0;
		}
		else
		{
		  // Show image in landscape orientation...
		  pagew = top - bottom;
		  pagel = right - left - 18;
		  pageo = 1;
		}

        	for (i = 0, calpage = 0; i < num_images; i ++)
		{
		  // Skip even/odd pages as needed...
		  calpage ++;

		  if (((calpage & 1) &&
		       bound != Fl_Print_Dialog::CALENDAR_BOUND_BACK_PAGES) ||
		      (!(calpage & 1) &&
		       bound != Fl_Print_Dialog::CALENDAR_BOUND_FRONT_PAGES))
                  {
		    // Print an image page...
		    page ++;
		    progval ++;
		    pd->progress_show(100 * progval / progmax,
	                              _("Printing page %d..."), page);
		    write_start_page(fp, page, width, length, !pd->have_ppd());

        	    item = album->browser_->value(images[i]);
        	    img  = album->browser_->load_item(images[i]);

        	    if (orient == Fl_Print_Dialog::CALENDAR_LEFT)
        	    {
		      // Show image in portrait orientation...
                      imgw = pagew;
	              imgh = imgw * img->h() / img->w();

        	      if (imgh > (pagel - 9))
		      {
        		imgh = pagel - 9;
			imgw = imgh * img->w() / img->h();
		      }


		      fprintf(fp, "%d %d translate\n", left,
		              !i ? bottom - 9 : bottom + 9);
		    }
		    else
		    {
		      // Show image in landscape orientation...
        	      imgh = pagel - 9;
		      imgw = imgh * img->w() / img->h();

        	      if (imgw > pagew)
		      {
                	imgw = pagew;
	        	imgh = imgw * img->h() / img->w();
		      }

        	      fprintf(fp, "%d 0 translate 90 rotate\n", width);
		      fprintf(fp, "%d %d translate\n", bottom,
		              !i ? left - 9 : left + 9);
		    }

		    switch (pd->quality())
		    {
		      case 0 : // Draft
	        	  img = make_optimal_image(album->browser_->load_item(images[i]),
		                        	   imgw / 4, imgh / 4);
	        	  break;
		      case 1 : // Normal
	        	  img = make_optimal_image(album->browser_->load_item(images[i]),
		                        	   imgw, imgh);
	        	  break;
        	      default : // Best
                	  img = album->browser_->load_item(images[i]);
			  break;
		    }

        	    if (pageo)
		    {
		      // Put image on the left side...
		      imgy = write_image(fp, img, pd->mode(), pd->quality(),
	                		 0, 9, pagew, pagel, 0);
        	    }
		    else
		    {
		      // Put image on the top...
		      imgy = write_image(fp, img, pd->mode(), pd->quality(),
	                		 0, 9, pagew, pagel, 0);
        	    }

                    if (item->comments)
		    {
                      write_ps_string(fp, item->comments);
		      fprintf(fp, " %.1f %.1f L\n", 0.5 * pagew, imgy - 9.0);
		    }

        	    if (img != item->image && img != item->thumbnail)
		      img->release();

		    if (item->image && item != album->image_item_ &&
			!item->changed)
		    {
		      item->image->release();
		      item->image = 0;
		    }

        	    write_end_page(fp);
                  }

                  // First image is cover page, every other image is a
		  // calendar...
                  if (!i)
		    continue;

                  // See if we need to print the calendar page...
		  calpage ++;

		  if (bound != Fl_Print_Dialog::CALENDAR_BOUND_BACK_PAGES)
		  {
                    // Yes, do a calendar page...
		    page ++;
		    progval ++;
		    pd->progress_show(100 * progval / progmax,
	                              _("Printing page %d..."), page);
		    write_start_page(fp, page, width, length, !pd->have_ppd());

        	    if (pageo)
		    {
		      // Put calendar in landscape orientation...
        	      fprintf(fp, "%d 0 translate 90 rotate\n", width);
		      fprintf(fp, "%d %d translate\n", bottom, left + 9);

        	      write_calendar(fp, day, month, year, 0, 0, pagew, pagel,
		                     num_dates, dates);
        	    }
		    else
		    {
		      // Put calendar in portrait orientation...
		      fprintf(fp, "%d %d translate\n", left, bottom + 9);

        	      write_calendar(fp, day, month, year, 0, 0, pagew, pagel,
		                     num_dates, dates);
        	    }

        	    write_end_page(fp);

                    // Advance to the next month
        	    month ++;
		    if (month > 11)
		    {
		      month = 0;
		      year ++;
		    }
                  }
	        }

                if (bound != Fl_Print_Dialog::CALENDAR_BOUND_FRONT_PAGES)
		{
		  // Last is the credits page...
		  page ++;
		  progval ++;
		  pd->progress_show(100 * progval / progmax,
	                            _("Printing page %d..."), page);
		  write_start_page(fp, page, width, length, !pd->have_ppd());

        	  if (pageo)
		  {
		    // Put credits in landscape orientation...
        	    fprintf(fp, "0 %d translate -90 rotate\n", length);
		    fprintf(fp, "%d %d translate\n", bottom, left);
        	  }
		  else
		  {
		    // Put credits in portrait orientation...
		    fprintf(fp, "%d %d translate\n", left, bottom);
        	  }

        	  fprintf(fp, _("(Created using flPhoto) %d %d L\n"),
		          pagew / 2, pagel / 2 + 6);
		  fprintf(fp, _("(flPhoto Copyright 2002-2006 by Michael Sweet) %d %d L\n"),
		          pagew / 2, pagel / 2 - 6);

        	  write_end_page(fp);
		}
		break;

            default :
		// Print calendars on separate pages...
        	for (i = 0; i < num_images; i ++)
		{
		  page ++;
		  progval ++;
		  pd->progress_show(100 * progval / progmax,
	                            _("Printing page %d..."), page);
		  write_start_page(fp, page, width, length, !pd->have_ppd());

        	  item = album->browser_->value(images[i]);
        	  img  = album->browser_->load_item(images[i]);

        	  if (orient == Fl_Print_Dialog::CALENDAR_LEFT ||
	              (orient == Fl_Print_Dialog::CALENDAR_AUTO &&
		       ((img->w() > img->h() && width > length) ||
	        	(img->w() < img->h() && width < length))))
        	  {
		    // Show page in landscape orientation...
		    pagew = top - bottom;
		    pagel = right - left;
		    pageo = 1;

        	    imgh = pagel - 9;
		    imgw = imgh * img->w() / img->h();

        	    if (imgw > (pagew / 2))
		    {
                      imgw = pagew / 2;
	              imgh = imgw * img->h() / img->w();
		    }

        	    fprintf(fp, "%d 0 translate 90 rotate\n", width);
		    fprintf(fp, "%d %d translate\n", bottom, width - right);
		  }
		  else
		  {
		    // Show image at top...
		    pagew = right - left;
		    pagel = top - bottom;
		    pageo = 0;

        	    imgw = pagew;
		    imgh = imgw * img->h() / img->w();

        	    if (imgh > (pagel / 2 - 9))
		    {
                      imgh = pagel / 2 - 9;
	              imgw = imgh * img->w() / img->h();
		    }

		    fprintf(fp, "%d %d translate\n", left, bottom);
		  }

		  switch (pd->quality())
		  {
		    case 0 : // Draft
	        	img = make_optimal_image(album->browser_->load_item(images[i]),
		                        	 imgw / 4, imgh / 4);
	        	break;
		    case 1 : // Normal
	        	img = make_optimal_image(album->browser_->load_item(images[i]),
		                        	 imgw, imgh);
	        	break;
        	    default : // Best
                	img = album->browser_->load_item(images[i]);
			break;
		  }

        	  if (pageo)
		  {
		    // Put image on the left side...
		    imgy = write_image(fp, img, pd->mode(), pd->quality(),
	                	       0, 9, imgw, pagel, 0);

                    if (item->comments)
		    {
                      write_ps_string(fp, item->comments);
		      fprintf(fp, " %.1f %.1f P\n", 0.5 * imgw, imgy - 9.0);
		    }
        	  }
		  else
		  {
		    // Put image on the top...
		    imgy = write_image(fp, img, pd->mode(), pd->quality(),
	                	       0, pagel - imgh, pagew, pagel, 0);

                    if (item->comments)
		    {
                      write_ps_string(fp, item->comments);
		      fprintf(fp, " %.1f %.1f P\n", 0.5 * pagew, imgy - 9.0);
		    }
        	  }

        	  if (img != item->image && img != item->thumbnail)
		    img->release();

		  if (item->image && item != album->image_item_ && !item->changed)
		  {
		    item->image->release();
		    item->image = 0;
		  }

        	  if (pageo)
		  {
		    // Put calendar on the right side...
        	    write_calendar(fp, day, month, year, imgw + 18, 0, pagew,
		                   pagel, num_dates, dates);
        	  }
		  else
		  {
		    // Put calendar on the bottom...
        	    write_calendar(fp, day, month, year, 0, 0, pagew,
		                   pagel - imgh - 9, num_dates, dates);
        	  }

        	  write_end_page(fp);

        	  month ++;
		  if (month > 11)
		  {
		    month = 0;
		    year ++;
		  }
	        }
		break;
	  }
	}

	if (num_dates)
	  delete[] dates;
        break;


    case Fl_Print_Dialog::PRINT_MATTED :
        pd->matting(mattype, matwidth, matrgb, imgwidth, imgheight, imgcols,
	            imgrows, matcomments);
        pd->matimage(matdata);

        imgw = (int)((imgwidth + matwidth) * imgcols + matwidth);
	imgh = (int)((imgheight + matwidth) * imgrows + matwidth);

        if ((imgw > imgh && width < length) ||
	    (imgw < imgh && width > length))
        {
	  // Show page in landscape orientation...
	  pagew = top - bottom;
	  pagel = right - left;
	  pageo = 1;
	}
	else
	{
	  pagew = right - left;
	  pagel = top - bottom;
	  pageo = 0;
	}

        loff = (int)(pagew - (imgcols + 1) * matwidth -
	             imgcols * imgwidth) / 2;
	toff = pagel - (int)(pagel - (imgrows + 1) * matwidth -
	                     imgrows * imgheight) / 2;

	for (copy = 0, x = -1, y = -1, progval = 0; copy < pd->copies(); copy ++)
	{
          for (i = 0; i < num_images; i ++)
	  {
            if (x < 0 || y < 0)
	    {
	      page ++;
	      write_start_page(fp, page, width, length, !pd->have_ppd());
	      x = 0;
	      y = 0;

              if (pageo)
              {
		// Show page in landscape orientation...
        	fprintf(fp, "%d 0 translate 90 rotate\n", width);
		fprintf(fp, "%d %d translate\n", bottom, width - right);
	      }
	      else
		fprintf(fp, "%d %d translate\n", left, bottom);

	    }

            progval ++;
	    pd->progress_show(100 * progval / progmax,
	                      _("Printing page %d..."), page);

            item = album->browser_->value(images[i]);

            // Figure out the maximum image size...
	    img  = album->browser_->load_item(images[i]);

            if ((img->w() > img->h() && imgheight > imgwidth) ||
                (img->w() < img->h() && imgwidth > imgheight))
            {
	      // Rotate image size...
	      imgo = 1;
              imgw = (int)imgheight;
	      imgh = imgw * img->h() / img->w();
	      if (imgh < (int)imgwidth)
	      {
		imgh = (int)imgwidth;
		imgw = imgh * img->w() / img->h();
	      }
	    }
	    else
	    {
	      // Don't rotate the image...
	      imgo = 0;
              imgw = (int)imgwidth;
	      imgh = imgw * img->h() / img->w();
	      if (imgh < (int)imgheight)
	      {
		imgh = (int)imgheight;
		imgw = imgh * img->w() / img->h();
	      }
	    }

	    switch (pd->quality())
	    {
	      case 0 : // Draft
	          img = make_optimal_image(album->browser_->load_item(images[i]),
		                           imgw / 2, imgh / 2);
	          break;
	      case 1 : // Normal
	          img = make_optimal_image(album->browser_->load_item(images[i]),
		                           imgw, imgh);
	          break;
              default : // Best
                  img = album->browser_->load_item(images[i]);
		  break;
	    }

            ltemp = (int)(loff + x * (imgwidth + matwidth) + matwidth);
	    btemp = (int)(toff - (y + 1) * (imgheight + matwidth));

            fprintf(fp, "gsave %d %d %d %d rectclip\n", ltemp, btemp,
	            (int)imgwidth, (int)imgheight);

            if (imgo)
	      write_image(fp, img, pd->mode(), pd->quality(),
	                  (int)(ltemp - (imgh - imgwidth) / 2),
	                  (int)(btemp - (imgw - imgheight) / 2),
	                  (int)(ltemp - (imgh - imgwidth) / 2) + imgh,
	                  (int)(btemp - (imgw - imgheight) / 2 + imgw),
			  1);
            else
	      write_image(fp, img, pd->mode(), pd->quality(),
	                  (int)(ltemp - (imgw - imgwidth) / 2),
	                  (int)(btemp - (imgh - imgheight) / 2),
	                  (int)(ltemp - (imgw - imgwidth) / 2) + imgw,
	                  (int)(btemp - (imgh - imgheight) / 2 + imgh),
			  1);

            fputs("grestore\n", fp);

            write_matting(fp, matdata, pd->mode(), mattype, (int)matwidth,
	                  (int)(ltemp - matwidth),
	                  (int)(btemp - matwidth),
	                  (int)(ltemp + imgwidth + matwidth),
	                  (int)(btemp + imgheight + matwidth));

            if (matcomments && item->comments)
	    {
	      write_ps_string(fp, item->comments);
	      fprintf(fp, " %.1f %.1f ML\n", ltemp + imgwidth * 0.5,
	              btemp - 0.5 * matwidth);
            }

            if (img != item->image && img != item->thumbnail)
	      img->release();

	    if (item->image && item != album->image_item_ && !item->changed)
	    {
	      item->image->release();
	      item->image = 0;
	    }

            x ++;
	    if (x >= imgcols)
	    {
	      x = 0;
              y ++;
	      if (y >= imgrows)
	      {
        	write_end_page(fp);
		x = y = -1;
	      }
	    }

	    if (pd->which() == Fl_Print_Dialog::PRINT_CURRENT)
	      break;
	  }
	}

	if (x >= 0 || y >= 0)
	  write_end_page(fp);
        break;
  }

  pd->progress_hide();

  write_trailer(fp, page);
  fclose(fp);

  return (album->album_printname_);
}


//
// 'get_moon_phase()' - Get the phase of the moon.
//

static MoonPhase			// O - Moon phase
get_moon_phase(int year,		// I - Year
               int month,		// I - Month
	       int day)			// I - Day
{
  struct tm	date;			// Date info
  time_t	newmoon,		// New moon time
		curmoon;		// Current moon time
  float		moonday;		// Moon phase day
  static int	newmoons[][4] =		// First new moon month, day, hour, and minute
		{
		  { 0, 6, 18, 14 }, 	// 2000
		  { 0, 24, 13, 7 }, 	// 2001
		  { 0, 13, 13, 29 }, 	// 2002
		  { 0, 2, 20, 23 }, 	// 2003
		  { 0, 21, 21, 5 }, 	// 2004
		  { 0, 10, 12, 3 }, 	// 2005
		  { 0, 29, 14, 15 }, 	// 2006
		  { 0, 19, 4, 1 }, 	// 2007
		  { 0, 8, 11, 37 }, 	// 2008
		  { 0, 26, 7, 55 }, 	// 2009
		  { 0, 15, 7, 11 } 	// 2010
		};


  // Get the reference time for the new moon...
  int y = year - 2000;
  if (y < 0)
    y = 0;
  else if (y > 10)
    y = 10;

  date.tm_year = 100 + y;
  date.tm_mon  = newmoons[y][0];
  date.tm_mday = newmoons[y][1];
  date.tm_hour = newmoons[y][2];
  date.tm_min  = newmoons[y][3];
  date.tm_sec  = 0;

  newmoon = mktime(&date);

  // Get the time for the given date...
  date.tm_year = year - 1900;
  date.tm_mon  = month - 1;
  date.tm_mday = day;

  curmoon = mktime(&date);

  // Return the percentage for the phase...
  moonday = ((curmoon - newmoon + MOON_PERIOD) % MOON_PERIOD) / 86400.0;

  // Now return the current phase...
  if (moonday >= 29.0 || moonday < 0.4694)
    return (MOON_NEW);
  else if (moonday >= 5.9 && moonday < 6.9)
    return (MOON_FIRSTQ);
  else if (moonday >= 13.25 && moonday < 14.25)
    return (MOON_FULL);
  else if (moonday > 21.0 && moonday <= 22.0)
    return (MOON_LASTQ);
  else
    return (MOON_TRANSITION);
}


//
// 'make_optimal_image()' - Make an optimally-sized copy of an image.
//

static Fl_Shared_Image	*		// O - Optimal image
make_optimal_image(
    Fl_Shared_Image *img,		// I - Original imae
    int             width,		// I - Target width
    int             length)		// I - Target length
{
  int	i;				// Looping var
  int	neww, newh,			// New width and height
	minw, maxw,			// Range of acceptable widths
	minh, maxh;			// Range of acceptable heights


  // Figure out the size of the image in the bounding box...
  if (img->w() > img->h() && length > width)
  {
    minh = width;
    minw = minh * img->w() / img->h();

    if (minw > length)
    {
      minw = length;
      minh = minw * img->h() / img->w();
    }
  }
  else
  {
    minw = width;
    minh = minw * img->h() / img->w();
    if (minh > length)
    {
      minh = length;
      minw = minh * img->w() / img->h();
    }
  }

  // Now figure out the optimal size of the bounding box...
  maxw = minw * 2;
  maxh = minh * 2;

#ifdef DEBUG
  printf("image = %dx%d, min = %dx%d, max = %dx%d\n", img->w(), img->h(),
         minw, minh, maxw, minh);
#endif // DEBUG

  for (i = 1; i < 20; i ++)
  {
    neww = img->w() / i;
    newh = img->h() / i;

    if (neww <= maxw && newh <= maxh)
      break;
  }

#ifdef DEBUG
  printf("new image = %dx%d\n", neww, newh);
#endif // DEBUG

  // Return the optimal image...
  return ((Fl_Shared_Image *)img->copy(neww, newh));
}


//
// 'write_ascii85()' - Print binary data as a series of base-85 numbers.
//

static void
write_ascii85(FILE  *fp,		// I - File to write to
              uchar *data,		// I - Data to print
	      int  length)		// I - Number of bytes to print
{
  unsigned	b;			// Binary data word
  unsigned char	c[5];			// ASCII85 encoded chars
  int		col;			// Current column


  col = 0;
  while (length > 3)
  {
    b = (((((data[0] << 8) | data[1]) << 8) | data[2]) << 8) | data[3];

    if (b == 0)
    {
      putc('z', fp);
      col ++;
    }
    else
    {
      c[4] = (b % 85) + '!';
      b /= 85;
      c[3] = (b % 85) + '!';
      b /= 85;
      c[2] = (b % 85) + '!';
      b /= 85;
      c[1] = (b % 85) + '!';
      b /= 85;
      c[0] = b + '!';

      fwrite(c, 5, 1, fp);
      col += 5;
    }

    data += 4;
    length -= 4;

    if (col >= 75)
    {
      putc('\n', fp);
      col = 0;
    }
  }

  if (length > 0)
  {
    memset(data + length, 0, 4 - length);
    b = (((((data[0] << 8) | data[1]) << 8) | data[2]) << 8) | data[3];

    c[4] = (b % 85) + '!';
    b /= 85;
    c[3] = (b % 85) + '!';
    b /= 85;
    c[2] = (b % 85) + '!';
    b /= 85;
    c[1] = (b % 85) + '!';
    b /= 85;
    c[0] = b + '!';

    fwrite(c, length + 1, 1, fp);
  }

  fputs("~>\n", fp);
}


//
// 'write_asciihex()' - Print binary data as a series of hex numbers.
//

static void
write_asciihex(FILE  *fp,		// I - File to write to
               uchar *data,		// I - Data to print
	       int  length)		// I - Number of bytes to print
{
  uchar			b;		// Current byte
  int			col;		// Current column
  static const char	*hex = "0123456789ABCDEF";
					// Hex digits...


  putc('<', fp);
  col = 0;
  while (length > 0)
  {
    b = *data >> 4;
    putc(hex[b], fp);
    putc(hex[*data & 15], fp);

    col += 2;
    length --;
    data ++;

    if (col >= 78)
    {
      putc('\n', fp);
      col = 0;
    }
  }

  fputs(">\n", fp);
}


//
// 'write_calendar()' - Write a calendar.
//

static void
write_calendar(FILE        *fp,		// I - File to write
               int         firstday,	// I - Start day
               int         month,	// I - Month (0-11)
	       int         year,	// I - Year
               int         left,	// I - Left edge
	       int         bottom,	// I - Bottom edge
	       int         right,	// I - Right edge
	       int         top,		// I - Top edge
               int         num_dates,	// I - Number of dates
	       fl_datefile *dates)	// I - Dates
{
  int			x, y;		// Current position
  int			width, height;	// Width and height of day
  int			wday,		// Day of the week
			day, days,	// Current day and number of days
			week, weeks;	// Current week and number of weeks
  struct tm		start_date;	// Start date
  time_t		start_time;	// Start time
  struct tm		end_date;	// End date
  time_t		end_time;	// End time
  fl_datefile		*date;		// Current date
  static const char	*months[] =	// Month names
			{
			  _("January"),
			  _("February"),
			  _("March"),
			  _("April"),
			  _("May"),
			  _("June"),
			  _("July"),
			  _("August"),
			  _("September"),
			  _("October"),
			  _("November"),
			  _("December")
			};
  static const char	*wdays[] =	// Day names
			{
			  _("Sunday"),
			  _("Monday"),
			  _("Tuesday"),
			  _("Wednesday"),
			  _("Thursday"),
			  _("Friday"),
			  _("Saturday")
			};


  memset(&start_date, 0, sizeof(start_date));
  start_date.tm_mon  = month;
  start_date.tm_mday = 1;
  start_date.tm_year = year - 1900;
  start_date.tm_hour = 12;
  start_time         = mktime(&start_date);

  memset(&end_date, 0, sizeof(end_date));

  if (month < 11)
  {
    end_date.tm_mon  = month + 1;
    end_date.tm_year = year - 1900;
  }
  else
    end_date.tm_year = year - 1900 + 1;

  end_date.tm_mday = 1;
  end_date.tm_hour = 12;
  end_time         = mktime(&end_date);

  days   = (end_time - start_time) / 86400;
  wday   = (start_date.tm_wday + firstday) % 7;
  weeks  = (wday + days + 6) / 7;
  width  = right - left;
  height = top - bottom - 45 - 18;

  fprintf(fp, "(%s %d) %d %d T\n", months[month], year,
          left + width / 2, top - 36);

  for (day = 0; day <= 7; day ++)
  {
    x = left + day * width / 7;
    fprintf(fp, "%d %d MO %d %d LI\n", x, bottom, x, bottom + height + 18);

    if (day < 7)
    {
      x = left + (2 * day + 1) * width / 14;
      fprintf(fp, "(%s) %d %d L\n", wdays[(14 + day - firstday) % 7],
              x, bottom + height + 5);
    }
  }

  for (week = 0; week <= weeks; week ++)
  {
    y = bottom + week * height / weeks;
    fprintf(fp, "%d %d MO %d %d LI\n", left, y, right, y);
  }

  y = bottom + height + 18;
  fprintf(fp, "%d %d MO %d %d LI\n", left, y, right, y);

  for (day = 1, week = 0; day <= days; day ++)
  {
    // Show the day of the month...
    x = left + (wday + 1) * width / 7 - 4;
    y = bottom + height - week * height / weeks - 20;

    fprintf(fp, "(%d) %d %d R\n", day, x, y);

    // Show the phase of the moon...
    x = left + wday * width / 7 + 14;
    y += 6;

    switch (get_moon_phase(year, month + 1, day))
    {
      case MOON_NEW :
          fprintf(fp, "BK newpath %d %d 7 0 360 arc fill\n", x, y);
          fprintf(fp, "newpath %d %d 7 0 360 arc stroke\n", x, y);
	  write_ps_string(fp, _("New Moon"));
	  fprintf(fp, " %d %d CP\n", x + 9, y - 1);
          break;
      case MOON_FIRSTQ :
          fprintf(fp, "BK newpath %d %d 7 90 270 arc fill\n", x, y);
          fprintf(fp, "newpath %d %d 7 0 360 arc stroke\n", x, y);
	  write_ps_string(fp, _("First Quarter"));
	  fprintf(fp, " %d %d CP\n", x + 9, y - 1);
          break;
      case MOON_FULL :
          fprintf(fp, "BK newpath %d %d 7 0 360 arc stroke\n", x, y);
	  write_ps_string(fp, _("Full Moon"));
	  fprintf(fp, " %d %d CP\n", x + 9, y - 1);
          break;
      case MOON_LASTQ :
          fprintf(fp, "BK newpath %d %d 7 270 450 arc fill\n", x, y);
          fprintf(fp, "newpath %d %d 7 0 360 arc stroke\n", x, y);
	  write_ps_string(fp, _("Last Quarter"));
	  fprintf(fp, " %d %d CP\n", x + 9, y - 1);
          break;
      case MOON_TRANSITION :
          break;
    }

    // Show comments for this date...
    int first = -1;

    y = bottom + height - (week + 1) * height / weeks - 6;

    while ((date = fl_datefile::lookup(year, month + 1, day, num_dates, dates,
                                       first)) != NULL)
    {
      char	temp[1024],		// Message string
		*start,			// Start of current line
		*end;			// End of current line
      int	lines;			// Number of lines


      strlcpy(temp, date->message, sizeof(temp));
      for (lines = 1, start = temp;
           (end = strchr(start, '/')) != NULL;
	   start = end + 1, lines ++);

      x = left + wday * width / 7 + 4;
      y += 7 * lines + 3;

      for (start = temp; start; start = end, y -= 7)
      {
        if ((end = strchr(start, '/')) != NULL)
	  *end++ = '\0';

        fprintf(fp, "(%s) %d %d CP\n", start, x, y);
      }

      y += 7 * lines + 3;
    }

    wday ++;
    if (wday > 6)
    {
      wday = 0;
      week ++;
    }
  }
}


//
// 'write_end_page()' - Show the current page.
//

static void
write_end_page(FILE *fp)		// I - File to write to
{
  fputs("grestore\n", fp);
  fputs("showpage\n", fp);
  fputs("%%PageTrailer\n", fp);
}


//
// 'write_image()' - Write an image at the specified location, mode, and
//                   quality.
//

static float				// O - Bottom of image on page
write_image(FILE            *fp,	// I - File to write to
            Fl_Shared_Image *img,	// I - Image to write
            int             mode,	// I - Grayscale or color
	    int             quality,	// I - Output quality
	    int             left,	// I - Left position
	    int             bottom,	// I - Bottom position
	    int             right,	// I - Right position
	    int             top,	// I - Top position
	    int             autorotate)	// I - Automatically rotate image?
{
  float			x, y, w, h;	// Bounding box
  int			rotate;		// Rotate?
  Fl_RGB_Image		*gray;		// Grayscale 


  if (img->d() == 1)
    mode = Fl_Print_Dialog::MODE_GRAYSCALE;

  rotate = 0;
  if (autorotate &&
      ((img->w() > img->h() && (top - bottom) > (right - left)) ||
       (img->w() < img->h() && (right - left) > (top - bottom))))
    rotate = 1;

#ifdef DEBUG
  printf("write_image(fp=%p, img=%p, mode=%d, quality=%d, left=%d,\n"
         "            bottom=%d, right=%d, top=%d, autorotate=%d)\n",
         fp, img, mode, quality, left, bottom, right, top, autorotate);
#endif // DEBUG

  if (rotate)
  {
    w = right - left;
    h = w * img->w() / img->h();

    if (h > (top - bottom))
    {
      h = top - bottom;
      w = h * img->h() / img->w();
    }
  }
  else
  {
    w = right - left;
    h = w * img->h() / img->w();
    if (h > (top - bottom))
    {
      h = top - bottom;
      w = h * img->w() / img->h();
    }
  }

#ifdef DEBUG
  printf("    rotate=%d, w=%.1f, h=%.1f\n", rotate, w, h);
#endif // DEBUG

  x = left + (right - left - w) * 0.5;
  y = bottom + (top - bottom - h) * 0.5;

  fputs("gsave\n", fp);

  if (rotate)
  {
    fprintf(fp, "%.1f %.1f translate\n", x, y);
    fprintf(fp, "%.3f %.3f scale\n", w / img->h(), h / img->w());
  }
  else
  {
    fprintf(fp, "%.1f %.1f translate\n", x, y + h);
    fprintf(fp, "%.3f %.3f scale\n", w / img->w(), h / img->h());
  }

  fprintf(fp, "<<"
              "/ImageType 1"
	      "/Width %d"
	      "/Height %d"
	      "/BitsPerComponent 8",
	  img->w(), img->h());

  if (mode == Fl_Print_Dialog::MODE_GRAYSCALE)
    fputs("/Decode[0 1]", fp);
  else
    fputs("/Decode[0 1 0 1 0 1]", fp);

  fputs("/DataSource currentfile /ASCII85Decode filter", fp);

  fputs("/Interpolate true", fp);

  if (rotate)
    fputs("/ImageMatrix[0 1 1 0 0 0]>>image\n", fp);
  else
    fputs("/ImageMatrix[1 0 0 -1 0 1]>>image\n", fp);

  if (mode == Fl_Print_Dialog::MODE_GRAYSCALE && img->d() == 3)
  {
    // Convert image to grayscale...
    gray = new Fl_RGB_Image((uchar *)img->data()[0], img->w(), img->h(),
                            img->d());
    gray->desaturate();

    write_ascii85(fp, (uchar *)gray->data()[0], img->w() * img->h());

    delete gray;
  }
  else
    write_ascii85(fp, (uchar *)img->data()[0], img->w() * img->h() * img->d());

  fputs("grestore\n", fp);

  return (y);
}


//
// 'write_matting()' - Write the matting around an image...
//

static void
write_matting(FILE  *fp,		// I - File to write
              uchar *data,		// I - 64x64 matting pattern
              int   mode,		// I - Color/grayscale?
	      int   type,		// I - Mat type
	      int   width,		// I - Mat width in points
              int   left,		// I - Left position in points
	      int   bottom,		// I - Bottom position in points
	      int   right,		// I - Right position in points
	      int   top)		// I - Top position in points
{
  int	x, y;				// Looping vars
  int	matsize,			// Mat size
	matscale;			// Mat scale
  uchar	zdata[128 * 128 * 3],		// Zoomed mat image
	*ptr,				// Pointer into mat image
	*zptr;				// Pointer into zoomed image


  if (width == 0 ||
      type == Fl_Print_Dialog::MAT_BLANK ||
      (data[0] == 255 && memcmp(data, data + 1, 64 * 64 * 3 - 1) == 0))
    return;				// Ignore blank or 0-width matting

  fputs("gsave\n", fp);
  fprintf(fp, "[ %d %d %d %d\n", left, bottom, width, top - bottom);
  fprintf(fp, "  %d %d %d %d\n", left, bottom, right - left, width);
  fprintf(fp, "  %d %d %d %d\n", right - width, bottom, width, top - bottom);
  fprintf(fp, "  %d %d %d %d ] rectclip\n", left, top - width, right - left, width);

  if (type >= Fl_Print_Dialog::MAT_STANDARD)
  {
    // Write a mat image...
    fputs("/matdata\n", fp);
    if (type == 2)
    {
      // Scale the mat image for velvet...
      matsize  = 128;
      matscale = 144;

      if (mode == Fl_Print_Dialog::MODE_COLOR)
      {
        // Print color matting...
	for (y = 64, ptr = data, zptr = zdata; y > 0; y --, zptr += 128 * 3)
	{
          for (x = 64; x > 0; x --, ptr += 3, zptr += 6)
	  {
	    zptr[0] = ptr[0];
	    zptr[1] = ptr[1];
	    zptr[2] = ptr[2];

            if (y == 1)
	    {
	      zptr[128 * 3 + 0] = (ptr[0] + ptr[-63 * 64 * 3 + 0]) / 2;
	      zptr[128 * 3 + 1] = (ptr[1] + ptr[-63 * 64 * 3 + 1]) / 2;
	      zptr[128 * 3 + 2] = (ptr[2] + ptr[-63 * 64 * 3 + 2]) / 2;
	    }
	    else
	    {
	      zptr[128 * 3 + 0] = (ptr[0] + ptr[64 * 3 + 0]) / 2;
	      zptr[128 * 3 + 1] = (ptr[1] + ptr[64 * 3 + 1]) / 2;
	      zptr[128 * 3 + 2] = (ptr[2] + ptr[64 * 3 + 2]) / 2;
	    }

            if (x == 1)
	    {
	      zptr[3] = (ptr[0] + ptr[-63 * 3 + 0]) / 2;
	      zptr[4] = (ptr[1] + ptr[-63 * 3 + 1]) / 2;
	      zptr[5] = (ptr[2] + ptr[-63 * 3 + 2]) / 2;

              if (y == 1)
	      {
		zptr[128 * 3 + 3] = (ptr[0] + ptr[-63 * 3 + 0] +
	                             ptr[-63 * 64 * 3 + 0] +
				     ptr[-63 * 64 * 3 - 63 * 3 + 0]) / 4;
		zptr[128 * 3 + 4] = (ptr[1] + ptr[-63 * 3 + 1] +
	                             ptr[-63 * 64 * 3 + 1] +
				     ptr[-63 * 64 * 3 - 63 * 3 + 1]) / 4;
		zptr[128 * 3 + 5] = (ptr[2] + ptr[-63 * 3 + 2] +
	                             ptr[-63 * 64 * 3 + 2] +
				     ptr[-63 * 64 * 3 - 63 * 3 + 2]) / 4;
	      }
	      else
	      {
		zptr[128 * 3 + 3] = (ptr[0] + ptr[-63 * 3 + 0] +
	                             ptr[64 * 3 + 0] +
				     ptr[64 * 3 - 63 * 3 + 0]) / 4;
		zptr[128 * 3 + 4] = (ptr[1] + ptr[-63 * 3 + 1] +
	                             ptr[64 * 3 + 1] +
				     ptr[64 * 3 - 63 * 3 + 1]) / 4;
		zptr[128 * 3 + 5] = (ptr[2] + ptr[-63 * 3 + 2] +
	                             ptr[64 * 3 + 2] +
				     ptr[64 * 3 - 63 * 3 + 2]) / 4;
	      }
	    }
	    else
	    {
	      zptr[3] = (ptr[0] + ptr[3]) / 2;
	      zptr[4] = (ptr[1] + ptr[4]) / 2;
	      zptr[5] = (ptr[2] + ptr[5]) / 2;

              if (y == 1)
	      {
		zptr[128 * 3 + 3] = (ptr[0] + ptr[3] +
	                             ptr[-63 * 64 * 3 + 0] +
				     ptr[-63 * 64 * 3 + 3]) / 4;
		zptr[128 * 3 + 4] = (ptr[1] + ptr[4] +
	                             ptr[-63 * 64 * 3 + 1] +
				     ptr[-63 * 64 * 3 + 4]) / 4;
		zptr[128 * 3 + 5] = (ptr[2] + ptr[5] +
	                             ptr[-63 * 64 * 3 + 2] +
				     ptr[-63 * 64 * 3 + 5]) / 4;
	      }
	      else
	      {
		zptr[128 * 3 + 3] = (ptr[0] + ptr[3] +
	                             ptr[64 * 3 + 0] +
				     ptr[64 * 3 + 3]) / 4;
		zptr[128 * 3 + 4] = (ptr[1] + ptr[4] +
	                             ptr[64 * 3 + 1] +
				     ptr[64 * 3 + 4]) / 4;
		zptr[128 * 3 + 5] = (ptr[2] + ptr[5] +
	                             ptr[64 * 3 + 2] +
				     ptr[64 * 3 + 5]) / 4;
	      }
	    }
	  }
	}

        write_asciihex(fp, zdata, 128 * 128 * 3);
      }
      else
      {
        // Print grayscale matting...
	for (y = 64, ptr = data, zptr = zdata; y > 0; y --, zptr += 128)
	{
          for (x = 64; x > 0; x --, ptr += 3, zptr += 2)
	  {
	    zptr[0] = (31 * ptr[0] + 61 * ptr[1] + 8 * ptr[2]) / 100;

            if (y == 1)
	      zptr[128] = (31 * (ptr[0] + ptr[-63 * 64 * 3 + 0]) +
	                   61 * (ptr[1] + ptr[-63 * 64 * 3 + 1]) +
			   8 * (ptr[2] + ptr[-63 * 64 * 3 + 2])) / 200;
	    else
	      zptr[128] = (31 * (ptr[0] + ptr[64 * 3 + 0]) +
	                   61 * (ptr[1] + ptr[64 * 3 + 1]) +
	                   8 * (ptr[2] + ptr[64 * 3 + 2])) / 200;

            if (x == 1)
	    {
	      zptr[1] = (31 * (ptr[0] + ptr[-63 * 3 + 0]) +
	                 61 * (ptr[1] + ptr[-63 * 3 + 1]) +
			 8 * (ptr[2] + ptr[-63 * 3 + 2])) / 200;

              if (y == 1)
	        zptr[129] = (31 * (ptr[0] + ptr[-63 * 3 + 0] +
	                           ptr[-63 * 64 * 3 + 0] +
				   ptr[-63 * 64 * 3 - 63 * 3 + 0]) +
			     61 * (ptr[1] + ptr[-63 * 3 + 1] +
	                           ptr[-63 * 64 * 3 + 1] +
				   ptr[-63 * 64 * 3 - 63 * 3 + 1]) +
			     8 * (ptr[2] + ptr[-63 * 3 + 2] +
	                          ptr[-63 * 64 * 3 + 2] +
				  ptr[-63 * 64 * 3 - 63 * 3 + 2])) / 400;
	      else
		zptr[129] = (31 * (ptr[0] + ptr[-63 * 3 + 0] +
	                           ptr[64 * 3 + 0] +
				   ptr[64 * 3 - 63 * 3 + 0]) +
			     61 * (ptr[1] + ptr[-63 * 3 + 1] +
	                           ptr[64 * 3 + 1] +
				   ptr[64 * 3 - 63 * 3 + 1]) +
			     8 * (ptr[2] + ptr[-63 * 3 + 2] +
	                          ptr[64 * 3 + 2] +
				  ptr[64 * 3 - 63 * 3 + 2])) / 400;
	    }
	    else
	    {
	      zptr[1] = (31 * (ptr[0] + ptr[3]) +
	                 61 * (ptr[1] + ptr[4]) +
			 8 * (ptr[2] + ptr[5])) / 200;

              if (y == 1)
		zptr[129] = (31 * (ptr[0] + ptr[3] +
	                           ptr[-63 * 64 * 3 + 0] +
				   ptr[-63 * 64 * 3 + 3]) +
			     61 * (ptr[1] + ptr[4] +
	                           ptr[-63 * 64 * 3 + 1] +
				   ptr[-63 * 64 * 3 + 4]) +
			     8 * (ptr[2] + ptr[5] +
	                          ptr[-63 * 64 * 3 + 2] +
				  ptr[-63 * 64 * 3 + 5])) / 400;
	      else
		zptr[129] = (31 * (ptr[0] + ptr[3] +
	                           ptr[64 * 3 + 0] +
				   ptr[64 * 3 + 3]) +
			     61 * (ptr[1] + ptr[4] +
	                           ptr[64 * 3 + 1] +
				   ptr[64 * 3 + 4]) +
			     8 * (ptr[2] + ptr[5] +
	                          ptr[64 * 3 + 2] +
				  ptr[64 * 3 + 5])) / 400;
	    }
	  }
	}

        write_asciihex(fp, zdata, 128 * 128);
      }
    }
    else if (mode == Fl_Print_Dialog::MODE_COLOR)
    {
      // Use the standard 64x64 image for others...
      matsize  = 64;
      matscale = 36;

      write_asciihex(fp, data, 64 * 64 * 3);
    }
    else
    {
      // Convert the standard 64x64 image to grayscale...
      matsize  = 64;
      matscale = 36;

      for (x = 64 * 64, ptr = data, zptr = zdata; x > 0; x --, ptr += 3)
        *zptr++ = (31 * ptr[0] + 61 * ptr[1] + 8 * ptr[2]) / 100;

      write_asciihex(fp, zdata, 64 * 64);
    }

    fputs("def\n", fp);

    for (y = bottom - (bottom % matscale); y < top; y += matscale)
      for (x = left - (left % matscale); x < right; x += matscale)
      {
	fputs("gsave\n", fp);
	fprintf(fp, "%d %d translate\n", x, y + matscale);
	fprintf(fp, "%.3f dup scale\n", (float)matscale / (float)matsize);
	fprintf(fp, "<</ImageType 1/Width %d/Height %d/BitsPerComponent 8",
	        matsize, matsize);
        if (mode == Fl_Print_Dialog::MODE_COLOR)
	  fputs("/Decode[0 1 0 1 0 1]", fp);
        else
	  fputs("/Decode[0 1]", fp);
	fputs("/DataSource matdata", fp);
	fputs("/Interpolate true/ImageMatrix[1 0 0 -1 0 1]>>image\n", fp);
	fputs("grestore\n", fp);
      }
  }
  else
  {
    // Draw a solid color mat...
    if (mode == Fl_Print_Dialog::MODE_COLOR)
      fprintf(fp, "%.3f %.3f %.3f setcolor\n",
              data[0] / 255.0, data[1] / 255.0, data[2] / 255.0);
    else
      fprintf(fp, "%.3f setcolor\n",
              (31 * data[0] + 61 * data[1] + 8 * data[2]) / 25500.0);

    fprintf(fp, "%d %d %d %d rectfill\n", left, bottom, right - left,
            top - bottom);
  }

  fputs("grestore\n", fp);

  // Draw the matting bezel...
  left   += width;
  bottom += width;
  right  -= width;
  top    -= width;

  fputs("0.7 G\n", fp);
  fprintf(fp, "%d %d moveto %d %d lineto %d %d lineto %d %d lineto closepath fill\n",
          left, bottom, left, top, left + 2, top - 2, left + 2, bottom + 2);
  fputs("0.5 G\n", fp);
  fprintf(fp, "%d %d moveto %d %d lineto %d %d lineto %d %d lineto closepath fill\n",
          left, top, right, top, right - 2, top - 2, left + 2, top - 2);
  fputs("0.8 G\n", fp);
  fprintf(fp, "%d %d moveto %d %d lineto %d %d lineto %d %d lineto closepath fill\n",
          right, bottom, right, top, right - 2, top - 2, right - 2, bottom + 2);
  fputs("1.0 G\n", fp);
  fprintf(fp, "%d %d moveto %d %d lineto %d %d lineto %d %d lineto closepath fill\n",
          left, bottom, right, bottom, right - 2, bottom + 2, left + 2,
	  bottom + 2);
}


//
// 'write_prolog()' - Write the PostScript prolog...
//

static void
write_prolog(FILE *fp,			// I - File to write to
             int  mode,			// I - Print mode (color or gray)
	     int  left,			// I - Left edge
	     int  bottom,		// I - Bottom edge
	     int  right,		// I - Right edge
	     int  top)			// I - Top edge
{
  // Document header...
  fputs("%!PS-Adobe-3.0\n", fp);
  fprintf(fp, "%%%%BoundingBox: %d %d %d %d\n", left, bottom, right, top);
  fputs("%%Creator: flPhoto " FLPHOTO_VERSION "\n", fp);
  fputs("%%LanguageLevel: 2\n", fp);
  fputs("%%DocumentData: Clean7Bit\n", fp);
  fputs("%%Pages: (atend)\n", fp);
  fputs("%%EndComments\n", fp);

  // Font encoding (ISO-8859-1 + Euro)
  fputs("%%BeginProlog\n", fp);
  fputs("/fontencoding[\n", fp);
  fputs("/.notdef/.notdef/.notdef/.notdef/.notdef/.notdef/.notdef/.notdef/.notdef/.notdef\n", fp);
  fputs("/.notdef/.notdef/.notdef/.notdef/.notdef/.notdef/.notdef/.notdef/.notdef/.notdef\n", fp);
  fputs("/.notdef/.notdef/.notdef/.notdef/.notdef/.notdef/.notdef/.notdef/.notdef/.notdef\n", fp);
  fputs("/.notdef/.notdef/space/exclam/quotedbl/numbersign/dollar/percent/ampersand\n", fp);
  fputs("/quotesingle/parenleft/parenright/asterisk/plus/comma/minus/period/slash/zero\n", fp);
  fputs("/one/two/three/four/five/six/seven/eight/nine/colon/semicolon/less/equal/greater\n", fp);
  fputs("/question/at/A/B/C/D/E/F/G/H/I/J/K/L/M/N/O/P/Q/R/S/T/U/V/W/X/Y/Z/bracketleft\n", fp);
  fputs("/backslash/bracketright/asciicircum/underscore/grave/a/b/c/d/e/f/g/h/i/j/k/l/m/n\n", fp);
  fputs("/o/p/q/r/s/t/u/v/w/x/y/z/braceleft/bar/braceright/asciitilde/.notdef/Euro\n", fp);
  fputs("/.notdef/quotesinglbase/florin/quotedblbase/ellipsis/dagger/daggerdbl/circumflex\n", fp);
  fputs("/perthousand/Scaron/guilsinglleft/OE/.notdef/.notdef/.notdef/.notdef/quoteleft\n", fp);
  fputs("/quoteright/quotedblleft/quotedblright/bullet/endash/emdash/tilde/trademark\n", fp);
  fputs("/scaron/guilsinglright/oe/.notdef/.notdef/Ydieresis/space/exclamdown/cent\n", fp);
  fputs("/sterling/currency/yen/brokenbar/section/dieresis/copyright/ordfeminine\n", fp);
  fputs("/guillemotleft/logicalnot/hyphen/registered/macron/degree/plusminus/twosuperior\n", fp);
  fputs("/threesuperior/acute/mu/paragraph/periodcentered/cedilla/onesuperior\n", fp);
  fputs("/ordmasculine/guillemotright/onequarter/onehalf/threequarters/questiondown\n", fp);
  fputs("/Agrave/Aacute/Acircumflex/Atilde/Adieresis/Aring/AE/Ccedilla/Egrave/Eacute\n", fp);
  fputs("/Ecircumflex/Edieresis/Igrave/Iacute/Icircumflex/Idieresis/Eth/Ntilde/Ograve\n", fp);
  fputs("/Oacute/Ocircumflex/Otilde/Odieresis/multiply/Oslash/Ugrave/Uacute/Ucircumflex\n", fp);
  fputs("/Udieresis/Yacute/Thorn/germandbls/agrave/aacute/acircumflex/atilde/adieresis\n", fp);
  fputs("/aring/ae/ccedilla/egrave/eacute/ecircumflex/edieresis/igrave/iacute/icircumflex\n", fp);
  fputs("/idieresis/eth/ntilde/ograve/oacute/ocircumflex/otilde/odieresis/divide/oslash\n", fp);
  fputs("/ugrave/uacute/ucircumflex/udieresis/yacute/thorn/ydieresis]def\n", fp);

  // sRGB colorspace definitions...
  fputs("\n", fp);
  if (mode == Fl_Print_Dialog::MODE_COLOR)
  {
    fputs("% Set color printing based on sRGB\n", fp);
    fputs("/colorspace [ /CIEBasedABC <<\n", fp);
  }
  else
  {
    fputs("% Set grayscale printing based on sRGB\n", fp);
    fputs("/colorspace [ /CIEBasedA <<\n", fp);
  }

  fputs("  /DecodeLMN [\n", fp);
  fputs("    { dup 0.03928 le\n", fp);
  fputs("        {12.92321 div}\n", fp);
  fputs("        {0.055 add 1.055 div 2.4 exp}\n", fp);
  fputs("      ifelse\n", fp);
  fputs("    } bind dup dup\n", fp);
  fputs("  ]\n", fp);
  fputs("  /MatrixLMN [0.4123907993 0.2126390059 0.0193308187\n", fp);
  fputs("              0.3575843394 0.7151686788 0.1191947798\n", fp);
  fputs("              0.1804807884 0.0721923154 0.9505321522]\n", fp);
  fputs("  /WhitePoint [0.9504559271 1.0 1.0890577508]\n", fp);
  fputs(">> ] def\n", fp);

  // Procedures...
  if (mode == Fl_Print_Dialog::MODE_COLOR)
  {
    fputs("/BK { 0 0 0 setcolor } bind def\n", fp);
    fputs("/G { dup dup setcolor } bind def\n", fp);
  }
  else
  {
    fputs("/BK { 0 setcolor } bind def\n", fp);
    fputs("/G { setcolor } bind def\n", fp);
  }

  fputs("/DF { findfont dup length dict begin { 1 index /FID ne \n"
        "{ def } { pop pop } ifelse } forall /Encoding fontencoding def\n"
	"currentdict end definefont pop } bind def\n", fp);
  fputs("/HV /Helvetica DF\n", fp);
  fputs("/LF /HV findfont 9 scalefont def\n", fp);
  fputs("/L { moveto LF setfont BK dup stringwidth pop -0.5 mul 0 "
        "rmoveto show } bind def\n", fp);
  fputs("/RF /HV findfont 18 scalefont def\n", fp);
  fputs("/R { moveto RF setfont BK dup stringwidth pop neg 0 "
        "rmoveto show } bind def\n", fp);
  fputs("/TF /HV findfont 36 scalefont def\n", fp);
  fputs("/T { moveto TF setfont BK dup stringwidth pop -0.5 mul 0 "
        "rmoveto show } bind def\n", fp);
  fputs("/LI { lineto stroke } bind def\n", fp);
  fputs("/MO { moveto } bind def\n", fp);
  fputs("/P { moveto LF setfont show } bind def\n", fp);
  fputs("/CF /HV findfont 6 scalefont def\n", fp);
  fputs("/CP { moveto CF setfont show } bind def\n", fp);

  fputs("/ML {\n"
        "  gsave translate LF setfont\n"
	"\n"
	"  dup stringwidth pop 18 add /_mlw exch def\n"
	"    /_mlx _mlw -2 div def\n"
	"    /_mly -13.5 def\n"
	"    /_mlh 27 def\n"
	"\n"
	"  _mlx _mly _mlw _mlh 0.95 G rectfill\n"
	"\n"
	"  BK _mlx 9 add -3.5 moveto show\n"
	"\n"
	"  0.7 G\n"
	"  _mlx _mly moveto 0 _mlh rlineto 2 -2 rlineto 0 4 _mlh sub rlineto\n"
	"    closepath fill\n"
	"\n"
	"  0.5 G\n"
	"  _mlx _mly _mlh add moveto _mlw 0 rlineto -2 -2 rlineto\n"
	"    4 _mlw sub 0 rlineto closepath fill\n"
	"\n"
	"  0.8 G\n"
	"  _mlx _mlw add _mly moveto 0 _mlh rlineto -2 -2 rlineto\n"
	"    0 4 _mlh sub rlineto closepath fill\n"
	"\n"
	"  1.0 G\n"
	"  _mlx _mly moveto _mlw 0 rlineto -2 +2 rlineto\n"
	"    4 _mlw sub 0 rlineto closepath fill\n"
	"\n"
	"  grestore\n"
	"} bind def\n", fp);

  fputs("%%EndProlog\n", fp);
}


//
// 'write_ps_string()' - Write a PostScript string...
//

static void
write_ps_string(FILE       *fp,		// I - File to write
                const char *s)		// I - String
{
  putc('(', fp);

  while (*s)
  {
    if ((*s & 255) < ' ')
      break;

    if (*s & 128 || *s == '(' || *s == ')' || *s == '\\')
      fprintf(fp, "\\%03o", *s & 255);
    else
      putc(*s, fp);

    s ++;
  }

  putc(')', fp);
}


//
// 'write_start_page()' - Start a page.
//

static void
write_start_page(FILE *fp,		// I - File to write
                 int  page,		// I - Page to write
		 int  width,		// I - Page width in points
                 int  length,		// I - Page length in points
		 int  commands)		// I - Embed page commands?
{
  fprintf(fp, "%%%%Page: %d %d\n", page, page);

  fputs("%%BeginPageSetup\n", fp);

  if (commands)
  {
    fputs("[{\n", fp);

    if (width == 612 && length == 792)
      fputs("%%BeginFeature: *PageSize Letter\n", fp);
    else if (width == 595 && length == 842)
      fputs("%%BeginFeature: *PageSize A4\n", fp);
    else
      fprintf(fp, "%%%%BeginFeature: *PageSize w%dl%d\n", width, length);

    fprintf(fp, "<</PageSize[%d %d]/ImagingBBox null>>setpagedevice\n",
            width, length);

    fputs("%%EndFeature\n", fp);

    fputs("} stopped cleartomark\n", fp);
  }

  fputs("colorspace setcolorspace\n", fp);
  fputs("%%EndPageSetup\n", fp);
  fputs("gsave\n", fp);
}


//
// 'write_trailer()' - Write the PostScript trailer...
//

static void
write_trailer(FILE *fp,			// I - File to write
              int  pages)		// I - Number of pages
{
  fputs("%%Trailer\n", fp);
  fprintf(fp, "%%%%Pages: %d\n", pages);
  fputs("%%EOF\n", fp);
}


//
// 'fl_datefile::load()' - Load datefile.
//

fl_datefile *				// O - Dates
fl_datefile::load(const char *filename,	// I - Date file
                  int        *num_dates)// O - Number of dates
{
  FILE		*fp;			// File pointer
  char		line[1024],		// Line from file
		*repeat,		// Pointer to repeat state
		*message;		// Pointer to message
  int		year,			// Year
		month,			// Month
		day,			// Day
		linenum,		// Line number
		alloc_dates;		// Allocated dates
  fl_datefile	*dates,			// Dates
		*temp;			// Pointer to date


  *num_dates = 0;
  dates      = NULL;

  if ((fp = fopen(filename, "r")) == NULL)
    return (NULL);

  linenum     = 0;
  alloc_dates = 0;

  while (fgets(line, sizeof(line), fp))
  {
    // Format of lines is:
    //
    // YYYY-MM-DD -tab- ONCE   -tab- message
    // YYYY-MM-DD -tab- REPEAT -tab- message
    linenum ++;

    if (line[0])
      line[strlen(line) - 1] = '\0';

    if (!line[0])
      continue;

    year = month = day = 0;

    if (sscanf(line, "%d-%d-%d", &year, &month, &day) != 3)
    {
      fl_alert(_("Date file format error on line %d!"), linenum);
      break;
    }

    if ((repeat = strchr(line, '\t')) != NULL)
      *repeat++ = '\0';
    else
    {
      fl_alert(_("Date file format error on line %d!"), linenum);
      break;
    }

    if ((message = strchr(repeat, '\t')) != NULL)
      *message++ = '\0';
    else
    {
      fl_alert(_("Date file format error on line %d!"), linenum);
      break;
    }

    if (*num_dates >= alloc_dates)
    {
      alloc_dates += 10;
      temp = new fl_datefile[alloc_dates];

      if (*num_dates > 0)
      {
        memcpy(temp, dates, *num_dates * sizeof(fl_datefile));
	delete[] dates;
      }

      dates = temp;
    }

    temp = dates + *num_dates;
    (*num_dates) ++;

    temp->year    = year;
    temp->month   = month;
    temp->day     = day;
    temp->repeat  = !strcasecmp(repeat, "REPEAT");
    temp->message = strdup(message);
  }

  fclose(fp);

  return (dates);
}


//
// 'fl_datefile::lookup()' - Lookup date.
//

fl_datefile *				// O - Date
fl_datefile::lookup(
    int         y,			// I - Year
    int         m,			// I - Month
    int         d,			// I - Day
    int         num_dates,		// I - Number of dates
    fl_datefile *dates,			// I - Dates
    int         &first)			// I - First date
{
  int		i;			// Looping var
  fl_datefile	*temp;			// Pointer to current date


  for (i = first + 1, temp = dates + i; i < num_dates; i ++, temp ++)
    if ((y == temp->year || temp->repeat) &&
        m == temp->month && d == temp->day)
    {
      first = i;
      return (temp);
    }

  first = -1;

  return (NULL);
}


//
// End of "$Id: print.cxx 441 2006-12-21 16:46:09Z mike $".
//
