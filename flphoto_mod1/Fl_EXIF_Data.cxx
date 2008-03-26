//
// "$Id: Fl_EXIF_Data.cxx 321 2005-01-23 03:52:44Z easysw $"
//
// EXIF data class methods for the Fast Light Tool Kit (FLTK).
//
// Copyright 2002-2005 by Michael Sweet.
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
//   Fl_EXIF_Data::Fl_EXIF_Data()  - Load EXIF data from an image file.
//   Fl_EXIF_Data::~Fl_EXIF_Data() - Destroy EXIF data.
//   Fl_EXIF_Data::get_ascii()     - Get a string value.
//   Fl_EXIF_Data::get_binary()    - Get a binary data value.
//   Fl_EXIF_Data::get_integer()   - Get an integer value.
//   Fl_EXIF_Data::get_rational()  - Get a rational value.
//   Fl_EXIF_Data::get_ushort()    - Get a 16-bit unsigned integer.
//   Fl_EXIF_Data::get_uint()      - Get a 32-bit unsigned integer.
//   Fl_EXIF_Data::get_sint()      - Get a 32-bit signed integer.
//   Fl_EXIF_Data::compare_ifds()  - Compare two directory entries.
//   Fl_EXIF_Data::find_ifd()      - Find a directory entry.
//   Fl_EXIF_Data::get_ifd()       - Get a tag directory.
//   Fl_EXIF_Data::parse_comment() - Parse comment data in an EXIF image.
//   Fl_EXIF_Data::parse_exif()    - Parse EXIF data in an EXIF image.
//

//
// Include necessary header files...
//

#include "Fl_EXIF_Data.H"
#include <stdio.h>
#include <stdlib.h>
#include "flstring.h"


// Some releases of the Cygwin JPEG libraries don't have a correctly
// updated header file for the INT32 data type; the following define
// from Shane Hill seems to be a usable workaround...

#if defined(WIN32) && defined(__CYGWIN__)
#  define XMD_H
#endif // WIN32 && __CYGWIN__


extern "C"
{
#include <jpeglib.h>
}


//
// 'jpeg_error_handler()' - Handle JPEG errors by not exiting.
//

static void
jpeg_error_handler(j_common_ptr)
{
  return;
}


//
// 'Fl_EXIF_Data::Fl_EXIF_Data()' - Load EXIF data from an image file.
//

Fl_EXIF_Data::Fl_EXIF_Data(const char *filename)// I - File to load
{
  FILE				*fp;		// File pointer
  struct jpeg_decompress_struct	cinfo;		// Decompressor info
  struct jpeg_error_mgr		jerr;		// Error handler info
  jpeg_saved_marker_ptr		marker;		// Pointer to marker data
  unsigned char			header[16];	// First 16 bytes of file


  // Initialize class data...
  comments_    = 0;
  exif_data_   = 0;
  exif_length_ = 0;
  exif_dir_    = 0;
  exif_count_  = 0;
  width_       = 0;
  height_      = 0;

  // Try opening the file...
  if ((fp = fopen(filename, "rb")) == NULL) return;

  // Check if this is a JPEG file...
  if (fread(header, 1, 16, fp) < 16)
  {
    fclose(fp);
    return;
  }

  rewind(fp);

  if (memcmp(header, "\377\330\377", 3) == 0 &&
					// Start-of-Image
      header[3] >= 0xe0 && header[3] <= 0xef)
	   				// APPn for JPEG file
  {
    // Yes, read the JPEG header and data...
    cinfo.err       = jpeg_std_error(&jerr);
    jerr.error_exit = jpeg_error_handler;

    jpeg_create_decompress(&cinfo);
    jpeg_save_markers(&cinfo, JPEG_APP0 + 1, 0xffff);
    jpeg_save_markers(&cinfo, JPEG_COM, 0xffff);
    jpeg_stdio_src(&cinfo, fp);
    jpeg_read_header(&cinfo, 1);

    // Save the image size...
    width_  = cinfo.image_width;
    height_ = cinfo.image_height;

    // Loop through the markers we are interested in...
    for (marker = cinfo.marker_list; marker; marker = marker->next)
    {
      switch (marker->marker)
      {
	case JPEG_COM :
            parse_comment(marker->data, marker->data_length);
	    break;
	case JPEG_APP0 + 1 :
            if (memcmp(marker->data, "Exif", 4) == 0)
              parse_exif(marker->data, marker->data_length);
	    break;
      }
    }

    // Finish up...
    jpeg_destroy_decompress(&cinfo);
  }

  // Close the file and return...
#ifdef DEBUG
  if (comments_)
  {
    puts("Comment:");
    puts(comments_);
  }
#endif // DEBUG

  fclose(fp);
}


