/*
 * "$Id: pcd-huff.c 321 2005-01-23 03:52:44Z easysw $"
 *
 * PhotoCD library huffman decompression routines.
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

#define HUFF1 0xc2000

int
pcd_read_htable(unsigned char *src,
		unsigned char **pseq, unsigned char **pbits)
{
    int             len, seq, seq2, bits, i, j;

    if (*pseq)
	free(*pseq);
    if (*pbits)
	free(*pbits);
    *pseq = malloc(0x10000 * sizeof(char));
    memset(*pseq, 0, 0x10000 * sizeof(char));
    *pbits = malloc(0x10000 * sizeof(char));
    memset(*pbits, 0, 0x10000 * sizeof(char));

    if (*pseq == NULL || *pbits == NULL)
	return -1;
    for (i = 1, len = *src; len >= 0; i += 4, len--) {
	seq = ((int) src[i + 1] << 8 | (int) src[i + 2]);
	bits = src[i] + 1;
	seq2 = seq + (0x10000 >> bits);
	for (j = seq; j < seq2; j++) {
	    if ((*pbits)[j]) {
		sprintf(pcd_errmsg, "Invalid huffmann code table, seems the file is'nt a PhotoCD image");
		return -1;
	    }
	    (*pseq)[j] = src[i + 3];
	    (*pbits)[j] = bits;
	}
    }
    return i;
}

#define SETSHIFT       { shiftreg=(((stream[0]<<16) | (stream[1]<<8 ) | \
				    (stream[2])) >> (8-bit)) & 0xffff; }

#define LEFTSHIFT      { shiftreg=((shiftreg<<1) & 0xffff) | \
    ((stream[2]>>(7-bit++))&1); stream += bit>>3; bit &= 7; }

static int
pcd_un_huff(struct PCD_IMAGE *img, unsigned char *start, int run)
{
    register int    shiftreg, bit;
    unsigned char  *stream = start;
    int             y, type, shift;
    int             height, y1, y2;

    switch (run) {
    case 1:
	height = pcd_def_height[4];
	y1 = img->top >> (img->res - 4);
	y2 = (img->top + img->height) >> (img->res - 4);
	break;
    case 2:
	height = pcd_def_height[5];
	y1 = img->top >> (img->res - 5);
	y2 = (img->top + img->height) >> (img->res - 5);
	break;
    default:
	fprintf(stderr, "internal error: pcd_decode: run %i ???\n", run);
	exit(1);
    }

    for (y = 0; y < height;) {
	ROTOR(y);
	for (;;) {
	    bit = 0;
	    stream = memchr(stream, 0xff, 10240);
	    if (stream[1] == 0xff)
		break;
	    stream++;
	}
	SETSHIFT;
	while (shiftreg != 0xfffe)
	    LEFTSHIFT;
	stream += 2;
	SETSHIFT;
	y = (shiftreg >> 1) & 0x1fff;
	type = (shiftreg >> 14);
	stream += 2;
	SETSHIFT;

	if (y > height) {
	    sprintf(pcd_errmsg, "Oops: invalid line nr (y=%i)\n", y);
	    return -1;
	}
	if (y < y1 || y >= y2)
	    continue;
	if (img->gray && type)
	    return 0;		/* cut color decoding */

	{
	    register unsigned char *data;
	    register int    x;
	    unsigned char  *seq;
	    unsigned char  *bits;
	    int             x1, x2;

	    switch (type) {
	    case 0:
		shift = 0;
		seq = img->seq1;
		bits = img->len1;
		data = img->luma[(y - y1) >> shift];
		break;
	    case 2:
		shift = 1;
		seq = img->seq2;
		bits = img->len2;
		data = img->blue[(y - y1) >> shift];
		break;
	    case 3:
		shift = 1;
		seq = img->seq3;
		bits = img->len3;
		data = img->red[(y - y1) >> shift];
		break;
	    default:
		sprintf(pcd_errmsg, "Oops: invalid line type (type=%i)\n", type);
		return -1;
	    }

	    if (run == 1) {
		x1 = img->left >> (img->res - 4 + shift);
		x2 = (img->width) >> (img->res - 4 + shift);
	    } else {
		x1 = img->left >> (img->res - 5 + shift);
		x2 = (img->width) >> (img->res - 5 + shift);
	    }
	    for (x = 0; x < x1; x++) {
		bit += bits[shiftreg];
		stream += bit >> 3, bit &= 7;
		SETSHIFT;
	    }
	    for (x = 0; x < x2; x++) {
		data[x] = LUT_range[PCD_RANGE + (int) data[x] +
				    (signed char) seq[shiftreg]];
		bit += bits[shiftreg];
		stream += bit >> 3, bit &= 7;
		SETSHIFT;
	    }
	}
    }
    return ((stream - start) + 0x6000 + 2047) & ~0x7ff;
}

