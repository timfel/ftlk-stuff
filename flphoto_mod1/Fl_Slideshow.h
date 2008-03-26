//
// "$Id: Fl_Slideshow.h 326 2005-01-25 07:29:17Z easysw $"
//
// Slideshow display class definitions for flPhoto.
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

//
// Include necessary headers...
//

#include <FL/Fl.H>
#include <FL/Fl_Widget.H>
#include <FL/Fl_Shared_Image.H>
#include "Compositor.h"

#include <FL/x.H>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>


//
// Slide position constants...
//

enum
{
  FL_SLIDESHOW_DEFAULT = -1,
  FL_SLIDESHOW_CENTER = 0,
  FL_SLIDESHOW_TOP = 1,
  FL_SLIDESHOW_BOTTOM = 2,
  FL_SLIDESHOW_LEFT = 4,
  FL_SLIDESHOW_RIGHT = 8,
  FL_SLIDESHOW_TOP_LEFT = 5,
  FL_SLIDESHOW_TOP_RIGHT = 9,
  FL_SLIDESHOW_BOTTOM_LEFT = 6,
  FL_SLIDESHOW_BOTTOM_RIGHT = 10,
  FL_SLIDESHOW_RANDOM = 15,
  FL_SLIDESHOW_PAN = 16
};


//
// Slideshow file...
//

struct Fl_SlideFile
{
  char		*filename;		// Filename
  char		*comment;		// Comment string
  int		position;		// Position
  float		zoom[2];		// Start and end zoom

  Fl_SlideFile(const char *f, const char *c, int p, float start, float end);
  ~Fl_SlideFile();
};


//
// Slideshow image...
//

struct Fl_SlideImage
{
  const char	*comment;		// Comment string
  Fl_Shared_Image *image;		// Original imae
  int		position;		// Position
  float		zoom[2];		// Start and end zoom
};


//
// Slideshow widget...
//

class Fl_Slideshow : public Fl_Widget
{
  int		alloc_;			// Allocated images
  bool		alternate_;		// Alternate zooms?
  Compositor	comp_;			// Compositor
  int		count_;			// Number of images
  int		current_;		// Current image
  float		delay_;			// Default delay between images
  float		fade_;			// Default fade time between images
  Fl_SlideFile	**files_;		// Image files
  Fl_SlideImage	image_;			// Current image data
  int		position_;		// Default position of images
  float		rate_;			// Update rate
  bool		repeat_;		// Repeat the slideshow?
  bool		running_;		// True if running
  int		step_;			// Direction of slideshow
  bool		swapped_;		// Swapped old image out?
  Fl_Align	textalign_;		// Alignment of comment text
  Fl_Color	textcolor_;		// Color of comment text
  uchar		textfont_;		// Typeface of comment text
  uchar		textsize_;		// Size of comment text
  float		timeout_;		// Timeout until next image
  XImage	*ximage_;		// X image
  XShmSegmentInfo xshminfo_;		// X shared memory info
  unsigned	xshmsize_;		// Size of X shared memory segment
  float		zoom_[2];		// Default start/end zoom

  void		draw();
  static void	timer_cb(void *d);

  public:

		Fl_Slideshow(int X, int Y, int W, int H, const char *L = 0);
		~Fl_Slideshow();

  void		add(const char *filename, const char *comment = (const char *)0,
		    int p = FL_SLIDESHOW_DEFAULT,
		    float start = FL_SLIDESHOW_DEFAULT,
		    float end = FL_SLIDESHOW_DEFAULT);
  void		clear();
  const char	*comment(int f) { return (files_[f]->comment); }
  int		count() const { return (count_); }
  void		current(int c) { current_ = c; }
  int		current() const { return (current_); }
  void		delay(float d) { delay_ = d; }
  float		delay() const { return (delay_); }
  void		fade(float f) { fade_ = f; }
  float		fade() const { return (fade_); }
  const char	*filename(int f) { return (files_[f]->filename); }
  int		handle(int event);
  void		pan(bool p) { if (p) position_ |= FL_SLIDESHOW_PAN; else position_ &= ~FL_SLIDESHOW_PAN; }
  bool		pan() const { return ((position_ & FL_SLIDESHOW_PAN) != 0); }
  void		position(int p) { position_ = (position_ & FL_SLIDESHOW_PAN) | p; }
  int		position() const { return (position_); }
  void		rate(float r) { rate_ = r; }
  float		rate() const { return (rate_); }
  void		repeat(bool r) { repeat_ = r; }
  bool		repeat() const { return (repeat_); }
  void		resize(int X, int Y, int W, int H);
  bool		running() const { return (running_); }
  void		start();
  void		step(int s) { step_ = s; }
  int		step() const { return (step_); }
  void		stop();
  void		textalign(Fl_Align a) { textalign_ = a; }
  Fl_Align	textalign() const { return (textalign_); }
  void		textcolor(Fl_Color c) { textcolor_ = c; }
  Fl_Color	textcolor() const { return (textcolor_); }
  void		textfont(uchar f) { textfont_ = f; }
  uchar		textfont() const { return (textfont_); }
  void		textsize(uchar s) { textsize_ = s; }
  uchar		textsize() const { return (textsize_); }
  float		timeout() const { return (timeout_); }
  void		zoom(float start, float end, bool a = true) { zoom_[0] = start; zoom_[1] = end; alternate_ = a;}
};


//
// End of "$Id: Fl_Slideshow.h 326 2005-01-25 07:29:17Z easysw $".
//
