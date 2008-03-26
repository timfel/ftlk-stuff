//
// "$Id: Fl_Slideshow.cxx 327 2005-01-25 22:31:11Z easysw $"
//
// Slideshow display class code for flPhoto.
//
// Copyright 2005 by Michael Sweet
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
//

//
// Include necessary headers...
//

#include "Fl_Slideshow.h"
#include "debug.h"
#include <FL/fl_draw.H>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


//
// 'Fl_SlideFile::Fl_SlideFile()' - Create a slide file.
//

Fl_SlideFile::Fl_SlideFile(const char *f,
					// I - Filename
                           const char *c,
					// I - Comment text
			   int        p,// I - Position
                           float      start,
					// I - Start zoom
			   float      end)
					// I - End zoom
{
  filename = f ? strdup(f) : NULL;
  comment  = c ? strdup(c) : NULL;
  position = p;
  zoom[0]  = start;
  zoom[1]  = end;
}


//
// 'Fl_SlideFile::~Fl_SlideFile()' - Destroy a slide file.
//

Fl_SlideFile::~Fl_SlideFile()
{
  if (filename)
  {
    free(filename);
    filename = (char *)0;
  }

  if (comment)
  {
    free(comment);
    comment = (char *)0;
  }
}


//
// 'Fl_Slideshow::Fl_Slideshow()' - Create a slideshow widget.
//

Fl_Slideshow::Fl_Slideshow(int        X,// I - X position
                           int        Y,// I - Y position
			   int        W,// I - Width
			   int        H,// I - Height
			   const char *L)
					// I - Label
  : Fl_Widget(X, Y, W, H, L),
    comp_(W, H)
{
  alloc_       = 0;
  count_       = 0;
  files_       = (Fl_SlideFile **)0;
  image_.image = (Fl_Shared_Image *)0;
  running_     = false;
  timeout_     = 0.0f;
  ximage_      = (XImage *)0;
  xshmsize_    = 0;

  current(0);
  delay(10.0f);
  fade(1.0f);
  position(FL_SLIDESHOW_CENTER);
  rate(0.1f);
  repeat(false);
  step(1);
  textcolor(FL_WHITE);
  textfont(FL_HELVETICA);
  textsize(FL_NORMAL_SIZE);
  zoom(0.0, 0.0);
}


//
// 'Fl_Slideshow::~Fl_Slideshow()' - Destroy a slideshow widget.
//

Fl_Slideshow::~Fl_Slideshow()
{
  stop();

  clear();

  if (alloc_)
    delete[] files_;

  if (ximage_)
  {
    XShmDetach(fl_display, &xshminfo_);
    XDestroyImage(ximage_);
    ximage_ = (XImage *)0;
  }

  if (xshmsize_)
  {
    DEBUG_printf(("removing shm %d...\n", xshminfo_.shmid));

    if (shmdt(xshminfo_.shmaddr))
      perror("shmdt failed");

    if (shmctl(xshminfo_.shmid, IPC_RMID, 0))
      perror("shmctl failed");
  }
}


//
// 'Fl_Slideshow::add()' - Add an image file.
//

void
Fl_Slideshow::add(const char *f,	// I - Filename to add
                  const char *c,	// I - Comment text
		  int        p,		// I - Position
                  float      start,	// I - Start zoom
		  float      end)	// I - End zoom
{
  Fl_SlideFile	**temp;			// New file array pointer


  if (!f)
    return;

  if (count_ >= alloc_)
  {
    temp = new Fl_SlideFile *[alloc_ + 10];

    if (alloc_ > 0)
      memcpy(temp, files_, alloc_ * sizeof(Fl_SlideFile *));

    delete[] files_;
    files_ = temp;

    alloc_ += 10;
  }

  files_[count_] = new Fl_SlideFile(f, c, p, start, end);
  count_ ++;

  if (count_ == 1 && running_)
    Fl::add_timeout(rate_, timer_cb, this);
}


//
// 'Fl_Slideshow::clear()' - Clear the list of image files.
//

