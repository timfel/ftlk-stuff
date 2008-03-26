//
// "$Id: export.cxx 414 2006-11-18 03:24:41Z mike $"
//
// HTML export methods for flPhoto.
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
//   flphoto::export_html_cb()    - Export the current album.
//   flphoto::export_html_ok_cb() - Start HTML export.
//   export_copy()                - Copy a file verbatim.
//   export_footer()              - Write a HTML footer.
//   export_header()              - Write a HTML header.
//   export_jpeg()                - Write an image file in JPEG format.
//   export_string()              - Write a string to a HTML file.
//   export_dir()                 - Create the remote directory.
//   export_file()                - Copy a local file to a remote file.
//

#include "flphoto.h"
#include "i18n.h"
#include "flstring.h"
#include "http.h"
#include <stdlib.h>
#include <sys/stat.h>
#include <FL/fl_draw.H>
#include <FL/x.H>

#if defined(WIN32) && !defined(__CYGWIN__)
#  include <direct.h>
#  include <io.h>
#  define fl_mkdir(p)	mkdir(p)
#else
#  include <unistd.h>
#  define fl_mkdir(p)	mkdir(p, 0777)
#endif // WIN32 && !__CYGWIN__

#include <errno.h>

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



//
// Local functions...
//

int			export_copy(const char *src, const char *dst);
#ifdef USE_MKCOL
static HTTPStatus	export_dir(HTTP *http, const char *username,
			           const char *password, const char *url);
#endif // USE_MKCOL
static HTTPStatus	export_file(HTTP *http, const char *username,
			            const char *password, const char *filename,
				    const char *url);
static void		export_footer(FILE *fp, const char *footer);
static FILE		*export_header(const char *filename, const char *header,
			               const char *comment);
static int		export_jpeg(const char *filename, Fl_Shared_Image *img,
			            int w, int h, int quality,
                                    const char *watermark, int font,
				    int fontsize, int opacity, int pos);
static void		export_string(FILE *fp, const char *s,
			              bool breaks = false, const char *el = 0);


enum { // Watermark positions
  WM_POS_CENTER,
  WM_POS_TOP,
  WM_POS_BOTTOM,
  WM_POS_TOP_BOTTOM,
  WM_POS_LEFT,
  WM_POS_RIGHT,
  WM_POS_LEFT_RIGHT,
  WM_POS_ALL
};


//
// 'flphoto::export_html_cb()' - Export the current album as HTML.
//

void
flphoto::export_html_cb()
{
  int	val;				// Integer preference value
  char	s[1024];			// String preference value


  // Get the defaults and put them in the dialog...
  prefs.get("export_path", s, "", sizeof(s));
  export_path_field_->value(s);
  prefs.get("export_jpeg_size", val, 640);
  export_imagesize_value_->value(val);
  prefs.get("export_jpeg_columns", val, 1);
  export_imagecols_value_->value(val);
  prefs.get("export_jpeg_rows", val, 1);
  export_imagerows_value_->value(val);
  prefs.get("export_jpeg_quality", val, 75);
  export_imagequal_value_->value(val);
  prefs.get("export_thumb_size", val, 120);
  export_thumbsize_value_->value(val);
  prefs.get("export_thumb_columns", val, 5);
  export_thumbcols_value_->value(val);
  prefs.get("export_thumb_rows", val, 3);
  export_thumbrows_value_->value(val);
  prefs.get("export_thumb_quality", val, 50);
  export_thumbqual_value_->value(val);
  prefs.get("export_header", s, "", sizeof(s));
  export_header_field_->value(s);
  prefs.get("export_footer", s, "", sizeof(s));
  export_footer_field_->value(s);
  prefs.get("export_watermark", s, "", sizeof(s));
  export_watermark_field_->value(s);
  prefs.get("export_typeface", val, 0);
  export_typeface_chooser_->value(val);
  prefs.get("export_fontstyle", val, 0);
  export_fontstyle_chooser_->value(val);
  prefs.get("export_fontsize", val, 10);
  export_fontsize_slider_->value(val);
  prefs.get("export_opacity", val, 30);
  export_opacity_slider_->value(val);
  prefs.get("export_position", val, 0);
  export_position_chooser_->value(val);
  prefs.get("export_original", val, 1);
  export_original_button_->value(val);
  prefs.get("export_style", s, "", sizeof(s));
  export_style_field_->value(s);

  export_progress_->hide();
  export_group_->activate();

  export_tabs_->value(export_general_tab_);
  export_html_window_->hotspot(export_html_window_);
  export_html_window_->show();
}


//
// 'flphoto::export_html_ok_cb()' - Start HTML export.
//

