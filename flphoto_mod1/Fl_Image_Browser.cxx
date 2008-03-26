//
// "$Id: Fl_Image_Browser.cxx 440 2006-12-08 04:40:27Z mike $"
//
// Image browser widget methods for the Fast Light Tool Kit (FLTK).
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
//   Fl_Image_Browser::Fl_Image_Browser()     - Create a new image display widget.
//   Fl_Image_Browser::~Fl_Image_Browser()    - Destroy an image display widget.
//   Fl_Image_Browser::draw()                 - Draw the image display widget.
//   Fl_Image_Browser::handle()               - Handle events in the widget.
//   Fl_Image_Browser::resize()               - Resize the image display widget.
//   Fl_Image_Browser::scrollbar_cb()         - Update the display based on the scrollbar position.
//   Fl_Image_Browser::update_scrollbar()     - Update the scrollbar.
//   Fl_Image_Browser::add()                  - Add an image to the browser.
//   Fl_Image_Browser::clear()                - Remove all items from the browser.
//   Fl_Image_Browser::delete_item()          - Delete an item from the browser.
//   Fl_Image_Browser::insert_item()          - Insert an item in the browser.
//   Fl_Image_Browser::move_item()            - Move an image in the browser.
//   Fl_Image_Browser::load()                 - Load all images in a directory.
//   Fl_Image_Browser::load_item()            - Load the image for an item.
//   Fl_Image_Browser::remove()               - Remove an item.
//   Fl_Image_Browser::ITEM::save_thumbnail() - Save the thumbnail image.
//   Fl_Image_Browser::select()               - Select an image.
//

#include "Fl_Image_Browser.H"
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/fl_draw.H>
#include <stdio.h>
#include <stdlib.h>
#include "flstring.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <FL/filename.H>

#if defined(WIN32) && !defined(__CYGWIN__)
#  include <direct.h>
#  include <io.h>
#  define fl_mkdir(p)	mkdir(p)
#else
#  include <unistd.h>
#  define fl_mkdir(p)	mkdir(p, 0777)
#endif // WIN32 && !__CYGWIN__


//
// Scrollbar and item widths...
//

#define SBWIDTH		Fl::scrollbar_size()
#define ITEMWIDTH	100


//
// 'Fl_Image_Browser::Fl_Image_Browser()' - Create a new image display widget.
//

Fl_Image_Browser::Fl_Image_Browser(
    int        X,			// I - X position
    int        Y,			// I - Y position
    int        W,			// I - Width
    int        H,			// I - Height
    const char *L)			// I - Label string
  : Fl_Group(X, Y, W, H, L),
    scrollbar_(X, Y + H - SBWIDTH, W, SBWIDTH)
{
  end();

  box(FL_DOWN_BOX);
  selection_color(FL_SELECTION_COLOR);

  items_       = 0;
  num_items_   = 0;
  alloc_items_ = 0;
  selected_    = -1;
  textfont_    = FL_HELVETICA;
  textsize_    = FL_NORMAL_SIZE;

  scrollbar_.type(FL_HORIZONTAL);
  scrollbar_.callback(scrollbar_cb, this);

  resize(X, Y, W, H);
}


//
// 'Fl_Image_Browser::~Fl_Image_Browser()' - Destroy an image display widget.
//

Fl_Image_Browser::~Fl_Image_Browser()
{
  clear();

  if (items_)
    delete[] items_;
}


//
// 'Fl_Image_Browser::draw()' - Draw the image display widget.
//