void
Fl_Slideshow::clear()
{
  int	i;				// Looping var


  if (image_.image)
  {
    image_.image->release();
    image_.image = (Fl_Shared_Image *)0;
  }

  for (i = 0; i < count_; i ++)
  {
    delete files_[i];
    files_[i] = (Fl_SlideFile *)0;
  }

  count_ = 0;
}


//
// 'Fl_Slideshow::draw()' - Draw the current slide...
//

void
Fl_Slideshow::draw()
{
  int			i;		// Looping var...
  float			alpha;		// Alpha value
  int			X, Y;		// X/Y position of image
  int			W, H;		// Width and height
  float			z;		// Zoom value
  Fl_Shared_Image	*img;		// Current image
  uchar			r, g, b;	// Background color
  unsigned char		*src;		// Source pixel pointer
  unsigned		*dst;		// Destination pixel pointer
  static bool		xshm_check = false;
					// Checked for XShm extension?
  static bool		xshm_present;
					// XShm extension present?


  if (!xshm_check)
  {
    xshm_check = true;

    if (fl_visual->depth != 32 || fl_visual->red_mask != 0xff0000)
      // We only support 32-bit TrueColor right now...
      xshm_present = false;
    else
      xshm_present = XShmQueryExtension(fl_display) != 0;
  }

  if (xshm_present)
  {
    if (!xshmsize_)
    {
      memset(&xshminfo_, 0, sizeof(xshminfo_));

      xshmsize_         = Fl::w() * Fl::h() * 4;
      xshminfo_.shmid   = shmget(IPC_PRIVATE, xshmsize_, IPC_CREAT | 0777);
      xshminfo_.shmaddr = (char *)shmat(xshminfo_.shmid, 0, 0);

      DEBUG_printf(("created shm %d...\n", xshminfo_.shmid));
    }

    if (!ximage_)
    {
      ximage_ = XShmCreateImage(fl_display, fl_visual->visual, fl_visual->depth,
                        	ZPixmap, xshminfo_.shmaddr, &xshminfo_, w(), h());
      ximage_->data = xshminfo_.shmaddr;
      XShmAttach(fl_display, &xshminfo_);

      DEBUG_printf(("ximage_ =\n"
        	    "    width=%d\n"
		    "    height=%d\n"
		    "    xoffset=%d\n"
		    "    format=%d\n"
		    "    data=%p\n"
		    "    byte_order=%d\n"
		    "    bitmap_unit=%d\n"
		    "    bitmap_bit_order=%d\n"
		    "    bitmap_pad=%d\n"
		    "    depth=%d\n"
		    "    bytes_per_line=%d\n"
		    "    bits_per_pixel=%d\n"
		    "    red_mask=%lx\n"
		    "    green_mask=%lx\n"
		    "    blue_mask=%lx\n",
		    ximage_->width, ximage_->height, ximage_->xoffset,
		    ximage_->format, ximage_->data,
		    ximage_->byte_order, ximage_->bitmap_unit,
		    ximage_->bitmap_bit_order, ximage_->bitmap_pad,
		    ximage_->depth, ximage_->bytes_per_line,
		    ximage_->bits_per_pixel, ximage_->red_mask,
		    ximage_->green_mask, ximage_->blue_mask));
    }
  }

  if (timeout_ >= delay_)
  {
    if (swapped_)
    {
      // Load the next image...
      Fl_SlideFile	*current;	// Current slide file
      int		p;		// Current position


      swapped_ = false;

      current = files_[current_];

      DEBUG_printf(("Loading '%s'...\n", current->filename));

      image_.comment = current->comment;
      image_.image   = img = Fl_Shared_Image::get(current->filename);

      DEBUG_printf(("    image WxH=%dx%d\n", img->w(), img->h()));

      if ((p = current->position) == FL_SLIDESHOW_DEFAULT)
        p = position_;

      if (p == FL_SLIDESHOW_RANDOM)
	image_.position = (rand() % 3) | ((rand() % 3) << 2) |
	                  ((rand() & 1) << 4);
      else if (p == (FL_SLIDESHOW_RANDOM | FL_SLIDESHOW_PAN))
      {
        if (image_.position & FL_SLIDESHOW_BOTTOM_LEFT)
	{
          if ((w() > h()) == (img->w() > img->h()))
	    image_.position = FL_SLIDESHOW_RIGHT | FL_SLIDESHOW_PAN;
	  else
	    image_.position = FL_SLIDESHOW_TOP | FL_SLIDESHOW_PAN;
	}
	else
	{
          if ((w() > h()) == (img->w() > img->h()))
	    image_.position = FL_SLIDESHOW_LEFT | FL_SLIDESHOW_PAN;
	  else
	    image_.position = FL_SLIDESHOW_BOTTOM | FL_SLIDESHOW_PAN;
	}
      }
      else
	image_.position = p;

      if ((image_.zoom[0] = current->zoom[0]) == FL_SLIDESHOW_DEFAULT)
      {
        if (alternate_)
          image_.zoom[0] = zoom_[current_ & 1];
	else
          image_.zoom[0] = zoom_[0];
      }

      if ((image_.zoom[1] = current->zoom[1]) == FL_SLIDESHOW_DEFAULT)
      {
        if (alternate_)
          image_.zoom[1] = zoom_[1 - (current_ & 1)];
	else
          image_.zoom[1] = zoom_[1];
      }

      // Update the zoom from 0.0 to scale value so that transition scaling
      // works right...
      if (img && img->w() && img->h() &&
          (image_.zoom[0] == 0.0 || image_.zoom[1] == 0.0))
      {
	W = w();
	H = W * img->h() / img->w();

	if (H > h())
	{
	  H = h();
	  W = H * img->w() / img->h();
	}

        if ((w() > h()) == (img->w() > img->h()))
	  z = (float)H / (float)h();
	else
	  z = (float)W / (float)w();

        DEBUG_printf(("    auto-zoom = %f (%dx%d)\n", z, W, H));

	for (i = 0; i < 2; i ++)
	  if (image_.zoom[i] == 0.0)
	    image_.zoom[i] = z;
      }
    }

    // Fade the new image in...
    alpha = 1.0f - (timeout_ - delay_) / fade_;
  }
  else if (timeout_ > fade_)
  {
    // Solid image...
    alpha = 1.0f;
  }
  else
  {
    // Fade the image out...
    alpha    = timeout_ / fade_;
    swapped_ = true;
  }

  Fl::get_color(color(), r, g, b);
  comp_.rgb(r, g, b);

  DEBUG_puts("Fl_Slideshow::draw()");

  if (image_.image && image_.image->w() && image_.image->h())
  {
    float delta;			// Delta within transition...

    delta = (delay_ + fade_ - timeout_) / (delay_ + fade_);

    // Figure out the size of the image...
    z   = image_.zoom[0] * (1.0f - delta) + image_.zoom[1] * delta;
    img = image_.image;

    DEBUG_printf(("    timeout_=%f, delta=%f, zoom=[%f %f], z=%f\n",
        	  timeout_, delta, image_.zoom[0], image_.zoom[1], z)); 

    if ((w() > h()) == (img->w() > img->h()))
    {
      H = (int)(h() * z);
      W = H * img->w() / img->h();
    }
    else
    {
      W = (int)(w() * z);
      H = W * img->h() / img->w();
    }

    DEBUG_printf(("    WxH=%dx%d...\n", W, H));

    if (!W || !H)
      W = H = 1;

    // Figure out the X,Y position of the image...
    if (image_.position & FL_SLIDESHOW_PAN)
    {
      if (image_.position & FL_SLIDESHOW_LEFT)
	X = (int)((w() - W) * delta);
      else if (image_.position & FL_SLIDESHOW_RIGHT)
	X = w() - W - (int)((w() - W) * delta);
      else
	X = (w() - W) / 2;

      if (image_.position & FL_SLIDESHOW_BOTTOM)
	Y = (int)((h() - H) * delta);
      else if (image_.position & FL_SLIDESHOW_TOP)
	Y = h() - H - (int)((h() - H) * delta);
      else
	Y = (h() - H) / 2;
    }
    else
    {
      if (image_.position & FL_SLIDESHOW_LEFT)
	X = 0;
      else if (image_.position & FL_SLIDESHOW_RIGHT)
	X = w() - W;
      else
	X = (w() - W) / 2;

      if (image_.position & FL_SLIDESHOW_BOTTOM)
	Y = 0;
      else if (image_.position & FL_SLIDESHOW_TOP)
	Y = h() - H;
      else
	Y = (h() - H) / 2;
    }

    DEBUG_printf(("    X,Y=%d,%d, position=%d\n", X, Y, image_.position));

    // Draw this image...
    comp_.image(X, Y, W, H,image_.image->w(), image_.image->h(),
                image_.image->d(), (int)(255.0f * alpha), true,
		(unsigned char *)(image_.image->data()[0]));

    if (ximage_)
    {
      DEBUG_puts("    using XShmPutImage()");

      for (i = w() * h(), src = comp_.pixels(), dst = (unsigned *)ximage_->data;
           i > 0;
	   i --)
      {
	*dst++ = (((src[0] << 8) | src[1]) << 8) | src[2];
	src += 3;
      }

      XShmPutImage(fl_display, fl_window, fl_gc, ximage_, 0, 0, x(), y(),
                   w(), h(), False);
      XFlush(fl_display);
    }
    else
    {
      DEBUG_puts("    using fl_draw_image()");

      fl_draw_image(comp_.pixels(), x(), y(), w(), h());
    }
  }
  else
  {
    DEBUG_puts("    using fl_rectf()");

    fl_color(color());
    fl_rectf(x(), y(), w(), h());
  }
}


