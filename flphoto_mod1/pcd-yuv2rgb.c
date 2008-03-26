/*
 * "$Id: pcd-yuv2rgb.c 321 2005-01-23 03:52:44Z easysw $"
 *
 * PhotoCD library YUV conversion routines.
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

/* ------------------------------------------------------------------------ */

#define RANGE       320

#define RED_NUL     137
#define BLUE_NUL    156

#define LUN_MUL     360
#define RED_MUL     512
#define BLUE_MUL    512
#define GREEN1_MUL  (-RED_MUL/2)
#define GREEN2_MUL  (-BLUE_MUL/6)

#define RED_ADD     (-RED_NUL*RED_MUL)
#define BLUE_ADD    (-BLUE_NUL*BLUE_MUL)
#define GREEN1_ADD  (-RED_ADD/2)
#define GREEN2_ADD  (-BLUE_ADD/6)

static unsigned int LUT_flag = 0;
static unsigned char LUT_gray_char[256];
static unsigned int LUT_gray_int[256];
static unsigned int LUT_red[256];
static unsigned int LUT_blue[256];
static unsigned int LUT_green1[256];
static unsigned int LUT_green2[256];
unsigned int    LUT_range[256 + 2 * RANGE];

unsigned long   LUT_15_red[256];
unsigned long   LUT_15_green[256];
unsigned long   LUT_15_blue[256];

unsigned long   LUT_16_red[256];
unsigned long   LUT_16_green[256];
unsigned long   LUT_16_blue[256];

unsigned long   LUT_24_red[256];
unsigned long   LUT_24_green[256];
unsigned long   LUT_24_blue[256];

/* ------------------------------------------------------------------------ */

void
pcd_get_LUT_init()
{
    register int    i;

    /* only once needed */
    if (LUT_flag)
	return;
    LUT_flag = 1;

    /* init Lookup tables */
    for (i = 0; i < 256; i++) {
	LUT_gray_int[i] = i * LUN_MUL >> 8;
	LUT_red[i] = (RED_ADD + i * RED_MUL) >> 8;
	LUT_blue[i] = (BLUE_ADD + i * BLUE_MUL) >> 8;
	LUT_green1[i] = (GREEN1_ADD + i * GREEN1_MUL) >> 8;
	LUT_green2[i] = (GREEN2_ADD + i * GREEN2_MUL) >> 8;
	LUT_gray_char[i] = LUT_gray_int[i] > 255 ? 255 : LUT_gray_int[i];

	LUT_15_red[i] = (i & 0xf8) << 7;	/* bits -rrrrr-- -------- */
	LUT_15_green[i] = (i & 0xf8) << 2;	/* bits------gg ggg----- */
	LUT_15_blue[i] = (i & 0xf8) >> 3;	/* bits-------- ---bbbbb */

	LUT_16_red[i] = (i & 0xf8) << 8;	/* bits rrrrr--- -------- */
	LUT_16_green[i] = (i & 0xfc) << 3;	/* bits -----ggg ggg----- */
	LUT_16_blue[i] = (i & 0xf8) >> 3;	/* bits -------- ---bbbbb */

	LUT_24_red[i] = i << 16;	/* byte -r-- */
	LUT_24_green[i] = i << 8;	/* byte --g- */
	LUT_24_blue[i] = i;		/* byte ---b */
    }
    for (i = 0; i < RANGE; i++)
	LUT_range[i] = 0;
    for (; i < RANGE + 256; i++)
	LUT_range[i] = i - RANGE;
    for (; i < 2 * RANGE + 256; i++)
	LUT_range[i] = 255;
}

void
pcd_set_lookup(struct PCD_IMAGE *img, unsigned long *red,
	       unsigned long *green, unsigned long *blue)
{
    img->lut_red = red;
    img->lut_green = green;
    img->lut_blue = blue;
}

/* ------------------------------------------------------------------------ */

#define GET_RED   (LUT_range[RANGE + gray + \
			    LUT_red[red[x]]])
#define GET_GREEN (LUT_range[RANGE + gray + \
			    LUT_green1[red[x]] + \
			    LUT_green2[blue[x]]])
#define GET_BLUE  (LUT_range[RANGE + gray + \
			    LUT_blue[blue[x]]])