void
Fl_Image_Browser::draw()
{
  int	i;				// Looping var
  ITEM	*item;				// Current item
  int	X, Y, W, H;			// Dimensions
  int	xoff;				// X offset


  X = x() + Fl::box_dx(box());
  Y = y() + Fl::box_dy(box());
  W = w() - Fl::box_dw(box());
  H = h() - Fl::box_dh(box()) - SBWIDTH;

  if (damage() & FL_DAMAGE_SCROLL)
    fl_push_clip(X, Y, W, H);

  draw_box(box(), x(), y(), w(), h() - SBWIDTH, color());

  if (!(damage() & FL_DAMAGE_SCROLL))
    fl_push_clip(X, Y, W, H);

#ifdef DEBUG
  printf("scrollbar_.value() = %d\n", scrollbar_.value());
#endif // DEBUG

  for (i = 0; i < num_items_; i ++)
  {
    xoff = i * ITEMWIDTH - scrollbar_.value();

#ifdef DEBUG
    printf("Item %d: xoff = %d...\n", i, xoff);
#endif // DEBUG

    if (xoff < -ITEMWIDTH || xoff >= W)
      continue;

    item = items_[i];

    if (!item)
      continue;

    Fl_Color bg;

    if (item->selected)
      bg = item->changed ?
               fl_color_average(FL_RED, selection_color(), 0.5) :
               selection_color();
    else
      bg = item->changed ? FL_RED : FL_WHITE;

    if (bg != FL_WHITE)
    {
      fl_color(bg);
      fl_rectf(X + xoff + 2, Y + 2, ITEMWIDTH - 4, H - 4);
    }

    if (item->thumbnail)
      item->thumbnail->draw(X + xoff + (ITEMWIDTH - item->thumbnail->w()) / 2,
                            Y + (H - textsize() - item->thumbnail->h()) / 2);

    fl_color(fl_contrast(FL_BLACK, bg));
    fl_font(textfont(), textsize());
    fl_draw(item->label, X + xoff + 2, Y, ITEMWIDTH - 4, H - 2,
            (Fl_Align)(FL_ALIGN_INSIDE | FL_ALIGN_BOTTOM |
	               FL_ALIGN_CLIP | FL_ALIGN_WRAP));
  }

  fl_pop_clip();

  if (damage() & FL_DAMAGE_SCROLL)
    update_child(scrollbar_);
  else
    draw_child(scrollbar_);
}


//
// 'Fl_Image_Browser::handle()' - Handle events in the widget.
//