//
// 'Fl_Slideshow::handle()' - Handle mouse clicks and key presses.
//

int					// O - 1 if handled, 0 otherwise
Fl_Slideshow::handle(int event)		// I - Event
{
  return (0);
}


//
// 'Fl_Slideshow::resize()' - Resize the widget.
//

void
Fl_Slideshow::resize(int X,		// I - X position
                     int Y,		// I - Y position
		     int W,		// I - Width
		     int H)		// I - Height
{
  Fl_Widget::resize(X, Y, W, H);

  comp_.resize(W, H);

  if (ximage_)
  {
    XShmDetach(fl_display, &xshminfo_);
    XDestroyImage(ximage_);
    ximage_ = (XImage *)0;
  }
}


//
// 'Fl_Slideshow::start()' - Start the slideshow.
//

void
Fl_Slideshow::start()
{
  if (running_)
    return;

  if (count_)
  {
    timeout_ = delay_ + fade_;
    swapped_ = true;

    Fl::add_timeout(rate_, timer_cb, this);

    running_ = true;

    do_callback();
  }
}


//
// 'Fl_Slideshow::stop()' - Stop the slideshow.
//

void
Fl_Slideshow::stop()
{
  if (running_)
  {
    Fl::remove_timeout(timer_cb, this);

    running_ = false;

    do_callback();
  }
}

//
// 'Fl_Slideshow::timer_cb()' - Update the slideshow.
//

void
Fl_Slideshow::timer_cb(void *d)		// I - Callback data
{
  Fl_Slideshow	*ss;			// Slideshow widget


  ss = (Fl_Slideshow *)d;

  if (!ss->count())
    return;

  ss->redraw();

  ss->timeout_ -= ss->rate_;

  if (ss->timeout_ < 0.0f)
  {
    ss->timeout_ = ss->delay_ + ss->fade_;

    ss->current_ += ss->step_;
    if (ss->current_ >= ss->count_)
    {
      ss->current_ = 0;

      if (!ss->repeat_)
      {
        ss->stop();
	return;
      }
    }
    else if (ss->current_ < 0)
    {
      ss->current_ = ss->count_ - 1;

      if (!ss->repeat_)
      {
        ss->stop();
	return;
      }
    }
  }

  Fl::repeat_timeout(ss->rate_, timer_cb, d);
}


//
// End of "$Id: Fl_Slideshow.cxx 327 2005-01-25 22:31:11Z easysw $".
//
