/*
 * "$Id: pcd-inter.c 321 2005-01-23 03:52:44Z easysw $"
 *
 * PhotoCD library interlace routines.
 *
 * Copyright 2002-2005 by Michael Sweet.
 * Original code by Gerd Knorr, no copyright information available.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <stdio.h>
#include <stdlib.h>
#include "flstring.h"

#include "pcd.h"

int
pcd_inter_m2(struct PCD_IMAGE *img)
{
    register unsigned char *src, *dest;
    register int    x;
    int             y;
    int             left = img->left >> (img->res - 3);
    int             top = img->top >> (img->res - 3);
    int             width = img->width >> (img->res - 3);
    int             height = img->height >> (img->res - 3);

    src = img->mmap + pcd_img_start[3] +
	(pcd_def_width[3] >> 1) * 3 * top;

    for (y = 0; y < height; y += 2) {
	/* luma */
	src += left;
	for (dest = img->luma[y << 1], x = 0; x < width - 1; x++) {
	    *(dest++) = src[x], *(dest++) = (src[x] + src[x + 1] + 1) >> 1;
	}
	*(dest++) = src[x], *(dest++) = src[x];
	src += pcd_def_width[3];
	for (dest = img->luma[(y << 1) + 2], x = 0; x < width - 1; x++) {
	    *(dest++) = src[x], *(dest++) = (src[x] + src[x + 1] + 1) >> 1;
	}
	*(dest++) = src[x], *(dest++) = src[x];
	src += pcd_def_width[3] - left;

	/* chroma */
	src += left >> 1;
	for (dest = img->blue[y], x = 0; x < (width >> 1) - 1; x++) {
	    *(dest++) = src[x], *(dest++) = (src[x] + src[x + 1] + 1) >> 1;
	}
	*(dest++) = src[x], *(dest++) = src[x];
	src += pcd_def_width[3] >> 1;
	for (dest = img->red[y], x = 0; x < (width >> 1) - 1; x++) {
	    *(dest++) = src[x], *(dest++) = (src[x] + src[x + 1] + 1) >> 1;
	}
	*(dest++) = src[x], *(dest++) = src[x];
	src += (pcd_def_width[3] - left) >> 1;
    }
    return 0;
}

int
pcd_inter_pixels(unsigned char **data, int width, int height)
{
    register unsigned char *src, *dest;
    register int    x;
    int             y;

    for (y = height - 2; y >= 0; y -= 2) {
	src = data[y >> 1];
	dest = data[y];
	dest[width - 2] = dest[width - 1] = src[(width >> 1) - 1];
	for (x = width - 4; x >= 0; x -= 2) {
	    dest[x] = src[x >> 1];
	    dest[x + 1] = (src[x >> 1] + src[(x >> 1) + 1] + 1) >> 1;
	}
    }
    return 0;
}

int
pcd_inter_lines(unsigned char **data, int width, int height)
{
    register unsigned char *src1, *src2, *dest;
    register int    x;
    int             y;

    for (y = 0; y < height - 2; y += 2) {
	src1 = data[y];
	dest = data[y + 1];
	src2 = data[y + 2];
	for (x = 0; x < width - 2; x += 2) {
	    dest[x] = (src1[x] + src2[x] + 1) >> 1;
	    dest[x + 1] = (src1[x] + src2[x] + src1[x + 2] + src2[x + 2] + 2) >> 2;
	}
	dest[x] = dest[x + 1] = (src1[x] + src2[x] + 1) >> 1;
    }
    src1 = data[y];
    dest = data[y + 1];
    for (x = 0; x < width - 2; x += 2) {
	dest[x] = src1[x];
	dest[x + 1] = (src1[x] + src1[x + 2] + 1) >> 1;
    }
    dest[x] = dest[x + 1] = src1[x];
    return 0;
}


/*
 * End of "$Id: pcd-inter.c 321 2005-01-23 03:52:44Z easysw $".
 */
