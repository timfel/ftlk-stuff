//
// "$Id: gl_draw.cxx 5190 2006-06-09 16:16:34Z mike $"
//
// OpenGL drawing support routines for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2005 by Bill Spitzak and others.
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA.
//
// Please report all bugs and problems on the following page:
//
//     http://www.fltk.org/str.php
//

// Functions from <FL/gl.h>
// See also Fl_Gl_Window and gl_start.cxx

#include "flstring.h"
#if HAVE_GL

#include <FL/Fl.H>
#include <FL/gl.h>
#include <FL/x.H>
#include <FL/fl_draw.H>
#include "Fl_Gl_Choice.H"
#include "Fl_Font.H"
#include <FL/fl_utf8.H>

#if !defined(WIN32) && !defined(__APPLE__)
#include <FL/Xutf8.h>
#endif

#if USE_XFT
extern XFontStruct* fl_xxfont();
#endif // USE_XFT

int   gl_height() {return fl_height();}
int   gl_descent() {return fl_descent();}
double gl_width(const char* s) {return fl_width(s);}
double gl_width(const char* s, int n) {return fl_width(s,n);}
double gl_width(uchar c) {return fl_width(c);}

static Fl_FontSize *gl_fontsize;

void  gl_font(int fontid, int size) {
  fl_font(fontid, size);
  if (!fl_fontsize->listbase) {
#ifdef WIN32
    int base = fl_fontsize->metr.tmFirstChar;
    int count = fl_fontsize->metr.tmLastChar-base+1;
    HFONT oldFid = (HFONT)SelectObject(fl_gc, fl_fontsize->fid);
    fl_fontsize->listbase = glGenLists(256);
    wglUseFontBitmaps(fl_gc, base, count, fl_fontsize->listbase+base);
    SelectObject(fl_gc, oldFid);
#elif defined(__APPLE_QD__)
    // undefined characters automatically receive an empty GL list in aglUseFont
    fl_fontsize->listbase = glGenLists(256);
    aglUseFont(aglGetCurrentContext(), fl_fontsize->font, fl_fontsize->face,
               fl_fontsize->size, 0, 256, fl_fontsize->listbase);
#elif defined(__APPLE_QUARTZ__)
    short font, face, size;
    uchar fn[256];
    fn[0]=strlen(fl_fontsize->q_name);
    strcpy((char*)(fn+1), fl_fontsize->q_name);
    GetFNum(fn, &font);
    face = 0;
    size = fl_fontsize->size;
    fl_fontsize->listbase = glGenLists(256);
    aglUseFont(aglGetCurrentContext(), font, face,
               size, 0, 256, fl_fontsize->listbase);
#else
#  if USE_XFT
#warning We really need a glXUseXftFont implementation here...
//    fl_xfont = fl_xxfont();
    XFontStruct *font = fl_xxfont();
    int base = font->min_char_or_byte2;
    int count = font->max_char_or_byte2-base+1;
    fl_fontsize->listbase = glGenLists(256);
    glXUseXFont(font->fid, base, count, fl_fontsize->listbase+base);
#  endif // USE_XFT
//    XFontStruct *font = NULL;
//    unsigned short id;
//    XGetUtf8FontAndGlyph(gl_fontsize->font, ii, &font, &id);
//    if (font) glXUseXFont(font->fid, id, 1, gl_fontsize->listbase+ii);
#endif
  }
  gl_fontsize = fl_fontsize;
  glListBase(fl_fontsize->listbase);
}

#ifndef __APPLE__
// The OSX build does not use this at present... It probbaly should, though...
static void get_list(int r) {
  gl_fontsize->glok[r] = 1;
#ifdef WIN32
  unsigned int ii = r * 0x400;
  HFONT oldFid = (HFONT)SelectObject(fl_gc, gl_fontsize->fid);
  wglUseFontBitmapsW(fl_gc, ii, ii + 0x03ff, gl_fontsize->listbase+ii);
  SelectObject(fl_gc, oldFid);
#elif defined(__APPLE_QD__)
  unsigned int ii = r * 0x400;
    aglUseFont(aglGetCurrentContext(), gl_fontsize->font, gl_fontsize->face,
               gl_fontsize->size, ii, 0x03ff, fl_fontsize->listbase+ii);
#elif defined(__APPLE_QUARTZ__)
// FIXME
    short font, face, size;
    uchar fn[256];
    fn[0]=strlen(fl_fontsize->q_name);
    strcpy((char*)(fn+1), fl_fontsize->q_name);
    GetFNum(fn, &font);
    face = 0;
    size = fl_fontsize->size;
    fl_fontsize->listbase = glGenLists(256);
    aglUseFont(aglGetCurrentContext(), font, face,
               size, 0, 256, fl_fontsize->listbase);
#elif USE_XFT
    XFontStruct *font = fl_xxfont();
    int base = font->min_char_or_byte2;
    int count = font->max_char_or_byte2-base+1;
    fl_fontsize->listbase = glGenLists(256);
    glXUseXFont(font->fid, base, count, fl_fontsize->listbase+base);
#else
  for (int i = 0; i < 0x400; i++) {
    XFontStruct *font = NULL;
    unsigned short id;
    XGetUtf8FontAndGlyph(gl_fontsize->font, ii, &font, &id);
    if (font) glXUseXFont(font->fid, id, 1, gl_fontsize->listbase+ii);
    ii++;
   }
#endif
} // get_list
#endif

