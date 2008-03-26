/*
 * "$Id: pcd-file.c 321 2005-01-23 03:52:44Z easysw $"
 *
 * PhotoCD library file routines, modified to not use mmap().
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
#if defined(WIN32) && !defined(__EMX__) && !defined(__CYGWIN__)
#  include <io.h>
#  include <direct.h>
#else
#  include <unistd.h>
#endif /* WIN32 && !__EMX__ && !__CYGWIN__ */
#include <fcntl.h>
#include <errno.h>

#include "pcd.h"

char            pcd_rotor[] =
{'-', '\\', '|', '/'};
int             pcd_img_start[] =
{0 /*dummy */ , 8192, 47104, 196608};
int             pcd_def_width[] =
{0 /*dummy */ , 192, 384, 768, 1536, 3072, 6144};
int             pcd_def_height[] =
{0 /*dummy */ , 128, 256, 512, 1024, 2048, 4096};
char            pcd_errmsg[512];

int
pcd_open(struct PCD_IMAGE *img, char *filename)
{
    int fd;
    int	status;


    fd = open(filename, O_RDONLY);
    if (-1 == fd) {
	sprintf(pcd_errmsg, "open %s: %s", filename, strerror(errno));
	return -1;
    }

    status = pcd_open_fd(img, fd);

    close(fd);

    return (status);
}

int
pcd_open_fd(struct PCD_IMAGE *img, int fd)
{
    pcd_get_LUT_init();
    memset(img, 0, sizeof(struct PCD_IMAGE));

    img->size = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    img->mmap = malloc(img->size);

    if (img->mmap == NULL) {
	sprintf(pcd_errmsg, "malloc: %s", strerror(errno));
	pcd_close(img);
	return -1;
    }

    read(fd, img->mmap, img->size);

    if (0 == strncmp("PCD_OPA", img->mmap, 7)) {
	/* this is the thumbnails file */
	img->thumbnails = (int) img->mmap[10] << 8 | (int) img->mmap[11];
    } else {
	if (img->size < 786432) {
	    strlcpy(pcd_errmsg, "probably not a PhotoCD image (too small)",
	            sizeof(pcd_errmsg));
	    pcd_close(img);
	    return -1;
	}
    }
    return img->thumbnails;
}

int
pcd_get_rot(struct PCD_IMAGE *img, int nr)
{
    if (img->thumbnails) {
	return img->mmap[12 + nr] & 3;
    } else {
	return img->mmap[0x48] & 3;
    }
}

int
pcd_get_maxres(struct PCD_IMAGE *img)
{
    if (img->thumbnails) {
	return 1;
    } else {
	if (img->size == 786432)
	    return 3;
	else
	    return 5;
    }
}