int					// O - 1 if event handled, 0 otherwise
Fl_Image_Browser::handle(int event)	// I - Event
{
  int	i;				// Looping var
  int	sel;				// Currently selected item
  int	X, Y;				// Mouse position


  X = Fl::event_x() - x() - Fl::box_dx(box());
  Y = Fl::event_y() - y();

  sel = (X + scrollbar_.value()) / ITEMWIDTH;

  if (sel < 0 || sel >= num_items_)
    sel = -1;

  switch (event)
  {
    case FL_PUSH :
        if (Y >= (h() - SBWIDTH))
	  break;

        pushed_ = sel;

	if (sel >= 0)
	{
	  if (Fl::event_state() & FL_CTRL)
	    items_[sel]->selected = !items_[sel]->selected;
	  else if (Fl::event_state() & FL_SHIFT)
	  {
	    if (selected_ < 0)
	      selected_ = 0;

	    if (sel < selected_)
	    {
	      for (i = sel; i <= selected_; i ++)
	        items_[i]->selected = 1;
	    }
	    else
	    {
	      for (i = selected_; i <= sel; i ++)
	        items_[i]->selected = 1;
	    }
	  }
	  else if (!items_[sel]->selected)
	  {
	    // Select item...
	    for (i = 0; i < num_items_; i ++)
	      items_[i]->selected = (i == sel);
	  }

  	  damage(FL_DAMAGE_SCROLL);
	}
	return (1);

    case FL_DRAG :
        if (X < 0)
	{
	  // Scroll left
	  set_scrollbar(scrollbar_.value() - ITEMWIDTH / 10);
	}
	else if (X > w())
	{
	  // Scroll right
	  set_scrollbar(scrollbar_.value() + ITEMWIDTH / 10);
	}

	if (!(Fl::event_state() & (FL_CTRL | FL_SHIFT)) &&
            pushed_ >= 0 && sel != pushed_)
	{
          if (sel > pushed_)
	  {
	    // Move items right...
	    if (items_[sel] && items_[sel]->selected)
	    {
	      // Find the end of the selection...
	      while (sel < num_items_ && items_[sel]->selected)
		sel ++;

              sel ++;
	    }

	    if (sel < num_items_)
	    {
	      for (i = num_items_ - 1; i >= 0; i --)
		if (items_[i] && items_[i]->selected)
	          move_item(i, sel);
	    }
	  }
	  else
	  {
	    // Move items left...
	    for (i = 0; i < num_items_; i ++)
	      if (items_[i] && items_[i]->selected)
	      {
	        move_item(i, sel);
		sel ++;
	      }
	  }

          pushed_ = sel;
	  set_changed();
	  do_callback();
	  clear_changed();
	}

	damage(FL_DAMAGE_SCROLL);
        return (1);

    case FL_RELEASE :
	if (sel < 0)
	{
	  if (!(Fl::event_state() & (FL_CTRL | FL_SHIFT)))
	  {
	    for (i = 0; i < num_items_; i ++)
	      items_[i]->selected = 0;
	  }
	}
	else if (!(Fl::event_state() & (FL_CTRL | FL_SHIFT)) && pushed_ == sel)
	{
	  // Select item...
	  for (i = 0; i < num_items_; i ++)
	    items_[i]->selected = (i == sel);
	}

        selected_ = sel;

	damage(FL_DAMAGE_SCROLL);

        do_callback();
	clear_changed();
	return (1);

    case FL_SHORTCUT :
    case FL_KEYDOWN :
        if (Fl::event_key() == FL_Left && selected_ > 0)
	  selected_ --;
	else if (Fl::event_key() == FL_Right && selected_ < (num_items_ - 1))
	  selected_ ++;
	else
	  break;

        if (Fl::event_state() & FL_SHIFT)
	  items_[selected_]->selected = 1;
	else
	{
	  for (i = 0; i < num_items_; i ++)
	    items_[i]->selected = (i == selected_);
	}

        make_visible(selected_);

        do_callback();
	return (1);

    case FL_FOCUS :
    case FL_UNFOCUS :
        return (1);
  }

  return (Fl_Group::handle(event));
}


//
// 'Fl_Image_Browser::resize()' - Resize the image display widget.
//

void
Fl_Image_Browser::resize(int X,		// I - X position
                         int Y,		// I - Y position
			 int W,		// I - Width
			 int H)		// I - Height
{
  Fl_Widget::resize(X, Y, W, H);

  scrollbar_.resize(X, Y + H - SBWIDTH, W, SBWIDTH);

  update_scrollbar();

  redraw();
}


//
// 'Fl_Image_Browser::scrollbar_cb()' - Update the display based on the scrollbar position.
//

void
Fl_Image_Browser::scrollbar_cb(
    Fl_Widget *,			// I - Widget (not used)
    void      *d)			// I - Image browser
{
  Fl_Image_Browser	*img = (Fl_Image_Browser *)d;


  img->damage(FL_DAMAGE_SCROLL);
}


//
// 'Fl_Image_Browser::set_scrollbar()' - Set the scrollbar position.
//

void
Fl_Image_Browser::set_scrollbar(int X)	// I - New scroll position
{
  int	W;				// Display width


  W = w() - Fl::box_dw(box());

  if (num_items_)
  {
    if ((num_items_ * ITEMWIDTH) <= W)
      X = 0;
    else if (X > (num_items_ * ITEMWIDTH - W))
      X = num_items_ * ITEMWIDTH - W;
    else if (X < 0)
      X = 0;

    scrollbar_.value(X, W, 0, num_items_ * ITEMWIDTH);
    scrollbar_.linesize(W / 2);
  }
  else
    scrollbar_.value(0, 1, 0, 1);

  if ((num_items_ * ITEMWIDTH) <= W)
    scrollbar_.deactivate();
  else
    scrollbar_.activate();
}