static int
pcd_get_image_line_0(struct PCD_IMAGE *img, int y,
		     unsigned char *dest, int type, int scale)
{
    unsigned char red[3072];
    unsigned char blue[3072];
    int             bytes, maxx;

    switch (type) {
    case PCD_TYPE_GRAY:
	bytes = 1;
	break;
    case PCD_TYPE_RGB:
	bytes = 3;
	break;
    case PCD_TYPE_BGR:
	bytes = 3;
	break;
    case PCD_TYPE_LUT_SHORT:
	bytes = 2;
	break;
    case PCD_TYPE_LUT_LONG:
	bytes = 4;
	break;
    default:
	fprintf(stderr, "Oops: invalid type (%i) for output format\n", type);
	exit(1);
    }

    if (img->rot & 2) {
	y = (img->height >> scale) - y - 1;
	dest += ((img->width >> scale) - 1) * bytes;
	bytes = -bytes;
    }
    if (type != PCD_TYPE_GRAY && !scale) {
	register int    x;
	register unsigned char *src1, *src2;

	maxx = (img->width >> 1) - 1;
	if (y & 1) {
	    src1 = img->blue[y >> 1];
	    src2 = img->blue[(y + 1 == img->height ? y : y + 1) >> 1];
	    for (x = 0; x < maxx; x++) {
		blue[x * 2] = (src1[x] + src2[x] + 1) >> 1;
		blue[x * 2 + 1] = (src1[x] + src1[x + 1] + src2[x] + src2[x + 1] + 2) >> 2;
	    }
	    blue[x * 2 + 1] = blue[x * 2] = (src1[x] + src2[x] + 1) >> 1;

	    src1 = img->red[y >> 1];
	    src2 = img->red[(y + 1 == img->height ? y : y + 1) >> 1];
	    for (x = 0; x < maxx; x++) {
		red[x * 2] = (src1[x] + src2[x] + 1) >> 1;
		red[x * 2 + 1] = (src1[x] + src1[x + 1] + src2[x] + src2[x + 1] + 2) >> 2;
	    }
	    red[x * 2 + 1] = red[x * 2] = (src1[x] + src2[x] + 1) >> 1;
	} else {
	    src1 = img->blue[y >> 1];
	    for (x = 0; x < maxx; x++) {
		blue[x * 2] = src1[x];
		blue[x * 2 + 1] = (src1[x] + src1[x + 1] + 1) >> 1;
	    }
	    blue[x * 2 + 1] = blue[x * 2] = src1[x];

	    src1 = img->red[y >> 1];
	    for (x = 0; x < maxx; x++) {
		red[x * 2] = src1[x];
		red[x * 2 + 1] = (src1[x] + src1[x + 1] + 1) >> 1;
	    }
	    red[x * 2 + 1] = red[x * 2] = src1[x];
	}
    }
    if (type != PCD_TYPE_GRAY && scale) {
	memcpy(blue, img->blue[y], img->width >> 1);
	memcpy(red, img->red[y], img->width >> 1);
    }
    maxx = img->width >> scale;
    switch (type) {
    case PCD_TYPE_GRAY:
	{
	    register int    x;
	    register unsigned char *luma = img->luma[y << scale];

	    for (x = 0; x < maxx; x++, dest += bytes)
		*dest = LUT_gray_char[luma[x << scale]];
	}
	break;
    case PCD_TYPE_RGB:
	{
	    register int    x, gray;
	    register unsigned char *luma = img->luma[y << scale];

	    for (x = 0; x < maxx; x++, dest += bytes) {
		gray = LUT_gray_int[luma[x << scale]];
		dest[0] = GET_RED;
		dest[1] = GET_GREEN;
		dest[2] = GET_BLUE;
	    }
	}
	break;
    case PCD_TYPE_BGR:
	{
	    register int    x, gray;
	    register unsigned char *luma = img->luma[y << scale];

	    for (x = 0; x < maxx; x++, dest += bytes) {
		gray = LUT_gray_int[luma[x << scale]];
		dest[0] = GET_BLUE;
		dest[1] = GET_GREEN;
		dest[2] = GET_RED;
	    }
	}
	break;
    case PCD_TYPE_LUT_SHORT:
	{
	    register int    x, gray;
	    register unsigned char *luma = img->luma[y << scale];
	    unsigned long  *lr = img->lut_red;
	    unsigned long  *lg = img->lut_green;
	    unsigned long  *lb = img->lut_blue;

	    for (x = 0; x < maxx; x++, dest += bytes) {
		gray = LUT_gray_int[luma[x << scale]];
		*((unsigned short *) dest) =
		    lr[GET_RED] | lg[GET_GREEN] | lb[GET_BLUE];
	    }
	}
	break;
    case PCD_TYPE_LUT_LONG:
	{
	    register int    x, gray;
	    register unsigned char *luma = img->luma[y << scale];
	    unsigned long  *lr = img->lut_red;
	    unsigned long  *lg = img->lut_green;
	    unsigned long  *lb = img->lut_blue;

	    for (x = 0; x < maxx; x++, dest += bytes) {
		gray = LUT_gray_int[luma[x << scale]];
		*((unsigned long *) dest) =
		    lr[GET_RED] | lg[GET_GREEN] | lb[GET_BLUE];
	    }
	}
	break;
    default:
	exit(1);
    }
    return 0;
}

