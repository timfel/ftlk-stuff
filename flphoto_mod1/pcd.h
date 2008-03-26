/*
 * "$Id: pcd.h 321 2005-01-23 03:52:44Z easysw $"
 *
 * PhotoCD library file routines.
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

#ifndef _PCD_H_
#  define _PCD_H_

#  ifdef __cplusplus
extern "C" {
#  endif /* __cplusplus */

struct PCD_IMAGE {
    int             size;
    unsigned char  *mmap;
    int             thumbnails;	/* # of thumbnails, 0 for normal image */

    int             res, nr, gray, verbose;
    int             left, top, width, height, rot;
    unsigned char **luma;
    unsigned char **red;
    unsigned char **blue;
    unsigned char  *data;

    unsigned long  *lut_red;
    unsigned long  *lut_green;
    unsigned long  *lut_blue;

    unsigned char  *seq1;	/* huffman tables */
    unsigned char  *len1;
    unsigned char  *seq2;
    unsigned char  *len2;
    unsigned char  *seq3;
    unsigned char  *len3;
};

/* --- pcd-file.c --- */

#define PCD_WIDTH(res,rot)   (rot&1?pcd_def_height[res]:pcd_def_width[res])
#define PCD_HEIGHT(res,rot)  (rot&1?pcd_def_width[res]:pcd_def_height[res])

extern char     pcd_rotor[];
extern int      pcd_img_start[];
extern int      pcd_def_width[];
extern int      pcd_def_height[];
extern char     pcd_errmsg[];

int             pcd_open(struct PCD_IMAGE *img, char *filename);
int             pcd_open_fd(struct PCD_IMAGE *img, int fd);
int             pcd_get_rot(struct PCD_IMAGE *img, int nr);
int             pcd_get_maxres(struct PCD_IMAGE *img);
int             pcd_select(struct PCD_IMAGE *img, int res, int nr, int gray, int verbose,
		  int rot, int *left, int *top, int *width, int *height);
int             pcd_free(struct PCD_IMAGE *img);
int             pcd_close(struct PCD_IMAGE *img);

/* --- pcd-yuv2rgb.c --- */
#define PCD_RANGE       320
extern unsigned int LUT_range[256 + 2 * PCD_RANGE];

extern unsigned long LUT_15_red[256];
extern unsigned long LUT_15_green[256];
extern unsigned long LUT_15_blue[256];

extern unsigned long LUT_16_red[256];
extern unsigned long LUT_16_green[256];
extern unsigned long LUT_16_blue[256];

extern unsigned long LUT_24_red[256];
extern unsigned long LUT_24_green[256];
extern unsigned long LUT_24_blue[256];

#define PCD_TYPE_GRAY       1	/* gray           - 1 byte/pixel */
#define PCD_TYPE_RGB        2	/* red,green,blue - 3 byte/pixel */
#define PCD_TYPE_BGR        3	/* blue,green,red - 3 byte/pixel */
#define PCD_TYPE_LUT_SHORT  4	/* lookup table   - 2 byte/pixel */
#define PCD_TYPE_LUT_LONG   5	/* lookup table   - 4 byte/pixel */

void            pcd_get_LUT_init(void);
void            pcd_set_lookup(struct PCD_IMAGE *img, unsigned long *red,
			       unsigned long *green, unsigned long *blue);
int             pcd_get_image_line(struct PCD_IMAGE *img, int line,
			       unsigned char *dest, int type, int scale);
int             pcd_get_image(struct PCD_IMAGE *img,
			      unsigned char *dest, int type, int scale);

/* --- pcd-huff.c --- */
int             pcd_read_htable(unsigned char *src,
			    unsigned char **pseq, unsigned char **pbits);
int             pcd_decode(struct PCD_IMAGE *img);

/* --- pcd-inter.c --- */
int             pcd_inter_m2(struct PCD_IMAGE *img);
int             pcd_inter_pixels(unsigned char **data, int width, int height);
int             pcd_inter_lines(unsigned char **data, int width, int height);

/* ----------------------------------------------------------------- */
#  define TELL(x)  { if(img->verbose) fputc(x,stderr); }
#  define ROTOR(x) { if(img->verbose) fprintf(stderr,"%c\010",pcd_rotor[x&3]); }

#  ifdef __cplusplus
}
#  endif /* __cplusplus */
#endif /* !_PCD_H_ */


/*
 * End of "$Id: pcd.h 321 2005-01-23 03:52:44Z easysw $".
 */