//
// 'Fl_Image_Browser::update_scrollbar()' - Update the scrollbar.
//

void
Fl_Image_Browser::update_scrollbar()
{
  set_scrollbar(scrollbar_.value());
}


//
// 'Fl_Image_Browser::add()' - Add an image to the browser.
//

void
Fl_Image_Browser::add(
    const char      *filename,		// I - File to add
    Fl_Shared_Image *img)		// I - Cached image data
{
  struct stat	fileinfo;		// Information about file


	printf("%s \n", filename);
  if (img || (!stat(filename, &fileinfo) && fileinfo.st_size) && find(filename) == -1) 
  {
    // Only add non-empty or cached files! We don't want duplicate files!
    insert_item(filename, img, num_items_);
    update_scrollbar();
    set_changed();
    do_callback();
    clear_changed();
    damage(FL_DAMAGE_SCROLL);
  }
}


//
// 'Fl_Image_Browser::clear()' - Remove all items from the browser.
//

void
Fl_Image_Browser::clear()
{
  while (num_items_ > 0)
    delete_item(0);

  update_scrollbar();
  clear_changed();
  damage(FL_DAMAGE_SCROLL);
}


//
// 'Fl_Image_Browser::delete_item()' - Delete an item from the browser.
//

void
Fl_Image_Browser::delete_item(int i)	// I - Item to delete
{
  ITEM	*item;				// Pointer to item


  if (i < 0 || i >= num_items_)
    return;

  item = items_[i];

  if (item->filename)
    delete[] item->filename;

  if (item->thumbname)
    delete[] item->thumbname;

  if (item->image)
    item->image->release();

  if (item->thumbnail)
    item->thumbnail->release();

  if (item->comments)
    delete[] item->comments;

  delete item;

  num_items_ --;
  if (i < num_items_)
    memmove(items_ + i, items_ + i + 1, (num_items_ - i) * sizeof(ITEM *));
}


//
// 'Fl_Image_Browser::find()' - Find an item in the browser.
//

int					// O - Item number or -1 if none
Fl_Image_Browser::find(
    const char *filename)		// I - File to find
{
  int	i;				// Looping var


  for (i = 0; i < num_items_; i ++)
    if (strcmp(filename, items_[i]->filename) == 0)
      return (i);

  return (-1);
}


//
// 'Fl_Image_Browser::insert_item()' - Insert an item in the browser.
//

Fl_Image_Browser::ITEM *		// O - New item
Fl_Image_Browser::insert_item(
    const char      *f,			// I - Filename
    Fl_Shared_Image *img,		// I - Image
    int             i)			// I - Index
{
  ITEM	*item,				// New item
	**temp;				// New item array
  char	thumbdir[1024],			// Thumbnail directory
	thumbname[1024],		// Thumbnail filename
	*ptr;				// Pointer into filename


  // Verify that the file exists...
  if (access(f, 0))
    return (0);

  // Create a new item...
  item = new ITEM;

  item->filename = new char[strlen(f) + 1];
  strcpy(item->filename, f);

  if ((item->label = strrchr(item->filename, '/')) != NULL)
    item->label ++;
  else
    item->label = item->filename;

  item->image     = img;
  item->thumbnail = 0;
  item->comments  = 0;
  item->changed   = 0;
  item->selected  = 0;

  // Load/create the thumbnail image...
  strlcpy(thumbdir, f, sizeof(thumbdir));

  if ((ptr = strrchr(thumbdir, '/')) != NULL)
    ptr ++;
  else
    ptr = thumbdir;

  strlcpy(ptr, ".xvpics", sizeof(thumbdir) - (ptr - thumbdir));

#ifdef DEBUG
  puts(thumbdir);
#endif // DEBUG

  snprintf(thumbname, sizeof(thumbname), "%s/%s", thumbdir, item->label);

  item->thumbname = new char[strlen(thumbname) + 1];
  strcpy(item->thumbname, thumbname);

#ifdef DEBUG
  puts(thumbname);
#endif // DEBUG

  if (access(thumbname, 0) ||
      (item->thumbnail = Fl_Shared_Image::get(thumbname)) == NULL)
  {
    fl_mkdir(thumbdir);
    item->save_thumbnail();
  }

  // Add to the item array...
  if (i < 0)
    i = 0;
  else if (i > num_items_)
    i = num_items_;

  if (num_items_ >= alloc_items_)
  {
    temp = new ITEM *[alloc_items_ + 10];

    if (items_)
    {
      memcpy(temp, items_, num_items_ * sizeof(ITEM *));

      delete[] items_;
    }

    items_       = temp;
    alloc_items_ += 10;
  }

  if (i < num_items_)
    memmove(items_ + i + 1, items_ + i, (num_items_ - i) * sizeof(ITEM *));

  items_[i] = item;
  num_items_ ++;

  return (item);
}


