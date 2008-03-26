//
// "$Id: gamma.cxx 405 2006-11-12 17:09:30Z mike $"
//
// Gamma calibration and option dialog methods.
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
//   flphoto::options_cb()      - Show the options window.
//   flphoto::options_ok_cb()   - Accept options changes.
//   flphoto::gamma_slider_cb() - Update the gamma image.
//

#include "flphoto.h"
#include <stdio.h>
#include <stdlib.h>
#include "flstring.h"
#include <ctype.h>
#include <errno.h>
#include <math.h>


//
// 'flphoto::options_cb()' - Show the options window.
//

void
flphoto::options_cb(Fl_Group *tab)	// I - Tab to show
{
  int	ival;				// Zoom/open values
  float	val;				// Gamma value
  char	sval[1024];			// String value


  // Get the image preferences...
  prefs.get("auto_open", ival, 1);
  auto_open_button_->value(ival);

  prefs.get("keep_zoom", ival, 1);
  keep_zoom_button_->value(ival);

  prefs.get("image_editor", sval, "gimp", sizeof(sval));
  image_editor_field_->value(sval);

  // Get the gamma preferences...
  prefs.get("gamma", val, 2.2);
  gamma_slider_->value(val);

  gamma_slider_cb();

  // Get the saved slideshow preferences...
  prefs.get("slideshow_delay", val, 5);
  ssoptions_delay_value_->value(val);

  prefs.get("slideshow_first", ival, 1);
  ssoptions_first_button_->value(ival);

  prefs.get("slideshow_random", ival, 0);
  ssoptions_random_button_->value(ival);

  prefs.get("slideshow_repeat", ival, 1);
  ssoptions_repeat_button_->value(ival);

  prefs.get("slideshow_comments", ival, 1);
  ssoptions_comments_button_->value(ival);

  Fl_Menu_Item *items = (Fl_Menu_Item *)ssoptions_screen_chooser_->menu();
  for (int i = 1; i < 5; i ++)
    if (i > Fl::screen_count())
      items[i].hide();
    else
      items[i].show();

  prefs.get("slideshow_screen", ival, 0);
  if (ival > Fl::screen_count())
    ssoptions_screen_chooser_->value(0);
  else
    ssoptions_screen_chooser_->value(ival);

  prefs.get("slideshow_size", ival, 0);
  ssoptions_size_chooser_->value(ival);

  prefs.get("slideshow_width", ival, 0);
  ssoptions_width_value_->value(ival);

  prefs.get("slideshow_height", ival, 0);
  ssoptions_height_value_->value(ival);

  if (ssoptions_size_chooser_->value())
    ssoptions_custom_group_->activate();
  else
    ssoptions_custom_group_->deactivate();

  // Get the saved flash card preferences...
  prefs.get("flash_dir", sval, "/mnt/flash", sizeof(sval));
  flash_dir_field_->value(sval);

  prefs.get("flash_mount", ival, 1);
  flash_mount_button_->value(ival);

  // Show the options window...
  options_tabs_->value(tab);
  options_window_->hotspot(options_window_);
  options_window_->show();
}


//
// 'flphoto::options_ok_cb()' - Accept options changes.
//

void
flphoto::options_ok_cb()
{
  float		val;			// Gamma value
  flphoto	*album;			// Looping var


  // Hide the options window...
  options_window_->hide();

  // Save the new image settings...
  prefs.set("auto_open", auto_open_button_->value());
  prefs.set("keep_zoom", keep_zoom_button_->value());
  prefs.set("image_editor", image_editor_field_->value());

  // Save the new gamma setting...
  val = gamma_slider_->value();

  prefs.set("gamma", val);
  Fl_Image_Display::set_gamma(val);

  // Save the slideshow settings...
  prefs.set("slideshow_delay", (int)ssoptions_delay_value_->value());
  prefs.set("slideshow_first", ssoptions_first_button_->value());
  prefs.set("slideshow_random", ssoptions_random_button_->value());
  prefs.set("slideshow_repeat", ssoptions_repeat_button_->value());
  prefs.set("slideshow_comments", ssoptions_comments_button_->value());
  prefs.set("slideshow_screen", ssoptions_screen_chooser_->value());
  prefs.set("slideshow_size", ssoptions_size_chooser_->value());
  prefs.set("slideshow_width", (int)ssoptions_width_value_->value());
  prefs.set("slideshow_height", (int)ssoptions_height_value_->value());

  // Save the flash card preferences...
  prefs.set("flash_dir", flash_dir_field_->value());
  prefs.set("flash_mount", flash_mount_button_->value());

  // Tell all of the album windows to redraw...
  for (album = album_first_; album; album = album->album_next_)
    album->display_->redraw();
}


//
// 'flphoto::gamma_slider_cb()' - Update the gamma image.
//

void
flphoto::gamma_slider_cb()
{
  int	x, y;					// Coordinates in image
  float	val;					// Gamma value
  uchar	g,					// Gamma-adjusted value
	*ptr;					// Pointer into image


  if (!gamma_image_)
  {
    gamma_image_ = new Fl_RGB_Image(gamma_data_, 100, 100, 1);
    gamma_box_->image(gamma_image_);
  }

  val = 1.0 / gamma_slider_->value();

  memset(gamma_data_, 255, sizeof(gamma_data_));

  for (y = 0, ptr = gamma_data_; y < 100; y ++, ptr += 100)
  {
    for (x = y & 1; x < 100; x += 2)
      ptr[x] = 0;

    if (y >= 25 && y < 75)
    {
      g = (int)(255.0 * pow(0.5, val) + 0.5);

      for (x = 25; x < 75; x ++)
	ptr[x] = g;
    }
  }

  gamma_image_->uncache();
  gamma_box_->redraw();
}


//
// End of "$Id: gamma.cxx 405 2006-11-12 17:09:30Z mike $".
//