//
// 'Fl_EXIF_Data::~Fl_EXIF_Data()' - Destroy EXIF data.
//

Fl_EXIF_Data::~Fl_EXIF_Data()
{
  if (comments_)
    delete[] comments_;
  if (exif_data_)
    delete[] exif_data_;
  if (exif_dir_)
    delete[] exif_dir_;
}


//
// 'Fl_EXIF_Data::get_ascii()' - Get a string value.
//

const char *
Fl_EXIF_Data::get_ascii(unsigned tag)
{
  const IFD	*ifd;


  if ((ifd = find_ifd(tag)) == NULL)
    return (0);

#ifdef DEBUG
  printf("get_ascii(tag=0x%x): type=%u...\n", tag, ifd->type);
#endif // DEBUG

  if (ifd->type != TYPE_ASCII)
    return (0);

#ifdef DEBUG
  printf("    offset=%u, exif_length_=%u...\n", ifd->offset, exif_length_);
#endif // DEBUG

  if (ifd->offset >= exif_length_)
    return (0);

  return ((const char *)(exif_data_ + ifd->offset));
}


//
// 'Fl_EXIF_Data::get_binary()' - Get a binary data value.
//

const unsigned char *
Fl_EXIF_Data::get_binary(unsigned tag,
                         unsigned &length)
{
  const IFD	*ifd;


  length = 0;

  if ((ifd = find_ifd(tag)) == NULL)
    return (0);

#ifdef DEBUG
  printf("get_binary(tag=0x%x): type=%u...\n", tag, ifd->type);
#endif // DEBUG

  if (ifd->type != TYPE_UNDEFINED)
    return (0);

#ifdef DEBUG
  printf("    offset=%u, exif_length_=%u...\n", ifd->offset, exif_length_);
#endif // DEBUG

  if (ifd->count <= 4)
  {
    length = ifd->count;

    return ((unsigned char *)&ifd->offset);
  }
  else if ((ifd->offset + ifd->count) > exif_length_)
    return (0);

  length = ifd->count;

  return (exif_data_ + ifd->offset);
}


//
// 'Fl_EXIF_Data::get_integer()' - Get an integer value.
//

int
Fl_EXIF_Data::get_integer(unsigned tag)	// I - Tag to lookup
{
  const IFD	*ifd;			// Pointer to matching tag


  if ((ifd = find_ifd(tag)) == NULL)
    return (-1);

#ifdef DEBUG
  printf("get_integer(tag=0x%x): type=%d, offset=%d, count=%d\n", tag,
         ifd->type, ifd->offset, ifd->count);
#endif // DEBUG

  if (ifd->type != TYPE_BYTE && ifd->type != TYPE_SHORT &&
      ifd->type != TYPE_LONG && ifd->type != TYPE_SLONG)
    return (-1);

  if (ifd->count == 1)
  {
    // If the data field is a short, return the high 16 bits, if the
    // data field is a byte, return the high 8 bits...
    if (ifd->type == TYPE_BYTE)
    {
      if (ifd->offset & 0xff000000)
	return ((int)(ifd->offset >> 24));
      else
	return ((int)(ifd->offset & 0xff));
    }
    else if (ifd->type == TYPE_SHORT)
    {
      if (ifd->offset & 0xffff0000)
	return ((int)(ifd->offset >> 16));
      else
	return ((int)(ifd->offset & 0xffff));
    }
    else
      return ((int)ifd->offset);
  }
  else if (ifd->type == TYPE_BYTE)
  {
    if (ifd->offset < exif_length_)
      return ((int)exif_data_[ifd->offset]);
    else
      return (-1);
  }
  else if (ifd->type == TYPE_SHORT)
    return ((int)get_ushort(ifd->offset));
  else
    return (get_sint(ifd->offset));
}