//
// 'Fl_Image_Browser::move_item()' - Move an image in the browser.
//

void
Fl_Image_Browser::move_item(int from,	// I - From item
                            int to)	// I - To item
{
  ITEM	*temp;				// Temporary item pointer


#ifdef DEBUG
  printf("Fl_Image_Browser::move_item(from=%d, to=%d)\n", from, to);
#endif // DEBUG

  if (from < 0 || from >= num_items_ || to < 0 || to > num_items_ ||
      from == to)
    return;

#ifdef DEBUG
  printf("    num_items_=%d\n", num_items_);
#endif // DEBUG

  temp = items_[from];

  if (to < from)
    memmove(items_ + to + 1, items_ + to, (from - to) * sizeof(ITEM *));
  else
  {
    memmove(items_ + from, items_ + from + 1, (to - from) * sizeof(ITEM *));

    to --;
  }

  items_[to] = temp;
}


//
// 'Fl_Image_Browser::load()' - Load all images in a directory.
//

void 
Fl_Image_Browser::load(const char *dirname)		// I - Directory to load
{
  int		i, j;			// Looping vars
  int		num_files;		// Number of files in directory
  dirent	**files;		// Files in directory
  char		absdir[1024],		// Absolute directory path
	filename[1024];		// Absolute filename path
  int		X, W;			// Position and width of scrollbar


  fl_filename_absolute(absdir, sizeof(absdir), dirname);

  num_files = fl_filename_list(dirname, &files);

  if (num_files > 0)
  {
    window()->cursor(FL_CURSOR_WAIT);

    W = w() - Fl::box_dw(box());

    for (i = 0; i < num_files; i ++)
    {
      snprintf(filename, sizeof(filename), "%s/%s", absdir, files[i]->d_name);

      for (j = 0; j < num_items_; j ++)
#if defined(WIN32) || defined(__EMX__) || defined(__APPLE__)
        if (!strcasecmp(items_[j]->filename, filename))
	  break;
#else
        if (!strcmp(items_[j]->filename, filename))
	  break;
#endif // WIN32 || __EMX__ || __APPLE__

      // Import all supported file formats *except* PPM to avoid cached
      // raw image files...
      //if (!fl_filename_isdir(filename) && j >= num_items_ &&
          //fl_filename_match(files[i]->d_name,
	                    //"*.{arw,avi,bay,bmp,bmq,cr2,crw,cs1,dc2,dcr,dng,"
			    //"erf,fff,hdr,jpg,k25,kdc,mdc,mos,nef,orf,pcd,pef,"
			    //"png,pxn,raf,raw,rdc,sr2,srf,sti,tif,x3f}"))
			if (!fl_filename_isdir(filename) && j >= num_items_ &&
					fl_filename_match(files[i]->d_name,
											"*.{bmp,jpg,png,raw,tif}"))
      {
        if (window()->shown())
				{
					int xx, yy;
					int ww, hh;

					window()->make_current();

					fl_font(textfont(), textsize());

					ww = w() / 2;
					hh = textsize() + 20;
					xx = x() + w() / 4;
					yy = y() + (h() - SBWIDTH - hh) / 2;

					if (i > 0)
					{
						fl_push_clip(xx, yy, ww * i / (num_files - 1), hh);
						draw_box(FL_UP_BOX, xx, yy, ww, hh, selection_color());
						fl_color(fl_contrast(FL_BLACK, selection_color()));
						fl_draw(files[i]->d_name, xx, yy, ww, hh, FL_ALIGN_CENTER);
						fl_pop_clip();
					}

					if (i < (num_files - 1))
					{
						fl_push_clip(xx + ww * i / (num_files - 1), yy,
						 ww - ww * i / (num_files - 1), hh);
						draw_box(FL_UP_BOX, xx, yy, ww, hh, color());
						fl_color(fl_contrast(FL_BLACK, color()));
						fl_draw(files[i]->d_name, xx, yy, ww, hh, FL_ALIGN_CENTER);
						fl_pop_clip();
					}

					Fl::flush();
				}

				add(filename);

							X = num_items_ * ITEMWIDTH - W;
				if (X < 0)
					X = 0;

				scrollbar_.value(X, W, 0, num_items_ * ITEMWIDTH);

				Fl::check();
      }

      free(files[i]);
    }

    free(files);

    window()->cursor(FL_CURSOR_DEFAULT);
  }
}