int
pcd_select(struct PCD_IMAGE *img, int res, int nr, int gray, int verbose,
	   int rot, int *left, int *top, int *width, int *height)
{
    int             y;
    unsigned char  *ptr;

    /* free old stuff */
    pcd_free(img);

    /* sanity checks... */
    if (0 == img->thumbnails) {
	if (res < 1 || res > 5) {
	    sprintf(pcd_errmsg, "invalid resolution (%i) specified", res);
	    return -1;
	}
	if (img->size == 786432 && res > 3) {
	    sprintf(pcd_errmsg,
	       "PhotoCD file contains only the three lower resolutions");
	    return -1;
	}
    } else {
	if (nr < 0 || nr >= img->thumbnails) {
	    sprintf(pcd_errmsg,
		    "thumbnail number (%i) out of range", nr);
	    return -1;
	}
    }

    /* width/height == 0: fill in default image size */
    if (*left == 0 && *width == 0)
	*width = PCD_WIDTH(res, rot);
    if (*top == 0 && *height == 0)
	*height = PCD_HEIGHT(res, rot);

    if (5 == res)
	*left &= ~7, *top &= ~7, *width &= ~7, *height &= ~7;
    else if (4 == res)
	*left &= ~3, *top &= ~3, *width &= ~3, *height &= ~3;
    else
	*left &= ~1, *top &= ~1, *width &= ~1, *height &= ~1;
    if (*left < 0 || *top < 0 ||
	*width < 1 || *height < 1 ||
	*left + *width > PCD_WIDTH(res, rot) ||
	*top + *height > PCD_HEIGHT(res, rot)) {
	sprintf(pcd_errmsg, "specified area (%ix%i+%i+%i) invalid",
		*width, *height, *left, *top);
	return -1;
    }
    /* recalc coordinates (rotation) */
    switch (rot) {
    case 0:			/* none */
	img->left = *left;
	img->top = *top;
	img->width = *width;
	img->height = *height;
	break;
    case 1:			/* 90° ccw */
	img->left = PCD_HEIGHT(res, rot) - *top - *height;
	img->top = *left;
	img->width = *height;
	img->height = *width;
	break;
    case 2:			/* 180° */
	img->left = PCD_WIDTH(res, rot) - *left - *width;
	img->top = PCD_HEIGHT(res, rot) - *top - *height;
	img->width = *width;
	img->height = *height;
	break;
    case 3:			/* 90° cw */
	img->left = *top;
	img->top = PCD_WIDTH(res, rot) - *left - *width;
	img->width = *height;
	img->height = *width;
	break;
    default:
	sprintf(pcd_errmsg, "specified orientation (%i) invalid", rot);
	return -1;
    }
    /* prepeare */
    img->res = res;
    img->nr = nr;
    img->gray = gray;
    img->verbose = verbose;
    img->rot = rot;
    img->luma = malloc(img->height * sizeof(unsigned char *));
    img->red = malloc(img->height * sizeof(unsigned char *) >> 1);
    img->blue = malloc(img->height * sizeof(unsigned char *) >> 1);

    if (img->luma == NULL ||
	img->red == NULL ||
	img->blue == NULL) {
	sprintf(pcd_errmsg, "out of memory (malloc failed)");
	pcd_free(img);
	return -1;
    }
    if (res <= 3) {
	/* just fill in pointers */
	if (img->thumbnails) {
	    ptr = img->mmap + 10240 + 36864 * nr +
		(pcd_def_width[res] >> 1) * 3 * img->top;
	} else {
	    ptr = img->mmap + pcd_img_start[res] +
		(pcd_def_width[res] >> 1) * 3 * img->top;
	}
	for (y = 0; y < img->height; y += 2, ptr += (pcd_def_width[res] >> 1) * 6) {
	    img->luma[y] = ptr + img->left;
	    img->luma[y + 1] = ptr + img->left + (pcd_def_width[res] >> 1) * 2;
	    img->blue[y >> 1] = ptr + (img->left >> 1) + (pcd_def_width[res] >> 1) * 4;
	    img->red[y >> 1] = ptr + (img->left >> 1) + (pcd_def_width[res] >> 1) * 5;
	}
    } else {
	/* high res, have to malloc memory */
	img->data = malloc(img->width * img->height * 3 / 2);
	if (img->data == NULL) {
	    sprintf(pcd_errmsg, "out of memory (malloc failed)");
	    pcd_free(img);
	    return -1;
	}
	ptr = img->data;
	for (y = 0; y < img->height; y++, ptr += img->width)
	    img->luma[y] = ptr;
	for (y = 0; y < img->height >> 1; y++, ptr += img->width >> 1)
	    img->blue[y] = ptr;
	for (y = 0; y < img->height >> 1; y++, ptr += img->width >> 1)
	    img->red[y] = ptr;
    }
    return 0;
}

int
pcd_free(struct PCD_IMAGE *img)
{
    img->res = 0;
    if (img->data)
	free(img->data);
    if (img->luma)
	free(img->luma);
    if (img->red)
	free(img->red);
    if (img->blue)
	free(img->blue);
    if (img->seq1)
	free(img->seq1);
    if (img->len1)
	free(img->len1);
    if (img->seq2)
	free(img->seq2);
    if (img->len2)
	free(img->len2);
    if (img->seq3)
	free(img->seq3);
    if (img->len3)
	free(img->len3);
    img->data = NULL;
    img->luma = img->red = img->blue = NULL;
    img->seq1 = img->seq2 = img->seq3 = NULL;
    img->len1 = img->len2 = img->len3 = NULL;
    return 0;
}

int
pcd_close(struct PCD_IMAGE *img)
{
    pcd_free(img);
    free(img->mmap);
    memset(img, 0, sizeof(struct PCD_IMAGE));

    return 0;
}


/*
 * End of "$Id: pcd-file.c 321 2005-01-23 03:52:44Z easysw $".
 */