//
// 'Fl_EXIF_Data::get_rational()' - Get a rational value.
//

double
Fl_EXIF_Data::get_rational(unsigned tag,
                           int      &numerator,
			   unsigned &denominator)
{
  const IFD	*ifd;


  if ((ifd = find_ifd(tag)) == NULL)
  {
    numerator   = 0;
    denominator = 1;
    return (-1.0);
  }

#ifdef DEBUG
  printf("get_integer(tag=0x%x): type=%d, offset=%d, count=%d\n", tag,
         ifd->type, ifd->offset, ifd->count);
#endif // DEBUG

  if (ifd->type != TYPE_RATIONAL && ifd->type != TYPE_SRATIONAL)
  {
    numerator   = 0;
    denominator = 1;
    return (-1.0);
  }

  numerator   = get_sint(ifd->offset);
  denominator = get_uint(ifd->offset + 4);

  if (denominator)
    return ((double)numerator / (double)denominator);
  else
    return (0.0);
}


//
// 'Fl_EXIF_Data::get_ushort()' - Get a 16-bit unsigned integer.
//

unsigned short
Fl_EXIF_Data::get_ushort(unsigned offset)
{
  unsigned char	*p;


  if (offset > (exif_length_ - 1))
    return (0);

  p = exif_data_ + offset;

  if (exif_data_[0] == 'I')
    return ((p[1] << 8) | p[0]);
  else
    return ((p[0] << 8) | p[1]);
}


//
// 'Fl_EXIF_Data::get_uint()' - Get a 32-bit unsigned integer.
//

unsigned
Fl_EXIF_Data::get_uint(unsigned offset)
{
  unsigned char	*p;


  if (offset > (exif_length_ - 3))
    return (0);

  p = exif_data_ + offset;

  if (exif_data_[0] == 'I')
    return ((((((p[3] << 8) | p[2]) << 8) | p[1]) << 8) | p[0]);
  else
    return ((((((p[0] << 8) | p[1]) << 8) | p[2]) << 8) | p[3]);
}


//
// 'Fl_EXIF_Data::get_sint()' - Get a 32-bit signed integer.
//

int
Fl_EXIF_Data::get_sint(unsigned offset)
{
  unsigned char	*p;


  if (offset > (exif_length_ - 3))
    return (0);

  p = exif_data_ + offset;

  if (exif_data_[0] == 'I')
    return ((int)((((((p[3] << 8) | p[2]) << 8) | p[1]) << 8) | p[0]));
  else
    return ((int)((((((p[0] << 8) | p[1]) << 8) | p[2]) << 8) | p[3]));
}


//
// 'Fl_EXIF_Data::compare_ifds()' - Compare two directory entries.
//

int
Fl_EXIF_Data::compare_ifds(const IFD *a,
                           const IFD *b)
{
  return (a->tag - b->tag);
}


//
// 'Fl_EXIF_Data::find_ifd()' - Find a directory entry.
//

Fl_EXIF_Data::IFD *
Fl_EXIF_Data::find_ifd(unsigned short tag)
{
#if 1
  IFD	key;


  if (!exif_data_)
    return (0);

  key.tag = tag;
  return ((IFD *)bsearch(&key, exif_dir_, exif_count_, sizeof(IFD),
                         (int (*)(const void *, const void *))compare_ifds));
#else
  IFD	*ifd;
  int	i;

  printf("find_ifd(tag=0x%x)\n", tag);

  for (i = 0, ifd = exif_dir_; i < exif_count_; i ++, ifd ++)
  {
    printf("exif_dir_[%d].tag = 0x%x\n", i, ifd->tag);

    if (ifd->tag == tag)
    {
      printf("Returning %p...\n", ifd);
      return (ifd);
    }
  }

  puts("returning NULL...");
  return (NULL);
#endif // 0
}


//
// 'Fl_EXIF_Data::get_ifd()' - Get a tag directory.
//