static int
pcd_get_image_line_90(struct PCD_IMAGE *img, int y,
		      unsigned char *dest, int type, int scale)
{
    unsigned char red[3072];
    unsigned char blue[3072];
    unsigned char **luma = img->luma;
    int             bytes, maxx, y1, y2;

    switch (type) {
    case PCD_TYPE_GRAY:
	bytes = 1;
	break;
    case PCD_TYPE_RGB:
	bytes = 3;
	break;
    case PCD_TYPE_BGR:
	bytes = 3;
	break;
    case PCD_TYPE_LUT_SHORT:
	bytes = 2;
	break;
    case PCD_TYPE_LUT_LONG:
	bytes = 4;
	break;
    default:
	fprintf(stderr, "Oops: invalid type (%i) for output format\n", type);
	exit(1);
    }

    if (!(img->rot & 2)) {
	y = (img->width >> scale) - y - 1;
    } else {
	dest += ((img->height >> scale) - 1) * bytes;
	bytes = -bytes;
    }

    if (type != PCD_TYPE_GRAY && !scale) {
	register int    x;
	register unsigned char **src;

	y1 = y >> 1;
	y2 = (y + 1 == img->width ? y : y + 1) >> 1;
	maxx = (img->height >> 1) - 1;

	if (y & 1) {
	    src = img->blue;
	    for (x = 0; x < maxx; x++) {
		blue[x * 2] = (src[x][y1] + src[x][y2] + 1) >> 1;
		blue[x * 2 + 1] = (src[x][y1] + src[x][y2] +
			       src[x + 1][y1] + src[x + 1][y2] + 2) >> 2;
	    }
	    blue[x * 2 + 1] = blue[x * 2] = (src[x][y1] + src[x][y2] + 1) >> 1;

	    src = img->red;
	    for (x = 0; x < maxx; x++) {
		red[x * 2] = (src[x][y1] + src[x][y2] + 1) >> 1;
		red[x * 2 + 1] = (src[x][y1] + src[x][y2] +
			       src[x + 1][y1] + src[x + 1][y2] + 2) >> 2;
	    }
	    red[x * 2 + 1] = red[x * 2] = (src[x][y1] + src[x][y2] + 1) >> 1;
	} else {
	    src = img->blue;
	    for (x = 0; x < maxx; x++) {
		blue[x * 2] = src[x][y1];
		blue[x * 2 + 1] = (src[x][y1] + src[x + 1][y1] + 1) >> 1;
	    }
	    blue[x * 2 + 1] = blue[x * 2] = src[x][y1];

	    src = img->red;
	    for (x = 0; x < maxx; x++) {
		red[x * 2] = src[x][y1];
		red[x * 2 + 1] = (src[x][y1] + src[x + 1][y1] + 1) >> 1;
	    }
	    red[x * 2 + 1] = red[x * 2] = src[x][y1];
	}
    }
    if (type != PCD_TYPE_GRAY && scale) {
	register int    x;
	register unsigned char **src;

	maxx = (img->height >> 1);

	src = img->blue;
	for (x = 0; x < maxx; x++)
	    blue[x] = src[x][y];

	src = img->red;
	for (x = 0; x < maxx; x++)
	    red[x] = src[x][y];
    }
    maxx = (img->height >> scale);
    switch (type) {
    case PCD_TYPE_GRAY:
	{
	    register int    x;

	    for (x = 0; x < maxx; x++, dest += bytes)
		*dest = LUT_gray_char[luma[x << scale][y << scale]];
	}
	break;
    case PCD_TYPE_RGB:
	{
	    register int    x, gray;

	    for (x = 0; x < maxx; x++, dest += bytes) {
		gray = LUT_gray_int[luma[x << scale][y << scale]];
		dest[0] = GET_RED;
		dest[1] = GET_GREEN;
		dest[2] = GET_BLUE;
	    }
	}
	break;
    case PCD_TYPE_BGR:
	{
	    register int    x, gray;

	    for (x = 0; x < maxx; x++, dest += bytes) {
		gray = LUT_gray_int[luma[x << scale][y << scale]];
		dest[0] = GET_BLUE;
		dest[1] = GET_GREEN;
		dest[2] = GET_RED;
	    }
	}
	break;
    case PCD_TYPE_LUT_SHORT:
	{
	    register int    x, gray;
	    unsigned long  *lr = img->lut_red;
	    unsigned long  *lg = img->lut_green;
	    unsigned long  *lb = img->lut_blue;

	    for (x = 0; x < maxx; x++, dest += bytes) {
		gray = LUT_gray_int[luma[x << scale][y << scale]];
		*((unsigned short *) dest) =
		    lr[GET_RED] | lg[GET_GREEN] | lb[GET_BLUE];
	    }
	}
	break;
    case PCD_TYPE_LUT_LONG:
	{
	    register int    x, gray;
	    unsigned long  *lr = img->lut_red;
	    unsigned long  *lg = img->lut_green;
	    unsigned long  *lb = img->lut_blue;

	    for (x = 0; x < maxx; x++, dest += bytes) {
		gray = LUT_gray_int[luma[x << scale][y << scale]];
		*((unsigned long *) dest) =
		    lr[GET_RED] | lg[GET_GREEN] | lb[GET_BLUE];
	    }
	}
	break;
    default:
	exit(1);
    }
    return 0;
}