void
flphoto::export_html_ok_cb()
{
  int			i, j;		// Looping vars
  int			delay;		// Delay between slides
  int			w, h;		// Resized width/height
  const char		*path,		// Export path
			*header,	// Header text
			*footer,	// Footer text
                        *watermark;	// Watermark text
  int			typeface,	// Watermark typeface
			fontstyle,	// Watermark style
			font,		// Watermark font
			fontsize,	// Watermark size
			opacity,	// Watermark opacity
			pos;		// Watermark position
  char			dstdir[1024],	// Destination path
			url[1024],	// Upload URL
			filename[1024],	// Output filename
			thumbname[1024],// Thumbname filename
			imagename[1024],// Image filename
			label[1024];	// Label text
  int			thumbsize,	// Thumbnail size
			thumbquality,	// Thumbnail quality
			thumbpage,	// Thumbnail page number
			thumbpages,	// Total thumbnail pages
			thumbcol,	// Thumbnail column
			thumbcols,	// Total thumbnail columns
			thumbrow,	// Thumbnail row
			thumbrows;	// Total thumbnail rows
  FILE			*thumbfile;	// Current thumbnail file
  int			imagesize,	// Image size
			imagequality,	// Image quality
			imagepage,	// Image page number
			imagepages,	// Total image pages
			imagecol,	// Image column
			imagecols,	// Total image columns
			imagerow,	// Image row
			imagerows;	// Total image rows
  int			originals;	// Include originals?
  FILE			*cssfile;	// Stylesheet file
  FILE			*imagefile;	// Image file
  FILE			*slidefile;	// Slideshow file
  Fl_Shared_Image	*img;		// Current image
  Fl_Image_Browser::ITEM *item;		// Current image data
  HTTP			*http;		// Connection to server
  HTTPStatus		status;		// Upload status


  // Check that the user has chosen an export path...
  path = export_path_field_->value();
  if (!path || !*path)
  {
    fl_alert(_("Please choose an export directory or URL."));
    return;
  }

  if (!strncmp(path, "http:", 5) ||
      !strncmp(path, "https:", 5) ||
      !strncmp(path, "ftp:", 4))
  {
    // Write the files to a temporary directory...
    prefs.getUserdataPath(dstdir, sizeof(dstdir));
  }
  else
  {
    // Write the files to the named directory...
    strlcpy(dstdir, path, sizeof(dstdir));
    if (access(path, 0))
    {
      if (fl_choice(_("Directory %s does not exist."), _("Cancel"),
                    _("Create Directory"), NULL, path))
      {
        if (fl_mkdir(path))
	{
	  fl_alert(_("Unable to create directory %s:\n\n%s"), path,
	           strerror(errno));
	  return;
	}
      }
      else
        return;
    }
  }

  // Save options as defaults...
  prefs.set("export_path", export_path_field_->value());
  prefs.set("export_jpeg_size",
            imagesize = (int)export_imagesize_value_->value());
  prefs.set("export_jpeg_columns",
            imagecols = (int)export_imagecols_value_->value());
  prefs.set("export_jpeg_rows",
            imagerows = (int)export_imagerows_value_->value());
  prefs.set("export_jpeg_quality",
            imagequality = (int)export_imagequal_value_->value());
  prefs.set("export_thumb_size",
            thumbsize = (int)export_thumbsize_value_->value());
  prefs.set("export_thumb_columns",
            thumbcols = (int)export_thumbcols_value_->value());
  prefs.set("export_thumb_rows",
            thumbrows = (int)export_thumbrows_value_->value());
  prefs.set("export_thumb_quality",
            thumbquality = (int)export_thumbqual_value_->value());
  prefs.set("export_header",
            header = export_header_field_->value());
  prefs.set("export_footer",
            footer = export_footer_field_->value());
  prefs.set("export_watermark",
            watermark = export_watermark_field_->value());
  prefs.set("export_typeface",
            typeface = export_typeface_chooser_->value());
  prefs.set("export_fontstyle",
            fontstyle = export_fontstyle_chooser_->value());
  prefs.set("export_fontsize",
            fontsize = (int)export_fontsize_slider_->value());
  prefs.set("export_opacity",
            opacity = (int)export_opacity_slider_->value());
  prefs.set("export_position",
            pos = export_position_chooser_->value());
  prefs.set("export_style", export_style_field_->value());
  prefs.set("export_original",
            originals = export_original_button_->value());

  if (!strncmp(path, "ftp:", 4))
  {
    fl_alert(_("Sorry, FTP export not currently supported."));
    export_html_window_->hide();
    return;
  }

  // Create necessary subdirs...
  snprintf(filename, sizeof(filename), "%s/images", dstdir);
  fl_mkdir(filename);

  snprintf(filename, sizeof(filename), "%s/thumbs", dstdir);
  fl_mkdir(filename);

  if (originals)
  {
    snprintf(filename, sizeof(filename), "%s/original", dstdir);
    fl_mkdir(filename);
  }

  // Start export...
  export_group_->deactivate();
  export_html_window_->cursor(FL_CURSOR_WAIT);
  export_progress_->show();

  thumbpage  = 0;
  thumbpages = (browser_->count() + thumbcols * thumbrows - 1) /
               (thumbcols * thumbrows);
  thumbcol   = 0;
  thumbrow   = 0;
  thumbfile  = NULL;

  imagepage  = 0;
  imagepages = (browser_->count() + imagecols * imagerows - 1) /
               (imagecols * imagerows);
  imagecol   = 0;
  imagerow   = 0;
  imagefile  = NULL;

  font       = typeface * 4 + fontstyle;

  prefs.get("slideshow_delay", delay, 5);

  // Copy or generate stylesheet file...
  snprintf(filename, sizeof(filename), "%s/style.css", dstdir);

  if (export_style_field_->value()[0])
    export_copy(export_style_field_->value(), filename);
  else if ((cssfile = fopen(filename, "w")) != NULL)
  {
    fputs("body.flphoto {\n", cssfile);
    fputs("  background: white;\n", cssfile);
    fputs("  color: black;\n", cssfile);
    fputs("  font: sans-serif;\n", cssfile);
    fputs("}\n", cssfile);

    fputs("body.flphoto_slideshow {\n", cssfile);
    fputs("  background: black;\n", cssfile);
    fputs("  color: white;\n", cssfile);
    fputs("  font: sans-serif;\n", cssfile);
    fputs("}\n", cssfile);

    fputs("a:link, a:visited, a:hover, a:hover:visited {\n", cssfile);
    fputs("  color: blue;\n", cssfile);
    fputs("  font-weight: bold;\n", cssfile);
    fputs("  text-decoration: none;\n", cssfile);
    fputs("}\n", cssfile);

    fputs("tr.flphoto_images img, tr.flphoto_thumbs img {\n", cssfile);
    fputs("  border: inset;\n", cssfile);
    fputs("}\n", cssfile);

    fputs("table.flphoto {\n", cssfile);
    fputs("  margin-left: auto;\n", cssfile);
    fputs("  margin-right: auto;\n", cssfile);
    fputs("}\n", cssfile);

    fputs("table.flphoto_slideshow {\n", cssfile);
    fputs("  height: 100%;\n", cssfile);
    fputs("  width: 100%;\n", cssfile);
    fputs("}\n", cssfile);

    fputs("tr.flphoto_navbar td {\n", cssfile);
    fputs("  background: #cccccc;\n", cssfile);
    fputs("  border: outset;\n", cssfile);
    fputs("  padding: 5px;\n", cssfile);
    fputs("  text-align: center;\n", cssfile);
    fputs("}\n", cssfile);

    fputs("tr.flphoto_images td {\n", cssfile);
    fputs("  padding: 5px;\n", cssfile);
    fputs("  text-align: center;\n", cssfile);
    fputs("}\n", cssfile);

    fputs("tr.flphoto_slides td {\n", cssfile);
    fputs("  text-align: center;\n", cssfile);
    fputs("  vertical-align: middle;\n", cssfile);
    fputs("}\n", cssfile);

    fputs("tr.flphoto_thumbs td {\n", cssfile);
    fputs("  padding: 5px;\n", cssfile);
    fputs("  text-align: center;\n", cssfile);
    fputs("}\n", cssfile);

    fclose(cssfile);
  }

  for (i = 0; i < browser_->count(); i ++)
  {
    // Load the current image...
    item = browser_->value(i);
    img  = browser_->load_item(i);

    if (!img)
      continue;

    // Update progress...
    snprintf(label, sizeof(label), _("Generating %s..."), item->label);
    export_progress_->label(label);
    export_progress_->value(100 * i / browser_->count());
    Fl::check();

    // Open files as needed..
    if (thumbcol == 0 && thumbrow == 0)
    {
      // Open the thumbnail index...
      export_footer(thumbfile, footer);

      if (thumbpage == 0)
        snprintf(thumbname, sizeof(thumbname), "%s/index.html", dstdir);
      else
        snprintf(thumbname, sizeof(thumbname), "%s/ind%05d.html", dstdir,
	         thumbpage);

      thumbfile = export_header(thumbname, header, album_comment_);

      fprintf(thumbfile, "<tr class='flphoto_navbar'><td colspan='%d'>"
                         "<a name='nav'>&nbsp;</a>",
              thumbcols);

      fputs("<a href='slideshow.html' target='_blank'>", thumbfile);
      export_string(thumbfile, _("Show Slideshow"));
      fputs("</a><br>\n", thumbfile);

      if (thumbpages > 1)
      {
	if (thumbpage > 1)
	  fprintf(thumbfile, "<a href='ind%05d.html#nav'>&lt;&lt;</a>",
	          thumbpage - 1);
	else if (thumbpage == 1)
	  fputs("<a href='index.html#nav'>&lt;&lt;</a>", thumbfile);

        for (j = 0; j < thumbpages; j ++)
	  if (j != thumbpage)
	  {
	    if (j == 0)
	      fputs(" <a href='index.html#nav'>1</a>", thumbfile);
            else
	      fprintf(thumbfile, " <a href='ind%05d.html#nav'>%d</a>", j,
	              j + 1);
	  }
	  else
	    fprintf(thumbfile, " <b>%d</b>", j + 1);

	if ((thumbpage + 1) < thumbpages)
	  fprintf(thumbfile, " <a href='ind%05d.html#nav'>&gt;&gt;</a>",
	          thumbpage + 1);
      }

      fputs("</td></tr>\n", thumbfile);

      thumbpage ++;
    }

    if (imagecol == 0 && imagerow == 0)
    {
      // Open the image index...
      export_footer(imagefile, footer);

      snprintf(imagename, sizeof(imagename), "%s/img%05d.html", dstdir,
               imagepage);

      imagefile = export_header(imagename, header, album_comment_);

      fprintf(imagefile, "<tr class='flphoto_navbar'><td colspan='%d'>"
			 "<a href='%s#nav'>%s</a></td></tr>\n",
	      imagecols, strrchr(thumbname, '/') + 1,
	      _("Back to Thumbnails"));

      if (imagepages > 1)
      {
        fprintf(imagefile, "<tr class='flphoto_navbar'><td colspan='%d'>",
	        imagecols);

	if (imagepage > 0)
	  fprintf(imagefile, "<a href='img%05d.html#nav'>&lt;&lt;</a>",
	          imagepage - 1);

        for (j = -10; j <= 10; j ++)
	  if ((imagepage + j) < 0 || (imagepage + j) >= imagepages)
	    continue;
	  else if (j != 0)
	    fprintf(imagefile, " <a href='img%05d.html#nav'>%d</a>",
	            imagepage + j, imagepage + j + 1);
	  else
	    fprintf(imagefile, " <b>%d</b>", imagepage + j + 1);

	if ((imagepage + 1) < imagepages)
	  fprintf(imagefile, " <a href='img%05d.html#nav'>&gt;&gt;</a>",
	          imagepage + 1);

        fputs("</td></tr>\n", imagefile);
      }

      imagepage ++;
    }

    // Write the thumbnail...
    w = thumbsize;
    h = w * img->h() / img->w();
    if (h > thumbsize)
    {
      h = thumbsize;
      w = h * img->w() / img->h();
    }

    if (thumbcol == 0)
      fputs("<tr class='flphoto_thumbs'>", thumbfile);

    fprintf(thumbfile, "<td><a href='%s#img%05d'>"
                       "<img src='thumbs/img%05d.jpg' width='%d' height='%d' "
		       "alt='",
            strrchr(imagename, '/') + 1, i, i, w, h);
    export_string(thumbfile, item->comments);
    fputs("' title='", thumbfile);
    export_string(thumbfile, item->comments);
    fputs("'><br>", thumbfile);
    export_string(thumbfile, item->label);
    fputs("</a></td>\n", thumbfile);

    thumbcol ++;
    if (thumbcol >= thumbcols)
    {
      fputs("</tr>\n", thumbfile);
      thumbcol = 0;
      thumbrow ++;
    }

    if (thumbrow >= thumbrows)
      thumbrow = 0;

    snprintf(filename, sizeof(filename), "%s/thumbs/img%05d.jpg", dstdir, i);
    export_jpeg(filename, item->image, w, h, thumbquality, watermark,
                font, fontsize, opacity, pos);

    // Write the image...
    w = imagesize;
    h = w * img->h() / img->w();
    if (h > imagesize)
    {
      h = imagesize;
      w = h * img->w() / img->h();
    }

    if (imagecol == 0)
      fputs("<tr class='flphoto_images'>", imagefile);

    fputs("<td>", imagefile);
    if (originals)
      fprintf(imagefile, "<a href='original/img%05d%s'>",
              i, fl_filename_ext(item->filename));
    fprintf(imagefile, "<img src='images/img%05d.jpg' "
                       "width='%d' height='%d' alt='",
            i, w, h);
    export_string(imagefile, item->comments);
    fputs("' title='", imagefile);
    export_string(imagefile, item->comments);
    fputs("'><br>", imagefile);
    export_string(imagefile, item->label, true);
    if (item->comments)
    {
      fputs("<br>", imagefile);
      export_string(imagefile, item->comments, true);
    }
    if (originals)
      fputs("</a>", imagefile);
    fputs("</td>\n", imagefile);

    imagecol ++;
    if (imagecol >= imagecols)
    {
      fputs("</tr>\n", imagefile);
      imagecol = 0;
      imagerow ++;
    }

    if (imagerow >= imagerows)
      imagerow = 0;

    snprintf(filename, sizeof(filename), "%s/images/img%05d.jpg", dstdir, i);
    export_jpeg(filename, item->image, w, h, imagequality, watermark,
                font, fontsize, opacity, pos);

    // Copy the image as needed...
    if (originals && strncmp(path, "http:", 5) && strncmp(path, "https:", 6))
    {
      snprintf(filename, sizeof(filename), "%s/original/img%05d%s", dstdir, i,
               fl_filename_ext(item->filename));
      export_copy(item->filename, filename);
    }

    // Unload the current image...
    if (item->image && !item->changed && item != image_item_)
    {
      item->image->release();
      item->image = 0;
    }
  }

  export_footer(imagefile, footer);
  export_footer(thumbfile, footer);

 /*
  * Write a slideshow file...
  */

  snprintf(filename, sizeof(filename), "%s/slideshow.html", dstdir);
  if ((slidefile = fopen(filename, "w")) != NULL)
  {
    fputs("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" "
          "\"http://www.w3.org/TR/html4/loose.dtd\">\n", slidefile);

    fputs("<html>\n", slidefile);
    fputs("<head>\n", slidefile);
    fputs("<meta http-equiv='Content-Type' content='text/html; "
          "charset=utf-8'>\n", slidefile);
    fputs("<link rel='StyleSheet' href='style.css' type='text/css'>\n",
          slidefile);
    if (header && *header)
      export_string(slidefile, header, false, "title");
    else
      export_string(slidefile, album_comment_, false, "title");
    fputs("<script type='text/javascript'><!--\n", slidefile);
    fputs("var imgnum=99999;\n", slidefile);
    fprintf(slidefile, "var imgdelay=%d;\n", delay * 1000);
    fputs("var imginterval=0;\n", slidefile);
    fputs("\n", slidefile);
    fputs("function next_image() {\n", slidefile);
    fputs("  var imgstr;\n", slidefile);
    fputs("\n", slidefile);
    fputs("  imgnum ++;\n", slidefile);
    fprintf(slidefile, "  if (imgnum > %d)\n", 99999 + browser_->count());
    fputs("    imgnum = 100000;\n", slidefile);
    fputs("\n", slidefile);
    fputs("  imgstr = imgnum.toString();\n", slidefile);
    fputs("\n", slidefile);
    fputs("  if (imginterval)\n", slidefile);
    fputs("    window.clearInterval(imginterval);\n", slidefile);
    fputs("  document.imgname.src = 'images/img' + imgstr.substring(1) + '.jpg';\n", slidefile);
    fputs("  imginterval = setInterval(\"next_image()\", imgdelay);\n", slidefile);
    fputs("}\n", slidefile);
    fputs("--></script>\n", slidefile);
    fputs("</head>\n", slidefile);
    fputs("<body class='flphoto_slideshow' onload='next_image()'>\n", slidefile);
    fputs("<table class='flphoto_slideshow' summary='Slideshow'>\n", slidefile);
    fprintf(slidefile, "<tr class='flphoto_slides'><td width='%d' height='%d'>"
                       "<img src='img00000.jpg' name='imgname' "
		       "onclick='next_image()' alt=''></td>\n",
            imagesize, imagesize);
    fputs("</tr></table>\n", slidefile);
    fputs("</body>\n", slidefile);
    fputs("</html>\n", slidefile);
    fclose(slidefile);
  }

  if (!strncmp(path, "http:", 5) ||
      !strncmp(path, "https:", 5))
  {
    // Connect to the remote system...
    char	scheme[HTTP_MAX_URI],	// URI scheme
		hostname[HTTP_MAX_URI],	// URI hostname
		username[HTTP_MAX_URI],	// URI username:password
		resource[HTTP_MAX_URI];	// URI resource path
    int		port;			// URI port number


    HTTP::separate(path, scheme, sizeof(scheme), username, sizeof(username),
                   hostname, sizeof(hostname), &port,
		   resource, sizeof(resource));

    if (resource[strlen(resource) - 1] != '/')
      strlcat(resource, "/", sizeof(resource));

    export_progress_->label(_("Connecting to remote server..."));
    export_progress_->value(0);
    Fl::check();

    if (!strcmp(scheme, "https"))
      http = new HTTP(hostname, port, HTTP_ENCRYPT_ALWAYS);
    else
      http = new HTTP(hostname, port);

    // Apparently MKCOL doesn't create a directory, so for now
    // leave this code commented out until we figure things out.
#ifdef USE_MKCOL
    // Create the destination folder...
    export_progress_->label(_("Creating destination folder..."));
    export_progress_->value(0);
    Fl::check();

    do
    {
      status = export_dir(http, auth_user_field_->value(),
                          auth_pass_field_->value(), resource);

      if (status == HTTP_UNAUTHORIZED)
      {
        // Show the authentication window...
	auth_window_->show();
	while (auth_window_->shown())
	  Fl::wait();

        if (!auth_user_field_->value()[0])
	  break;
      }
    }
    while (status == HTTP_UNAUTHORIZED);

    if (status != HTTP_CREATED &&
        status != HTTP_OK &&
	status != HTTP_METHOD_NOT_ALLOWED)
    {
      fl_alert(_("Unable to create destination URL:\n\n%d - %s"), status,
               HTTP::status_string(status));
      delete http;
      export_group_->activate();
      export_html_window_->cursor(FL_CURSOR_DEFAULT);
      export_progress_->hide();
      return;
    }
#endif // USE_MKCOL

    for (i = 0; i < browser_->count(); i ++)
    {
      // Update progress...
      snprintf(label, sizeof(label), _("Uploading images/img%05d.jpg..."), i);
      export_progress_->label(label);
      export_progress_->value(20 * i / browser_->count());
      Fl::check();

      snprintf(filename, sizeof(filename), "%s/images/img%05d.jpg", dstdir, i);
      snprintf(url, sizeof(url), "%simages/img%05d.jpg", resource, i);

      do
      {
	status = export_file(http, auth_user_field_->value(),
                             auth_pass_field_->value(), filename, url);

	if (status == HTTP_UNAUTHORIZED)
	{
          // Show the authentication window...
	  auth_window_->show();
	  while (auth_window_->shown())
	    Fl::wait();

          if (!auth_user_field_->value()[0])
	    break;
	}
      }
      while (status == HTTP_UNAUTHORIZED);

      unlink(filename);

      if (status != HTTP_CREATED &&
          status != HTTP_OK &&
	  status != HTTP_METHOD_NOT_ALLOWED)
      {
        fl_alert(_("Unable to create destination URL:\n\n%d - %s"), status,
                 HTTP::status_string(status));
        delete http;
        export_group_->activate();
        export_html_window_->cursor(FL_CURSOR_DEFAULT);
	export_progress_->hide();
        return;
      }
    }

    for (i = 0; i < browser_->count(); i ++)
    {
      // Update progress...
      snprintf(label, sizeof(label), _("Uploading thumbs/img%05d.jpg..."), i);
      export_progress_->label(label);
      export_progress_->value(20 + 20 * i / browser_->count());
      Fl::check();

      snprintf(filename, sizeof(filename), "%s/thumbs/img%05d.jpg", dstdir, i);
      snprintf(url, sizeof(url), "%sthumbs/img%05d.jpg", resource, i);

      status = export_file(http, auth_user_field_->value(),
                           auth_pass_field_->value(), filename, url);

      unlink(filename);

      if (status != HTTP_CREATED &&
          status != HTTP_OK &&
	  status != HTTP_METHOD_NOT_ALLOWED)
      {
        fl_alert(_("Unable to create destination URL:\n\n%d - %s"), status,
                 HTTP::status_string(status));
        delete http;
        export_group_->activate();
        export_html_window_->cursor(FL_CURSOR_DEFAULT);
	export_progress_->hide();
        return;
      }
    }

    if (originals)
    {
      for (i = 0; i < browser_->count(); i ++)
      {
	// Update progress...
        item = browser_->value(i);

	snprintf(label, sizeof(label), _("Uploading original/img%05d%s..."), i,
	         fl_filename_ext(item->filename));
	export_progress_->label(label);
	export_progress_->value(40 + 20 * i / browser_->count());
	Fl::check();

	snprintf(filename, sizeof(filename), "%s/original/img%05d%s", dstdir, i,
	         fl_filename_ext(item->filename));
	snprintf(url, sizeof(url), "%soriginal/img%05d%s", resource, i,
	         fl_filename_ext(item->filename));

	status = export_file(http, auth_user_field_->value(),
                             auth_pass_field_->value(), filename, url);

	unlink(filename);

	if (status != HTTP_CREATED &&
            status != HTTP_OK &&
	    status != HTTP_METHOD_NOT_ALLOWED)
	{
          fl_alert(_("Unable to create destination URL:\n\n%d - %s"), status,
                   HTTP::status_string(status));
          delete http;
          export_group_->activate();
          export_html_window_->cursor(FL_CURSOR_DEFAULT);
	  export_progress_->hide();
          return;
	}
      }
    }

    for (i = 0; i < thumbpages; i ++)
    {
      // Update progress...
      if (i == 0)
        strcpy(thumbname, "index.html");
      else
        snprintf(thumbname, sizeof(thumbname), "ind%05d.html", i);

      snprintf(label, sizeof(label), _("Uploading %s..."), thumbname);
      export_progress_->label(label);
      export_progress_->value(60 + 20 * i / thumbpages);
      Fl::check();

      snprintf(filename, sizeof(filename), "%s/%s", dstdir, thumbname);
      snprintf(url, sizeof(url), "%s%s", resource, thumbname);

      status = export_file(http, auth_user_field_->value(),
                           auth_pass_field_->value(), filename, url);

      unlink(filename);

      if (status != HTTP_CREATED &&
          status != HTTP_OK &&
	  status != HTTP_METHOD_NOT_ALLOWED)
      {
        fl_alert(_("Unable to create destination URL:\n\n%d - %s"), status,
                 HTTP::status_string(status));
        delete http;
        export_group_->activate();
        export_html_window_->cursor(FL_CURSOR_DEFAULT);
	export_progress_->hide();
        return;
      }
    }

    for (i = 0; i < imagepages; i ++)
    {
      // Update progress...
      snprintf(imagename, sizeof(imagename), "img%05d.html", i);

      snprintf(label, sizeof(label), _("Uploading %s..."), imagename);
      export_progress_->label(label);
      export_progress_->value(80 + 20 * i / imagepages);
      Fl::check();

      snprintf(filename, sizeof(filename), "%s/%s", dstdir, imagename);
      snprintf(url, sizeof(url), "%s%s", resource, imagename);

      status = export_file(http, auth_user_field_->value(),
                           auth_pass_field_->value(), filename, url);

      unlink(filename);

      if (status != HTTP_CREATED &&
          status != HTTP_OK &&
	  status != HTTP_METHOD_NOT_ALLOWED)
      {
        fl_alert(_("Unable to create destination URL:\n\n%d - %s"), status,
                 HTTP::status_string(status));
        delete http;
        export_group_->activate();
        export_html_window_->cursor(FL_CURSOR_DEFAULT);
	export_progress_->hide();
        return;
      }
    }

    export_progress_->label(_("Uploading slideshow.html..."));
    export_progress_->value(100);
    Fl::check();

    snprintf(filename, sizeof(filename), "%s/slideshow.html", dstdir);
    snprintf(url, sizeof(url), "%sslideshow.html", resource);

    status = export_file(http, auth_user_field_->value(),
                         auth_pass_field_->value(), filename, url);

    unlink(filename);

    if (status != HTTP_CREATED &&
        status != HTTP_OK &&
	status != HTTP_METHOD_NOT_ALLOWED)
    {
      fl_alert(_("Unable to create destination URL:\n\n%d - %s"), status,
               HTTP::status_string(status));
      delete http;
      export_group_->activate();
      export_html_window_->cursor(FL_CURSOR_DEFAULT);
      export_progress_->hide();
      return;
    }

    export_progress_->label(_("Uploading style.css..."));
    export_progress_->value(100);
    Fl::check();

    snprintf(filename, sizeof(filename), "%s/style.css", dstdir);
    snprintf(url, sizeof(url), "%sstyle.css", resource);

    status = export_file(http, auth_user_field_->value(),
                         auth_pass_field_->value(), filename, url);

    unlink(filename);

    if (status != HTTP_CREATED &&
        status != HTTP_OK &&
	status != HTTP_METHOD_NOT_ALLOWED)
    {
      fl_alert(_("Unable to create destination URL:\n\n%d - %s"), status,
               HTTP::status_string(status));
      delete http;
      export_group_->activate();
      export_html_window_->cursor(FL_CURSOR_DEFAULT);
      export_progress_->hide();
      return;
    }

    delete http;
    rmdir(dstdir);

    fl_message(_("Album exported successfully!"));
  }
  else if (!strncmp(path, "ftp:", 4))
  {
    fl_alert(_("FTP upload currently not supported, sorry!"));
  }

  // Hide the window...
  export_progress_->hide();
  export_group_->activate();
  export_html_window_->cursor(FL_CURSOR_DEFAULT);
  export_html_window_->hide();
}