void gl_remove_displaylist_fonts()
{
# if HAVE_GL

  // clear variables used mostly in fl_font
  fl_font_ = 0;
  fl_size_ = 0;

  for (int j = 0 ; j < FL_FREE_FONT ; ++j)
  {
    Fl_FontSize* past = 0;
    Fl_Fontdesc* s    = fl_fonts + j ;
    Fl_FontSize* f    = s->first;
    while (f != 0) {
      if(f->listbase) {
        if(f == s->first) {
          s->first = f->next;
        }
        else {
          past->next = f->next;
        }

        // It would be nice if this next line was in a desctructor somewhere
        glDeleteLists(f->listbase, 256);

        Fl_FontSize* tmp = f;
        f = f->next;
        delete tmp;
      }
      else {
        past = f;
        f = f->next;
      }
    }
  }

#endif
}

void gl_draw(const char* str, int n) {
#ifdef __APPLE__
// Should be converting the text here, as for other platforms???
  glCallLists(n, GL_UNSIGNED_BYTE, str);
#else
  static xchar *buf = NULL;
  static int l = 0;
//  if (n > l) {
//    buf = (xchar*) realloc(buf, sizeof(xchar) * (n + 20));
//    l = n + 20;
//  }
//  n = fl_utf2unicode((const unsigned char*)str, n, buf);

  int wn = fl_utf8toUtf16(str, n, (unsigned short*)buf, l);
  if(wn >= l) {
    buf = (xchar*) realloc(buf, sizeof(xchar) * (wn + 1));
    l = wn + 1;
    wn = fl_utf8toUtf16(str, n, (unsigned short*)buf, l);
  }
  n = wn;

  int i;
  for (i = 0; i < n; i++) {
    unsigned int r;
    r = (str[i] & 0xFC00) >> 10;
    if (!gl_fontsize->glok[r]) get_list(r);
  }
  glCallLists(n, GL_UNSIGNED_SHORT, buf);
#endif
}

void gl_draw(const char* str, int n, int x, int y) {
  glRasterPos2i(x, y);
  gl_draw(str, n);
}

void gl_draw(const char* str, int n, float x, float y) {
  glRasterPos2f(x, y);
  gl_draw(str, n);
}

void gl_draw(const char* str) {
  gl_draw(str, strlen(str));
}

void gl_draw(const char* str, int x, int y) {
  gl_draw(str, strlen(str), x, y);
}

void gl_draw(const char* str, float x, float y) {
  gl_draw(str, strlen(str), x, y);
}

static void gl_draw_invert(const char* str, int n, int x, int y) {
  glRasterPos2i(x, -y);
  gl_draw(str, n);
}

void gl_draw(
  const char* str, 	// the (multi-line) string
  int x, int y, int w, int h, 	// bounding box
  Fl_Align align) {
  fl_draw(str, x, -y-h, w, h, align, gl_draw_invert);
}

void gl_measure(const char* str, int& x, int& y) {fl_measure(str,x,y);}

void gl_rect(int x, int y, int w, int h) {
  if (w < 0) {w = -w; x = x-w;}
  if (h < 0) {h = -h; y = y-h;}
  glBegin(GL_LINE_STRIP);
  glVertex2i(x+w-1, y+h-1);
  glVertex2i(x+w-1, y);
  glVertex2i(x, y);
  glVertex2i(x, y+h-1);
  glVertex2i(x+w, y+h-1);
  glEnd();
}

#if HAVE_GL_OVERLAY
extern uchar fl_overlay;
extern int fl_overlay_depth;
#endif

void gl_color(Fl_Color i) {
#if HAVE_GL_OVERLAY
#ifdef WIN32
  if (fl_overlay && fl_overlay_depth) {
    if (fl_overlay_depth < 8) {
      // only black & white produce the expected colors.  This could
      // be improved by fixing the colormap set in Fl_Gl_Overlay.cxx
      int size = 1<<fl_overlay_depth;
      if (!i) glIndexi(size-2);
      else if (i >= size-2) glIndexi(size-1);
      else glIndexi(i);
    } else {
      glIndexi(i ? i : FL_GRAY_RAMP);
    }
    return;
  }
#else
  if (fl_overlay) {glIndexi(int(fl_xpixel(i))); return;}
#endif
#endif
  uchar red, green, blue;
  Fl::get_color(i, red, green, blue);
  glColor3ub(red, green, blue);
}

void gl_draw_image(const uchar* b, int x, int y, int w, int h, int d, int ld) {
  if (!ld) ld = w*d;
  glPixelStorei(GL_UNPACK_ROW_LENGTH, ld/d);
  glRasterPos2i(x,y);
  glDrawPixels(w,h,d<4?GL_RGB:GL_RGBA,GL_UNSIGNED_BYTE,(const ulong*)b);
}

#endif

//
// End of "$Id: gl_draw.cxx 5190 2006-06-09 16:16:34Z mike $".
//