//
// 'Fl_Image_Browser::load_item()' - Load the image for an item.
//

Fl_Shared_Image *			// O - Image
Fl_Image_Browser::load_item(int i)	// I - Index
{
  ITEM	*item;				// Item


  if (i < 0 || i >= num_items_)
    return (0);

  item = items_[i];

  if (!item->image)
    item->image = Fl_Shared_Image::get(item->filename);

  return (item->image);
}


//
// 'Fl_Image_Browser::remove()' - Remove an item.
//

void
Fl_Image_Browser::remove(int i)		// I - Index to remove
{
  delete_item(i);
  update_scrollbar();
  redraw();
}


//
// 'Fl_Image_Browser::ITEM::make_thumbnail()' - Make the thumbnail image.
//

void
Fl_Image_Browser::ITEM::make_thumbnail()
{
  int		releaseit;		// Need to release image?
  int		X, Y, W, H;		// Dimensions
  uchar		*rgb;			// Pointer to image data
  int		r, g, b;		// Current RGB color
  int		rp, gp, bp;		// Palette RGB color
  int		error;			// Error value
  int		errors[2][3][81],	// Dithering errors
		current,		// Current pixel
		next;			// Next pixel


  // Clear the thumbnail image as needed...
  if (thumbnail)
  {
    thumbnail->release();
    thumbnail = 0;
  }

  // Create the thumbnail image...
  if (!image)
  {
    releaseit = 1;
    image     = Fl_Shared_Image::get(filename);
  }
  else
    releaseit = 0;

  if (image && image->w() && image->h())
  {
    // Size the thumbnail within an 80x80 box...
    W = 80;
    H = W * image->h() / image->w();

    if (H > 80)
    {
      H = 80;
      W = H * image->w() / image->h();
    }

    // Dither the image data to a 3:3:2 RGB representation using
    // a Floyd-Steinberg error-diffusion dither.  Don't bother
    // with a serpentine scan since the image is too small to make
    // a difference...
    thumbnail = (Fl_Shared_Image *)image->copy(W, H);
    rgb       = (uchar *)thumbnail->data()[0];

    memset(errors[0], 0, sizeof(errors[0]));

    for (Y = 0; Y < H; Y ++)
    {
      current = Y & 1;
      next    = 1 - current;

      memset(errors[next], 0, sizeof(errors[0]));

      for (X = 0; X < W; X ++, rgb += 3)
      {
	// Dither red...
	r = rgb[0] + errors[current][0][X] / 16;
	if (r > 255)
	  r = 255;
	else if (r < 0)
	  r = 0;

        rp     = 7 * r / 255;
	rgb[0] = 255 * rp / 7;
	error  = r - rgb[0];

        errors[current][0][X + 1] += 7 * error;
	if (X)
          errors[next][0][X - 1]  += 3 * error;
        errors[next][0][X]        += 5 * error;
        errors[next][0][X + 1]    += error;

	// Dither green...
	g = rgb[1] + errors[current][1][X] / 16;
	if (g > 255)
	  g = 255;
	else if (g < 0)
	  g = 0;

        gp     = 7 * g / 255;
	rgb[1] = 255 * gp / 7;
	error  = g - rgb[1];

        errors[current][1][X + 1] += 7 * error;
	if (X)
          errors[next][1][X - 1]  += 3 * error;
        errors[next][1][X]        += 5 * error;
        errors[next][1][X + 1]    += error;

        // Dither blue...
	b = rgb[2] + errors[current][2][X] / 16;
	if (b > 255)
	  b = 255;
	else if (b < 0)
	  b = 0;

        bp     = 3 * b / 255;
	rgb[2] = 255 * bp / 3;
	error  = b - rgb[2];

        errors[current][2][X + 1] += 7 * error;
	if (X)
          errors[next][2][X - 1]  += 3 * error;
        errors[next][2][X]        += 5 * error;
        errors[next][2][X + 1]    += error;
      }
    }
  }

  if (image && releaseit)
  {
    image->release();
    image = 0;
  }
}