//
// 'export_copy()' - Copy a file verbatim.
//

int					// O - 0 on success, -1 on error
export_copy(const char *src,		// I - Source file
            const char *dst)		// I - Destination file
{
  FILE	*srcfp,				// Source file pointer
	*dstfp;				// Destination file pointer
  char	buffer[8192];			// Copy buffer
  int	bytes;				// Number of bytes read/written


  if ((srcfp = fopen(src, "rb")) == NULL)
    return (-1);

  if ((dstfp = fopen(dst, "wb")) == NULL)
  {
    fclose(srcfp);

    return (-1);
  }

  while ((bytes = fread(buffer, 1, sizeof(buffer), srcfp)) > 0)
    if ((int)fwrite(buffer, 1, bytes, dstfp) < bytes)
    {
      fclose(srcfp);
      fclose(dstfp);

      return (-1);
    }

  fclose(srcfp);
  fclose(dstfp);

  return (0);
}


#ifdef USE_MKCOL
//
// 'export_dir()' - Create the remote directory.
//

static HTTPStatus			// O - HTTP status
export_dir(HTTP       *http,		// I - HTTP connection
           const char *username,	// I - Username
           const char *password,	// I - Password
	   const char *url)		// I - URL to create
{
  HTTPStatus	status;


  http->clear_fields();
  http->set_field(HTTP_FIELD_USER_AGENT, "flPhoto " FLPHOTO_VERSION);
  http->set_field(HTTP_FIELD_CONNECTION, "Keep-Alive");
  if (username[0])
  {
    char	userpass[1024],
		auth[1024];


    strcpy(auth, "Basic ");
    snprintf(userpass, sizeof(userpass), "%s:%s", username, password);
    HTTP::encode64(auth + 6, sizeof(auth) - 6, userpass);
    
    http->set_field(HTTP_FIELD_AUTHORIZATION, auth);
  }

  if (http->send_mkcol(url))
  {
    http->reconnect();

    if (http->send_mkcol(url))
      return (HTTP_ERROR);
  }

  while ((status = http->update()) == HTTP_CONTINUE);

  http->flush();

  if (status >= HTTP_BAD_REQUEST ||
      !strcasecmp(http->get_field(HTTP_FIELD_CONNECTION), "close"))
    http->reconnect();

  return (status);
}
#endif // USE_MKCOL