int
pcd_decode(struct PCD_IMAGE *img)
{
    int             pos = HUFF1, rc;

    switch (img->res) {
    case 1:
    case 2:
    case 3:
	/* nothing to do */
	break;
    case 4:
	pcd_inter_m2(img);

	if (!img->gray) {
	    pcd_inter_lines(img->blue, img->width >> 1, img->height >> 1);
	    pcd_inter_lines(img->red, img->width >> 1, img->height >> 1);
	}
	pcd_inter_lines(img->luma, img->width, img->height);
	if (-1 == (rc = pcd_read_htable(img->mmap + pos, &img->seq1, &img->len1)))
	    return -1;
	pos += rc;
	pos = (pos + 2047) & ~0x3ff;
	if (-1 == pcd_un_huff(img, img->mmap + pos, 1))
	    return (-1);
	TELL('*');
	break;
    case 5:
	pcd_inter_m2(img);

	if (!img->gray) {
	    pcd_inter_lines(img->blue, img->width >> 2, img->height >> 2);
	    pcd_inter_pixels(img->blue, img->width >> 1, img->height >> 1);
	    pcd_inter_lines(img->blue, img->width >> 1, img->height >> 1);
	    pcd_inter_lines(img->red, img->width >> 2, img->height >> 2);
	    pcd_inter_pixels(img->red, img->width >> 1, img->height >> 1);
	    pcd_inter_lines(img->red, img->width >> 1, img->height >> 1);
	}
	pcd_inter_lines(img->luma, img->width >> 1, img->height >> 1);
	if (-1 == (rc = pcd_read_htable(img->mmap + pos, &img->seq1, &img->len1)))
	    return -1;
	pos += rc;
	pos = (pos + 2047) & ~0x3ff;
	if (-1 == (rc = pcd_un_huff(img, img->mmap + pos, 1)))
	    return (-1);
	pos += rc;
	TELL('*');
	pcd_inter_pixels(img->luma, img->width, img->height);
	pcd_inter_lines(img->luma, img->width, img->height);
	if (-1 == (rc = pcd_read_htable(img->mmap + pos, &img->seq1, &img->len1)))
	    return -1;
	pos += rc;
	if (-1 == (rc = pcd_read_htable(img->mmap + pos, &img->seq2, &img->len2)))
	    return -1;
	pos += rc;
	if (-1 == (rc = pcd_read_htable(img->mmap + pos, &img->seq3, &img->len3)))
	    return -1;
	pos += rc;
	pos = (pos + 2047) & ~0x3ff;
	if (-1 == pcd_un_huff(img, img->mmap + pos, 2))
	    return -1;
	TELL('*');
	break;
    default:
	fprintf(stderr, "Oops: invalid res %i, have you called pcd_select()?\n",
		img->res);
	exit(1);
	break;
    }
    return 0;
}


/*
 * End of "$Id: pcd-huff.c 321 2005-01-23 03:52:44Z easysw $".
 */