//
// 'Fl_Image_Browser::ITEM::save_thumbnail()' - Save the thumbnail image.
//

void
Fl_Image_Browser::ITEM::save_thumbnail(
    int createit)			// I - 1 = create thumbnail image
{
  int		X, Y, W, H;		// Dimensions
  uchar		*rgb;			// Image data
  int		r, g, b;		// Current color
  FILE		*thumbfile;		// Thumbnail file


  // Create the thumbnail image as needed...
  if (createit || !thumbnail)
    make_thumbnail();

  if (!thumbnail)
    return;

  W = thumbnail->w();
  H = thumbnail->h();

  // Save the thumbnail image...
  if ((thumbfile = fopen(thumbname, "wb")) != NULL)
  {
    fprintf(thumbfile, "P7 332\n%d %d 255\n", W, H);

    rgb = (uchar *)thumbnail->data()[0];
    for (Y = 0; Y < H; Y ++)
      for (X = 0; X < W; X ++)
      {
	r = *rgb++ >> 5;
	g = *rgb++ >> 5;
	b = *rgb++ >> 6;

        putc((((r << 3) | g) << 2) | b, thumbfile);
      }

    fclose(thumbfile);
  }
}


//
// 'Fl_Image_Browser::select()' - Select an image.
//

void
Fl_Image_Browser::select(int i)		// I - Index
{
  int	j;				// Looping var


  if (i < 0 || i >= num_items_)
    return;

  selected_ = i;

  for (j = 0; j < num_items_; j ++)
    items_[j]->selected = 0;

  items_[i]->selected = 1;

  make_visible(i);
}


//
// 'Fl_Image_Browser::make_visible()' - Make an image visible.
//

void
Fl_Image_Browser::make_visible(int i)	// I - Index
{
  int	X;				// X position


  if (i < 0 || i >= num_items_)
    return;

  X = i * ITEMWIDTH;

  if (X < scrollbar_.value() ||
      X >= (scrollbar_.value() + w() - Fl::box_dw(box())))
    set_scrollbar(X);

  damage(FL_DAMAGE_SCROLL);
}



//
// End of "$Id: Fl_Image_Browser.cxx 440 2006-12-08 04:40:27Z mike $".
//
