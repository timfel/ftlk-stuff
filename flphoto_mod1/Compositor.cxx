//
// "$Id: Compositor.cxx 326 2005-01-25 07:29:17Z easysw $"
//
// Compositor class code for flPhoto.
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

#include "debug.h"
#include "Compositor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


//
// 'Compositor::Compositor()' - Create a compositor.
//

Compositor::Compositor(int W,		// I - Width
                       int H)		// I - Height
{
  rgb(0, 0, 0);

  w_      = W;
  h_      = H;
  pixels_ = new unsigned char[W * H * 3];
}


//
// 'Compositor::~Compositor()' - Destroy a compositor.
//

Compositor::~Compositor()
{
  if (pixels_)
    delete[] pixels_;
}


//
// 'Compositor::image()' - Draw pixels from an image.
//

void
Compositor::image(int           dX,	// I - X position of image
                  int           dY,	// I - Y position of image
		  int           dW,	// I - Destination width of image
		  int           dH,	// I - Destination height of image
		  int           W,	// I - Width of image
		  int           H,	// I - Height of image
		  int           D,	// I - Depth of image
		  int           A,	// I - Alpha of image (0 = transparent)
		  bool          r,	// I - Replace (true) or add (false)
		  unsigned char *p)	// I - Pixels to draw
{
  int		i, j;			// Looping vars
  int		X, Y;			// Image X and Y
  unsigned	pixel;			// Pixel value
  int		xcount,			// X col count
		ycount;			// Y row count
  int		xerr,			// Bresenheim values
		xstep,
		xmod,
		yerr,
		ystep,
		ymod;
  unsigned char	*src,			// Pointer into images
		*dst;			// Pointer into compositor
  unsigned	lut[256];		// Alpha lookup table
  unsigned char	lr, lg, lb;		// Pre-multiplied RGB


  DEBUG_printf(("Compositor::image(dX=%d, dY=%d, dW=%d, dH=%d, W=%d, H=%d, D=%d, A=%d, r=%s, p=%p)\n",
                dX, dY, dW, dH, W, H, D, A, r ? "true" : "false", p));

  // Clear the buffer if replace is true...
  if (r)
  {
    // Use the background RGB color...
    if (rgb_[0] == rgb_[1] && rgb_[1] == rgb_[2])
    {
      // Optimize for grays...
      memset(pixels_, rgb_[0], w_ * h_ * 3);
    }
    else
    {
      // Fill with RGB color...
      for (i = w_ * h_, dst = pixels_; i > 0; i --)
      {
        *dst++ = rgb_[0];
        *dst++ = rgb_[1];
        *dst++ = rgb_[2];
      }
    }
  }

  // Bail out early if alpha == 0 or we have no pixels...
  if (!dW || !dH || !W || !H || !D || !A || !p)
    return;

  // Point at the origin of the image...
  X = 0;
  Y = 0;

  // Adjust X, Y, W, and H to be within the buffer...
  if ((dX + dW) > w_)
    xcount = w_ - dX;
  else
    xcount = dW;

  if (dX < 0)
  {
    xcount += dX;
    X      = -dX * W / dW;
    dX     = 0;
  }

  DEBUG_printf(("    dX=%d, X=%d, xcount=%d\n", dX, X, xcount));

  if ((dY + dH) > h_)
    ycount = h_ - dY;
  else
    ycount = dH;

  if (dY < 0)
  {
    ycount += dY;
    Y      = -dY * H / dH;
    dY     = 0;
  }

  DEBUG_printf(("    dY=%d, Y=%d, ycount=%d\n", dY, Y, ycount));

  // Compute the Bresenheim values...
  xstep = (W / dW) * D;
  xmod  = W % dW;
  xerr  = (X * xmod) % dW;

  DEBUG_printf(("    xstep=%d, xmod=%d, xerr=%d\n", xstep, xmod, xerr));

  ystep = H / dH;
  ymod  = H % dH;
  yerr  = (Y * ymod) % dH;

  DEBUG_printf(("    ystep=%d, ymod=%d, yerr=%d\n", ystep, ymod, yerr));

  // Generate a LUT for the alpha...
  for (i = 0; i < 256; i ++)
    lut[i] = i * A / 255;

  lr = rgb_[0] * (255 - A) / 255;
  lg = rgb_[1] * (255 - A) / 255;
  lb = rgb_[2] * (255 - A) / 255;

  // Then composite the remaining image...
  for (i = 0; i < ycount; i ++)
  {
    src = p + (Y * W + X) * D;
    dst = pixels_ + ((dY + i) * w_ + dX) * 3;

    switch (D)
    {
      default :
	  if (r)
	  {
	    // Just copy the source verbatim...
	    for (j = xcount; j > 0; j --)
	    {
	      pixel = lut[*src];

	      *dst++ = pixel + lr;
	      *dst++ = pixel + lg;
	      *dst++ = pixel + lb;

              src += xstep;
	      xerr += xmod;
	      if (xerr >= dW)
	      {
	        xerr -= dW;
		src += D;
	      }
	    }
	  }
	  else
	  {
	    for (j = xcount; j > 0; j --)
	    {
	      // Mix the source and destination colors, avoiding overflow.
	      pixel = *dst + lut[*src];

	      if (pixel < 255)
	      {
	        *dst++ = pixel;
	        *dst++ = pixel;
	        *dst++ = pixel;
	      }
	      else
	      {
	        *dst++ = 255;
	        *dst++ = 255;
	        *dst++ = 255;
	      }

              src += xstep;
	      xerr += xmod;
	      if (xerr >= dW)
	      {
	        xerr -= dW;
		src += D;
	      }
	    }
	  }
	  break;

      case 2 :
	  if (r)
	  {
	    // Just copy the source verbatim...
	    for (j = xcount / 2; j > 0; j --)
	    {
	      pixel = lut[src[0]] * src[1] / 255;

	      *dst++ = pixel + lr;
	      *dst++ = pixel + lg;
	      *dst++ = pixel + lb;

              src += xstep;
	      xerr += xmod;
	      if (xerr >= dW)
	      {
	        xerr -= dW;
		src += D;
	      }
	    }
	  }
	  else
	  {
	    for (j = xcount / 2; j > 0; j --)
	    {
	      // Mix the source and destination colors, avoiding overflow.
	      pixel = *dst + lut[src[0]] * src[1] / 255;

	      if (pixel < 255)
	      {
	        *dst++ = pixel;
	        *dst++ = pixel;
	        *dst++ = pixel;
	      }
	      else
	      {
	        *dst++ = 255;
	        *dst++ = 255;
	        *dst++ = 255;
	      }

              src += xstep;
	      xerr += xmod;
	      if (xerr >= dW)
	      {
	        xerr -= dW;
		src += D;
	      }
	    }
	  }
	  break;

      case 3 :
	  if (r)
	  {
	    for (j = xcount; j > 0; j --)
	    {
	      *dst++ = lut[src[0]] + lr;
	      *dst++ = lut[src[1]] + lg;
	      *dst++ = lut[src[2]] + lb;

              src += xstep;
	      xerr += xmod;
	      if (xerr >= dW)
	      {
	        xerr -= dW;
		src += D;
	      }
	    }
	  }
	  else
	  {
	    for (j = xcount; j > 0; j --)
	    {
	      // Mix the source and destination colors, avoiding overflow.
	      pixel = *dst + lut[src[0]];

	      if (pixel < 255)
	        *dst++ = pixel;
	      else
	        *dst++ = 255;

	      pixel = *dst + lut[src[1]];

	      if (pixel < 255)
	        *dst++ = pixel;
	      else
	        *dst++ = 255;

	      pixel = *dst + lut[src[2]];

	      if (pixel < 255)
	        *dst++ = pixel;
	      else
	        *dst++ = 255;

              src += xstep;
	      xerr += xmod;
	      if (xerr >= dW)
	      {
	        xerr -= dW;
		src += D;
	      }
	    }
	  }
	  break;

      case 4 :
	  if (r)
	  {
	    // Just copy the source verbatim...
	    for (j = xcount; j > 0; j --)
	    {
	      *dst++ = lut[src[0] * src[3] / 255] + lr;
	      *dst++ = lut[src[1] * src[3] / 255] + lg;
	      *dst++ = lut[src[2] * src[3] / 255] + lb;

              src += xstep;
	      xerr += xmod;
	      if (xerr >= dW)
	      {
	        xerr -= dW;
		src += D;
	      }
	    }
	  }
	  else
	  {
	    for (j = xcount; j > 0; j --, p)
	    {
	      // Mix the source and destination colors, avoiding overflow.
	      pixel = *dst + lut[src[0] * src[3] / 255];

	      if (pixel < 255)
	        *dst++ = pixel;
	      else
	        *dst++ = 255;

	      pixel = *dst + lut[src[1] * src[3] / 255];

	      if (pixel < 255)
	        *dst++ = pixel;
	      else
	        *dst++ = 255;

	      pixel = *dst + lut[src[2] * src[3] / 255];

	      if (pixel < 255)
	        *dst++ = pixel;
	      else
	        *dst++ = 255;

              src += xstep;
	      xerr += xmod;
	      if (xerr >= dW)
	      {
	        xerr -= dW;
		src += D;
	      }
	    }
	  }
	  break;
    }

    Y += ystep;
    yerr += ymod;
    if (yerr >= dH)
    {
      yerr -= dH;
      Y ++;
    }
  }
}


//
// 'Compositor::resize()' - Resize a compositor.
//

void
Compositor::resize(int W,		// I - Width
                   int H)		// I - Height
{
  if (pixels_)
    delete[] pixels_;

  w_      = W;
  h_      = H;
  pixels_ = new unsigned char[W * H * 3];
}


//
// End of "$Id: Compositor.cxx 326 2005-01-25 07:29:17Z easysw $".
//