//
// 'export_file()' - Copy a local file to a remote file.
//

static HTTPStatus			// O - HTTP status
export_file(HTTP       *http,		// I - HTTP connection
            const char *username,	// I - Username
            const char *password,	// I - Password
	    const char *filename,	// I - File to upload
	    const char *url)		// I - URL to create
{
  HTTPStatus	status;			// HTTP status
  FILE		*fp;			// File
  char		buffer[1024];		// Copy buffer
  int		bytes;			// Number of bytes


  if ((fp = fopen(filename, "rb")) == NULL)
    return (HTTP_ERROR);

  fseek(fp, 0, SEEK_END);
  bytes = ftell(fp);
  rewind(fp);

  http->clear_fields();
  http->set_field(HTTP_FIELD_USER_AGENT, "flPhoto " FLPHOTO_VERSION);
  http->set_field(HTTP_FIELD_CONNECTION, "Keep-Alive");
  sprintf(buffer, "%d", bytes);
  http->set_field(HTTP_FIELD_CONTENT_LENGTH, buffer);
  if (username[0])
  {
    char	userpass[1024],
		auth[1024];


    strcpy(auth, "Basic ");
    snprintf(userpass, sizeof(userpass), "%s:%s", username, password);
    HTTP::encode64(auth + 6, sizeof(auth) - 6, userpass);
    
    http->set_field(HTTP_FIELD_AUTHORIZATION, auth);
  }

  if (http->send_put(url))
  {
    http->reconnect();
    if (http->send_put(url))
    {
      fclose(fp);
      return (HTTP_ERROR);
    }
  }

  status = HTTP_CONTINUE;

  while ((bytes = fread(buffer, 1, sizeof(buffer), fp)) > 0)
  {
    if (http->check())
    {
      if ((status = http->update()) != HTTP_CONTINUE)
        break;
    }

    http->write(buffer, bytes);
  }

  fclose(fp);

  if (status == HTTP_CONTINUE)
  {
    while ((status = http->update()) == HTTP_CONTINUE);
  }

  http->flush();
  if (status >= HTTP_BAD_REQUEST ||
      !strcasecmp(http->get_field(HTTP_FIELD_CONNECTION), "close"))
    http->reconnect();

  return (status);
}


