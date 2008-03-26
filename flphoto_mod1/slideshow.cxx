//
// "$Id: slideshow.cxx 427 2006-11-21 03:09:10Z mike $"
//
// Slideshow methods.
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
//   flphoto::slideshow_cb() - Show a slideshow.
//   flphoto::slidenext_cb() - Show the next image...
//

#include "flphoto.h"
#include "i18n.h"


//
// Local globals...
//

static flphoto		*current_album;	// Current album
static int		current_image;	// Current image
static int		*image_order;	// Order of images
static Fl_Double_Window	*window = 0;	// Display window
static Fl_Image_Display	*display = 0;	// Display widget
static float		slide_delay;	// Delay between slides
static int		slide_repeat;	// Repeat slides
static int		slide_comments;	// Show comments
static char		comment[1024];	// Comment label


//
// 'flphoto::slideshow_cb()' - Show a slideshow.
//

void
flphoto::slideshow_cb()
{
  int	i, j;				// Looping vars
  int	x, y, w, h;			// Size and position of slideshow
  int	val;				// Preference value
  int	slide_screen,			// Screen to show
	slide_size,			// Size option for slideshow
	slide_width,			// Width of slideshow
	slide_height;			// Height of slideshow


  // See if we have any images in this album...
  if (!browser_->count())
  {
    fl_alert(_("No images to show in a slideshow!"));
    return;
  }

  // Save the current album...
  current_album = this;
  current_image = -1;

  // Determine the location and size of the slideshow window...
  prefs.get("slideshow_screen", slide_screen, 0);

  if (slide_screen)
    Fl::screen_xywh(x, y, w, h, slide_screen - 1);
  else
    Fl::screen_xywh(x, y, w, h, window_->x(), window_->y());

  prefs.get("slideshow_size", slide_size, 0);
  prefs.get("slideshow_width", slide_width, 0);
  prefs.get("slideshow_height", slide_height, 0);

  if (slide_width < 1 || slide_size <= 0)
    slide_width = w;
  if (slide_height < 1 || slide_size <= 0)
    slide_height = h;

  // Create the slideshow window as needed...
  if (!window)
  {
    window = new Fl_Double_Window(x, y, w, h);
    window->color(FL_BLACK);
    window->box(FL_FLAT_BOX);
    window->image(0);
    window->border(0);
    window->modal();

    display = new Fl_Image_Display((w - slide_width) / 2,
                                   (h - slide_height) / 2,
                                   slide_width, slide_height, comment);

    display->align((Fl_Align)(FL_ALIGN_BOTTOM | FL_ALIGN_INSIDE |
                              FL_ALIGN_WRAP));
    display->labelcolor(FL_WHITE);
    display->labelsize(25);
    display->labelfont(FL_BOLD);
    display->labeltype(FL_SHADOW_LABEL);
    display->box(FL_FLAT_BOX);
    display->color(FL_BLACK);
    display->mode(FL_IMAGE_CLICK);
    display->callback((Fl_Callback *)slidenext_cb);
  }
  else
  {
    // Resize as needed...
    window->resize(x, y, w, h);

    display->resize((w - slide_width) / 2, (h - slide_height) / 2,
                    slide_width, slide_height);
  }

  // Create an array for the images...
  image_order = new int[browser_->count()];

  prefs.get("slideshow_delay", slide_delay, 5.0);
  prefs.get("slideshow_repeat", slide_repeat, 1);
  prefs.get("slideshow_comments", slide_comments, 1);
  prefs.get("slideshow_first", i, 1);

  if (i || browser_->selected() < 0)
  {
    for (i = 0; i < browser_->count(); i ++)
      image_order[i] = i;
  }
  else
  {
    for (i = 0, j = browser_->selected(); i < browser_->count(); i ++)
    {
      image_order[i] = j;

      j ++;
      if (j >= browser_->count())
        j = 0;
    }
  }

  prefs.get("slideshow_random", i, 0);

  if (i)
  {
    // Randomize the slideshow...
    for (i = 1; i < browser_->count(); i ++)
    {
      // Pick a slide to swap with...
      do
      {
        j = rand() % browser_->count();
      }
      while (j == 0 || j == i);

      // Swap...
      val            = image_order[i];
      image_order[i] = image_order[j];
      image_order[j] = val;
    }
  }

  // Clear the slideshow window and then show the next image...
  display->image(0);
  comment[0] = '\0';

  window->show();

  slidenext_cb();

  // Wait until the user closes the slideshow...
  while (window->shown())
    Fl::wait();

  // Remove any pending timeouts...
  Fl::remove_timeout((Fl_Timeout_Handler)slidenext_cb);

  // Show the current image in the window...
  open_image_cb(current_image);
}


//
// 'flphoto::slidenext_cb()' - Show the next image...
//

void
flphoto::slidenext_cb()
{
  Fl_Image_Browser::ITEM *item;		// Item in image browser...


  // Remove any prior slideshow timeouts...
  Fl::remove_timeout((Fl_Timeout_Handler)slidenext_cb);

  // If the window is no longer visible, then we're done!
  if (!window->shown())
    return;

  // Loop until we find the next image...
  for (;;)
  {
    // Unload the current image, if any...
    if (current_image >= 0)
    {
      item = current_album->browser_->value(image_order[current_image]);

      if (item->image && !item->changed && item != current_album->image_item_)
      {
        item->image->release();
	item->image = 0;
      }
    }

    // Advance to the next or previous image...
    if (Fl::event_button() == FL_RIGHT_MOUSE ||
        Fl::event_key() == FL_BackSpace)
      current_image --;
    else
      current_image ++;

    // Wrap to the beginning or end, as needed...
    if (current_image >= current_album->browser_->count())
    {
      current_image = 0;

      if (!slide_repeat)
      {
        // Don't go back to the first image...
        window->hide();
        return;
      }
    }
    else if (current_image < 0)
      current_image = current_album->browser_->count() - 1;

    // Load the current image...
    window->cursor(FL_CURSOR_WAIT);
    Fl::flush();

    display->value(current_album->browser_->load_item(
                         image_order[current_image]));
    if (slide_comments)
    {
      const char *src = current_album->browser_->value(
                            image_order[current_image])->comments;
      char	*dst = comment;

      if (src)
      {
	while (*src && dst < (comment + sizeof(comment) - 2))
	{
          if (*src == '@')
	    *dst++ = '@';

	  *dst++ = *src++;
	}
      }

      *dst = '\0';
    }

    Fl::flush();

    // Pre-load next image...
    if ((current_image + 1) < current_album->browser_->count())
      current_album->browser_->load_item(image_order[current_image + 1]);
    else
      current_album->browser_->load_item(image_order[0]);

    window->cursor(FL_CURSOR_DEFAULT);

    break;
  }

  // Set a timer for the next image...
  if (slide_delay > 0.0)
    Fl::add_timeout(slide_delay, (Fl_Timeout_Handler)slidenext_cb);
}


//
// End of "$Id: slideshow.cxx 427 2006-11-21 03:09:10Z mike $".
//
