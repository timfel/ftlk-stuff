//
// "$Id: testslideshow.cxx 326 2005-01-25 07:29:17Z easysw $"
//
// Compositor/slideshow test program for flPhoto.
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

#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Button.H>
#include "Fl_Slideshow.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


//
// Local functions...
//

void	start_stop_cb(Fl_Widget *w, void *d);
void	usage();


//
// 'main()' - Main entry for test program.
//

int					// O - Exit status
main(int  argc,				// I - Number of command-line args
     char *argv[])			// I - Command-line arguments
{
  int			i;		// Looping var
  Fl_Double_Window	*win;		// Window
  Fl_Button		*button;	// Start/stop button
  Fl_Slideshow		*ss;		// Slideshow


  fl_register_images();

  win = new Fl_Double_Window(320, 285, "Slideshow Test");
  ss  = new Fl_Slideshow(0, 0, 320, 240);
  ss->callback(start_stop_cb, ss);

  for (i = 1; i < argc; i ++)
  {
    if (!strcmp(argv[i], "--bottom"))
      ss->position(FL_SLIDESHOW_BOTTOM);
    else if (!strcmp(argv[i], "--bottom-left"))
      ss->position(FL_SLIDESHOW_BOTTOM_LEFT);
    else if (!strcmp(argv[i], "--bottom-right"))
      ss->position(FL_SLIDESHOW_BOTTOM_RIGHT);
    else if (!strcmp(argv[i], "--center"))
      ss->position(FL_SLIDESHOW_CENTER);
    else if (!strcmp(argv[i], "--delay"))
    {
      i ++;
      if (i < argc)
        ss->delay(atof(argv[i]));
      else
        usage();
    }
    else if (!strcmp(argv[i], "--fade"))
    {
      i ++;
      if (i < argc)
        ss->fade(atof(argv[i]));
      else
        usage();
    }
    else if (!strcmp(argv[i], "--left"))
      ss->position(FL_SLIDESHOW_LEFT);
    else if (!strcmp(argv[i], "--no-pan"))
      ss->pan(false);
    else if (!strcmp(argv[i], "--no-repeat"))
      ss->repeat(false);
    else if (!strcmp(argv[i], "--pan"))
      ss->pan(true);
    else if (!strcmp(argv[i], "--random"))
      ss->position(FL_SLIDESHOW_RANDOM);
    else if (!strcmp(argv[i], "--rate"))
    {
      i ++;
      if (i < argc)
        ss->rate(atof(argv[i]));
      else
        usage();
    }
    else if (!strcmp(argv[i], "--repeat"))
      ss->repeat(true);
    else if (!strcmp(argv[i], "--right"))
      ss->position(FL_SLIDESHOW_RIGHT);
    else if (!strcmp(argv[i], "--top"))
      ss->position(FL_SLIDESHOW_TOP);
    else if (!strcmp(argv[i], "--top-left"))
      ss->position(FL_SLIDESHOW_TOP_LEFT);
    else if (!strcmp(argv[i], "--top-right"))
      ss->position(FL_SLIDESHOW_TOP_RIGHT);
    else if (!strcmp(argv[i], "--zoom"))
    {
      i ++;
      if (i < argc)
      {
        float start, end;

        switch (sscanf(argv[i], "%f,%f", &start, &end))
	{
	  case 1 :
	      end = start;
	  case 2 :
	      break;
	  default :
	      usage();
	}

        ss->zoom(start, end);
      }
      else
        usage();
    }
    else if (argv[i][0] == '-')
      usage();
    else
      ss->add(argv[i]);
  }

  button = new Fl_Button(10, 250, 300, 25, "Start Slideshow");
  button->callback(start_stop_cb, ss);

  if (ss->count() < 2)
    usage();

  win->resizable(ss);
  win->end();
  win->show(1, argv);

  Fl::run();

  delete win;

  return (0);
}


//
// 'start_stop_cb()' - Toggle slideshow on/off.
//

void
start_stop_cb(Fl_Widget *w,		// I - Widget
              void      *d)		// I - Pointer to slideshow widget
{
  Fl_Slideshow	*ss;			// Slideshow


  ss = (Fl_Slideshow *)d;

  if ((void *)w != d)
  {
    if (ss->running())
      ss->stop();
    else
      ss->start();
  }

  if (ss->running())
    w->label("Stop Slideshow");
  else
    w->label("Start Slideshow");
}


//
// 'usage()' - Show program usage.
//

void
usage()
{
  puts("Usage: testslideshow [options] file1 file2 [... fileN]");
  puts("Options:");
  puts("    --bottom");
  puts("    --bottom-left");
  puts("    --bottom-right");
  puts("    --center");
  puts("    --delay seconds");
  puts("    --fade seconds");
  puts("    --left");
  puts("    --no-pan");
  puts("    --no-repeat");
  puts("    --pan");
  puts("    --random");
  puts("    --rate seconds");
  puts("    --repeat");
  puts("    --right");
  puts("    --top");
  puts("    --top-left");
  puts("    --top-right");
  puts("    --zoom start,end");

  exit(1);
}


//
// End of "$Id: testslideshow.cxx 326 2005-01-25 07:29:17Z easysw $".
//