//
// 'export_footer()' - Write a HTML footer.
//

static void
export_footer(FILE       *fp,		// I - File to write to
              const char *footer)	// I - Footer text
{
  if (fp)
  {
    fputs("</table>\n", fp);
    if (footer && *footer)
    {
      fputs("<hr>\n", fp);
      export_string(fp, footer, true, "p");
    }
    fputs("</body>\n", fp);
    fputs("</html>\n", fp);
    fclose(fp);
  }
}


//
// 'export_header()' - Write a HTML header.
//

static FILE *				// O - New file
export_header(const char *filename,	// I - File to create
              const char *header,	// I - Header text
              const char *comment)	// I - Comment text
{
  FILE	*fp;				// New file


  if ((fp = fopen(filename, "wb")) != NULL)
  {
    fputs("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" "
          "\"http://www.w3.org/TR/html4/loose.dtd\">\n", fp);

    fputs("<html>\n", fp);
    fputs("<head>\n", fp);
    fputs("<meta http-equiv='Content-Type' content='text/html; "
          "charset=utf-8'>\n", fp);
    fputs("<link rel='StyleSheet' href='style.css' type='text/css'>\n", fp);
    if (header && *header)
      export_string(fp, header, false, "title");
    else
      export_string(fp, comment, false, "title");
    fputs("</head>\n", fp);
    fputs("<body class='flphoto'>\n", fp);
    export_string(fp, header, true, "h1");
    export_string(fp, comment, true, "p");
    fputs("<table class='flphoto' summary=''>\n", fp);
  }

  return (fp);
}


