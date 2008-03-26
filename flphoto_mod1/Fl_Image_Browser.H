//
// "$Id: Fl_Image_Browser.H 401 2006-11-11 03:19:07Z mike $"
//
// Image browser widget header file for the Fast Light Tool Kit (FLTK).
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

#ifndef Fl_Image_Browser_H
#  define Fl_Image_Browser_H
#  include <FL/Fl_Group.H>
#  include <FL/Fl_Scrollbar.H>
#  include <FL/Fl_Shared_Image.H>


class FL_EXPORT Fl_Image_Browser : public Fl_Group
{
  public:

  struct ITEM
  {
    char		*filename;
    char		*thumbname;
    char		*label;
    char		*comments;
    Fl_Shared_Image	*image;
    Fl_Shared_Image	*thumbnail;
    int			changed;
    int			selected;

    void make_thumbnail();
    void save_thumbnail(int createit = 0);
  };

  private:

  ITEM		**items_;
  int		num_items_;
  int		alloc_items_;
  int		pushed_;
  int		selected_;
  uchar		textfont_;
  uchar		textsize_;
  Fl_Scrollbar	scrollbar_;

  void		delete_item(int i);
  ITEM		*insert_item(const char *f, Fl_Shared_Image *img, int i);
  void		move_item(int from, int to);
  static void	scrollbar_cb(Fl_Widget *w, void *d);
  void		set_scrollbar(int X);
  void		update_scrollbar();

  protected:

  void		draw();

  public:

  Fl_Image_Browser(int X, int Y, int W, int H, const char *L = 0);
  ~Fl_Image_Browser();

  void		add(const char *filename, Fl_Shared_Image *img = 0);
  void		clear();
  int		count() const { return num_items_; }
  int		find(const char *filename);
  int		handle(int event);
  void		load(const char *dirname);
  Fl_Shared_Image *load_item(int i);
  void		make_visible(int i);
  void		move(int from, int to) { move_item(from, to); make_visible(to); }
  void		remove(int i);
  void		resize(int X, int Y, int W, int H);
  void		select(int i);
  int		selected() const { return selected_; }
  int		selected(int i) { if (i < 0 || i >= num_items_) return 0; else return items_[i]->selected; }
  uchar		textfont() const { return textfont_; }
  void		textfont(uchar f) { textfont_ = f; }
  uchar		textsize() const { return textsize_; }
  void		textsize(uchar f) { textsize_ = f; }
  ITEM		*value(int i) { if (i < 0 || i >= num_items_) return 0; else return items_[i]; }
};


#endif // !Fl_Image_Browser_H

//
// End of "$Id: Fl_Image_Browser.H 401 2006-11-11 03:19:07Z mike $".
//