Fl_EXIF_Data::IFD *
Fl_EXIF_Data::get_ifd(unsigned offset,
                      int      &num_ifds)
{
  int		i;
  IFD		*ifds,
		*ifd;


#ifdef DEBUG
  printf("get_ifd(offset=%u, ...)\n", offset);
#endif // DEBUG

  // Read the tag directory...
  num_ifds = get_ushort(offset);
  ifds     = new IFD[num_ifds];
  offset   += 2;

#ifdef DEBUG
  printf("num_ifds = %d\n", num_ifds);
#endif // DEBUG

  for (i = 0, ifd = ifds; i < num_ifds; i ++, ifd ++, offset += 12)
  {
    ifd->tag    = get_ushort(offset);
    ifd->type   = get_ushort(offset + 2);
    ifd->count  = get_uint(offset + 4);
    ifd->offset = get_uint(offset + 8);

#ifdef DEBUG
    printf("ifds[%d]: tag=0x%04X, type=0x%04X, count=%d, offset=%d\n", i,
           ifd->tag, ifd->type, ifd->count, ifd->offset);
#endif // DEBUG
  }

  return (ifds);
}


//
// 'Fl_EXIF_Data::parse_comment()' - Parse comment data in an EXIF image.
//

void
Fl_EXIF_Data::parse_comment(const unsigned char *data,	// I - Data
                            unsigned            length)	// I - Length of data
{
  char		*temp;					// Temporary string
  unsigned	templen;				// Temporary length


  if (comments_)
    templen = strlen(comments_) + 2;
  else
    templen = 1;

  templen += length;

  temp = new char[templen];

  if (comments_)
  {
    sprintf(temp, "%s\n", comments_);
    memcpy(temp + templen - length - 1, data, length);
    delete[] comments_;
  }
  else
    memcpy(temp, data, length);

  temp[templen - 1] = '\0';

  comments_ = temp;
}


//
// 'Fl_EXIF_Data::parse_exif()' - Parse EXIF data in an EXIF image.
//

void
Fl_EXIF_Data::parse_exif(const unsigned char *data,
                         unsigned            length)
{
  int		i;
  IFD		*ifd,
		*ifds;
  int		num_ifds,
		total_ifds;
  unsigned	offset;


  // Copy the EXIF data...
  data     += 6;
  length   -= 6;

  if (exif_data_)
  {
    delete[] exif_data_;
    exif_data_ = 0;
  }

  if (exif_dir_)
  {
    delete[] exif_dir_;
    exif_dir_ = 0;
  }

  exif_data_ = new unsigned char[length];
  memcpy(exif_data_, data, length);
  exif_length_ = length;

  // Read the primary tag directory...
  ifds = get_ifd(get_uint(4), num_ifds);

  total_ifds = num_ifds;

  // See if we have an EXIF tag directory...
  offset = 0;
  for (i = 0, ifd = ifds; i < num_ifds; i ++, ifd ++)
    if (ifd->tag == TAG_EXIF_IFD && ifd->type == TYPE_LONG)
    {
      offset     = ifd->offset;
      total_ifds += get_ushort(offset);
      break;
    }

  exif_dir_   = new IFD[total_ifds];
  exif_count_ = total_ifds;

  memcpy(exif_dir_, ifds, sizeof(IFD) * num_ifds);
  delete[] ifds;

  if (total_ifds > num_ifds)
  {
    // Read the EXIF tag directory...
    ifd  = exif_dir_ + num_ifds;
    ifds = get_ifd(offset, num_ifds);

    memcpy(ifd, ifds, sizeof(IFD) * num_ifds);
    delete[] ifds;

    qsort(exif_dir_, exif_count_, sizeof(IFD),
          (int (*)(const void *, const void *))compare_ifds);
  }

#ifdef DEBUG
  printf("exif_count_ = %d\n", exif_count_);

  for (i = 0, ifd = exif_dir_; i < exif_count_; i ++, ifd ++, offset += 12)
  {
    printf("exif_dir_[%d]: tag=0x%04X, type=0x%04X, count=%d, offset=%d\n", i,
           ifd->tag, ifd->type, ifd->count, ifd->offset);
  }
#endif // DEBUG
}


//
// End of "$Id: Fl_EXIF_Data.cxx 321 2005-01-23 03:52:44Z easysw $".
//