//
// 'export_jpeg()' - Write an image file in JPEG format.
//

static int				// O - 0 on failure, 1 on success
export_jpeg(const char      *filename,	// I - File to write to
            Fl_Shared_Image *img,	// I - Image to write
	    int             w,		// I - Width of image
	    int             h,		// I - Height of image
            int             quality,	// I - Image quality
            const char      *watermark,	// I - Watermark text, if any
	    int             font,	// I - Font for watermark(s)
	    int             fontsize,	// I - Size of watermark(s)
            int             opacity,	// I - Opacity of watermark(s)
	    int             pos)	// I - Position of watermark(s)
{
  int				i;	// Looping var
  Fl_Shared_Image		*simg;	// Resized image
  uchar				*ptr;	// Pointer to image data
  FILE				*fp;	// File pointer
  struct jpeg_compress_struct	info;	// Compressor info
  struct jpeg_error_mgr		err;	// Error handler info
  Fl_Offscreen			offscreen;
					// Offscreen rendering buffer
  uchar				*waterimg,
					// Watermark image
				*waterptr;
					// Pointer into image
  int				waterv;	// Pixel value
  Fl_Color			shadow,	// Shadow color
				highlight,
					// Highlight color
				text;	// Text color


  // Create the output file...
  if ((fp = fopen(filename, "wb")) == NULL)
  {
    fl_alert(_("Unable to create JPEG image:\n\n%s"), strerror(errno));

    return (0);
  }

  // Resize the image...
  simg = (Fl_Shared_Image *)img->copy(w, h);

  if (watermark && *watermark)
  {
    // Apply a watermark in the middle of the image...
    if (w > h)
      fl_font(font, fontsize * h / 100);
    else
      fl_font(font, fontsize * w / 100);

    offscreen = fl_create_offscreen(w, h);
    fl_begin_offscreen(offscreen);

    fl_color(fl_rgb_color(127, 127, 127));
    fl_rectf(0, 0, w, h);

    shadow    = fl_rgb_color(127 - opacity, 127 - opacity, 127 - opacity);
    highlight = fl_rgb_color(127 + opacity, 127 + opacity, 127 + opacity);
    i         = 127 + opacity * opacity * opacity / 10000;
    text      = fl_rgb_color(i, i, i);

    if (pos == WM_POS_CENTER || pos == WM_POS_ALL)
    {
      fl_color(shadow);
      fl_draw(watermark, 2, 0, w - 2, h - 2,
              (Fl_Align)(FL_ALIGN_WRAP | FL_ALIGN_INSIDE), 0, 0);
      fl_color(highlight);
      fl_draw(watermark, 0, 2, w - 2, h - 2,
              (Fl_Align)(FL_ALIGN_WRAP | FL_ALIGN_INSIDE), 0, 0);
      fl_color(text);
      fl_draw(watermark, 1, 1, w - 2, h - 2,
              (Fl_Align)(FL_ALIGN_WRAP | FL_ALIGN_INSIDE), 0, 0);
    }

    if (pos == WM_POS_TOP || pos == WM_POS_TOP_BOTTOM || pos == WM_POS_ALL)
    {
      fl_color(shadow);
      fl_draw(watermark, 2, 0, w - 2, h - 2,
              (Fl_Align)(FL_ALIGN_WRAP | FL_ALIGN_INSIDE | FL_ALIGN_TOP), 0, 0);
      fl_color(highlight);
      fl_draw(watermark, 0, 2, w - 2, h - 2,
              (Fl_Align)(FL_ALIGN_WRAP | FL_ALIGN_INSIDE | FL_ALIGN_TOP), 0, 0);
      fl_color(text);
      fl_draw(watermark, 1, 1, w - 2, h - 2,
              (Fl_Align)(FL_ALIGN_WRAP | FL_ALIGN_INSIDE | FL_ALIGN_TOP), 0, 0);
    }

    if (pos == WM_POS_BOTTOM || pos == WM_POS_TOP_BOTTOM || pos == WM_POS_ALL)
    {
      fl_color(shadow);
      fl_draw(watermark, 2, 0, w - 2, h - 2,
              (Fl_Align)(FL_ALIGN_WRAP | FL_ALIGN_INSIDE | FL_ALIGN_BOTTOM), 0, 0);
      fl_color(highlight);
      fl_draw(watermark, 0, 2, w - 2, h - 2,
              (Fl_Align)(FL_ALIGN_WRAP | FL_ALIGN_INSIDE | FL_ALIGN_BOTTOM), 0, 0);
      fl_color(text);
      fl_draw(watermark, 1, 1, w - 2, h - 2,
              (Fl_Align)(FL_ALIGN_WRAP | FL_ALIGN_INSIDE | FL_ALIGN_BOTTOM), 0, 0);
    }

    if (pos == WM_POS_LEFT || pos == WM_POS_LEFT_RIGHT || pos == WM_POS_ALL)
    {
      fl_color(shadow);
      fl_draw(watermark, 2, 0, w - 2, h - 2,
              (Fl_Align)(FL_ALIGN_WRAP | FL_ALIGN_INSIDE | FL_ALIGN_LEFT), 0, 0);
      fl_color(highlight);
      fl_draw(watermark, 0, 2, w - 2, h - 2,
              (Fl_Align)(FL_ALIGN_WRAP | FL_ALIGN_INSIDE | FL_ALIGN_LEFT), 0, 0);
      fl_color(text);
      fl_draw(watermark, 1, 1, w - 2, h - 2,
              (Fl_Align)(FL_ALIGN_WRAP | FL_ALIGN_INSIDE | FL_ALIGN_LEFT), 0, 0);
    }

    if (pos == WM_POS_RIGHT || pos == WM_POS_LEFT_RIGHT || pos == WM_POS_ALL)
    {
      fl_color(shadow);
      fl_draw(watermark, 2, 0, w - 2, h - 2,
              (Fl_Align)(FL_ALIGN_WRAP | FL_ALIGN_INSIDE | FL_ALIGN_RIGHT), 0, 0);
      fl_color(highlight);
      fl_draw(watermark, 0, 2, w - 2, h - 2,
              (Fl_Align)(FL_ALIGN_WRAP | FL_ALIGN_INSIDE | FL_ALIGN_RIGHT), 0, 0);
      fl_color(text);
      fl_draw(watermark, 1, 1, w - 2, h - 2,
              (Fl_Align)(FL_ALIGN_WRAP | FL_ALIGN_INSIDE | FL_ALIGN_RIGHT), 0, 0);
    }

    // Make sure everything is drawn...
    Fl::flush();

    // Read the image back...
    waterimg = fl_read_image(0, 0, 0, w, h);
    fl_end_offscreen();
    fl_delete_offscreen(offscreen);

    if (img->d() == 1)
    {
      for (ptr = (uchar *)(simg->data()[0]), waterptr = waterimg, i = w * h;
           i > 0;
           ptr ++, waterptr += 3, i --)
      {
        waterv = ptr[0] + waterptr[0] - 127;
        if (waterv < 0)
          ptr[0] = 0;
        else if (waterv < 255)
          ptr[0] = waterv;
        else
          ptr[0] = 255;
      }
    }
    else
    {
      for (ptr = (uchar *)(simg->data()[0]), waterptr = waterimg, i = w * h * 3;
           i > 0;
           i --)
      {
        waterv = *ptr + *waterptr++ - 127;

        if (waterv < 0)
          *ptr++ = 0;
        else if (waterv < 255)
          *ptr++ = waterv;
        else
          *ptr++ = 255;
      }
    }

    free(waterimg);
  }

  // Setup the JPEG compression stuff...
  info.err = jpeg_std_error(&err);
  jpeg_create_compress(&info);
  jpeg_stdio_dest(&info, fp);

  info.image_width      = simg->w();
  info.image_height     = simg->h();
  info.input_components = simg->d();
  info.in_color_space   = simg->d() == 1 ? JCS_GRAYSCALE : JCS_RGB;

  jpeg_set_defaults(&info);
  jpeg_set_quality(&info, quality, 1);
  jpeg_simple_progression(&info);

  info.optimize_coding = 1;

  // Save the image...
  jpeg_start_compress(&info, 1);

  i = 0;
  while (info.next_scanline < info.image_height)
  {
    ptr = (uchar *)simg->data()[0] + info.next_scanline * simg->w() * simg->d();

    jpeg_write_scanlines(&info, &ptr, 1);
    i ++;
  }

  jpeg_finish_compress(&info);
  jpeg_destroy_compress(&info);

  fclose(fp);

  simg->release();

  return (1);
}