int
pcd_get_image_line(struct PCD_IMAGE *img, int y,
		   unsigned char *dest, int type, int scale)
{
    if (img->res == 0) {
	fprintf(stderr, "Oops: invalid res %i, have you called pcd_select()?\n",
		img->res);
	exit(1);
    }
    if (img->rot & 1)
	return pcd_get_image_line_90(img, y, dest, type, scale);
    else
	return pcd_get_image_line_0(img, y, dest, type, scale);
}

int
pcd_get_image(struct PCD_IMAGE *img, unsigned char *dest, int type, int scale)
{
    int             y, maxx, maxy, bytes;

    if (img->res == 0) {
	fprintf(stderr, "Oops: invalid res %i, have you called pcd_select()?\n",
		img->res);
	exit(1);
    }
    switch (type) {
    case PCD_TYPE_GRAY:
	bytes = 1;
	break;
    case PCD_TYPE_RGB:
	bytes = 3;
	break;
    case PCD_TYPE_BGR:
	bytes = 3;
	break;
    case PCD_TYPE_LUT_SHORT:
	bytes = 2;
	break;
    case PCD_TYPE_LUT_LONG:
	bytes = 4;
	break;
    default:
	fprintf(stderr, "Oops: invalid type (%i) for output format\n", type);
	exit(1);
    }
    maxx = (img->rot & 1 ? img->height : img->width) >> scale;
    maxy = (img->rot & 1 ? img->width : img->height) >> scale;

    for (y = 0; y < maxy; y++, dest += bytes * maxx) {
	ROTOR(y);
	pcd_get_image_line(img, y, dest, type, scale);
    }
    TELL('*');

    return 0;
}


/*
 * End of "$Id: pcd-yuv2rgb.c 321 2005-01-23 03:52:44Z easysw $".
 */