//
// 'export_string()' - Write a string to a HTML file.
//

static void
export_string(FILE       *fp,		// I - File to write to
              const char *s,		// I - String to write
	      bool       breaks,	// I - Replace newlines with <br>?
	      const char *el)		// I - Element to wrap with
{
  if (!s || !*s)
    return;

  if (el)
    fprintf(fp, "<%s>", el);

  while (*s)
  {
    switch (*s)
    {
      case '&' :
          fputs("&amp;", fp);
	  break;
      case '<' :
          fputs("&lt;", fp);
	  break;
      case '>' :
          fputs("&gt;", fp);
	  break;
      case '\"' :
          fputs("&quot;", fp);
	  break;
      case '\'' :
          fputs("&#39;", fp);
	  break;
      case '\n' :
          if (breaks)
            fputs("<br>", fp);
	  else
	    putc(' ', fp);
	  break;
      default :
          if (*s & 128)
	  {
	    // Convert ISO-8859-1 to UTF-8...
	    putc(0xc0 | ((*s & 255) >> 6), fp);
            putc(0x80 | (*s & 63), fp);
	  }
	  else
            putc(*s, fp);
	  break;
    }

    s ++;
  }

  if (el)
    fprintf(fp, "</%s>", el);
}


//
// End of "$Id: export.cxx 414 2006-11-18 03:24:41Z mike $".
//
