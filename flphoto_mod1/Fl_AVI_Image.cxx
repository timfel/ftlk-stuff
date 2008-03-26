//
// "$Id: Fl_AVI_Image.cxx 387 2006-05-22 00:04:53Z mike $"
//
// AVI image routines for the Fast Light Tool Kit (FLTK).
//
// Copyright 2004 by Michael Sweet.
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

#include "Fl_AVI_Image.H"
extern "C" {
#include <jpeglib.h>
}
#include <stdlib.h>
#include <string.h>


//
// Source manager for JPEG data...
//

typedef struct
{
  struct jpeg_source_mgr src;		// Source manager data
  unsigned char		*data;		// Buffer
  unsigned		length;		// Length of buffer
} jpeg_mem_mgr_t;


//
// Handlers for JPEG files...
//

//
// 'fl_jpeg_error_exit()' - Error handler.
//

static void
fl_jpeg_error_exit(j_common_ptr)
{
}


//
// 'fl_jpeg_init_source()' - Initialize source.
//

static void
fl_jpeg_init_source(j_decompress_ptr dinfo)	// I - Decompressor info
{
  jpeg_mem_mgr_t	*mm;		// Memory manager


  mm = (jpeg_mem_mgr_t *)dinfo->src;

  mm->src.next_input_byte = NULL;
  mm->src.bytes_in_buffer = 0;
}


//
// 'fl_jpeg_fill_input_buffer()' - Fill the input buffer.
//

static boolean
fl_jpeg_fill_input_buffer(j_decompress_ptr dinfo)
					// I - Decompressor info
{
  jpeg_mem_mgr_t	*mm;		// Memory manager


  mm = (jpeg_mem_mgr_t *)dinfo->src;

  if (mm->src.next_input_byte)
  {
    mm->src.next_input_byte = (JOCTET *)"\377\331";
    mm->src.bytes_in_buffer = 2;
  }
  else
  {
    mm->src.next_input_byte = mm->data;
    mm->src.bytes_in_buffer = mm->length;
  }

  return (TRUE);
}


//
// 'fl_jpeg_skip_input_data()' - Skip data in buffer.
//

static void
fl_jpeg_skip_input_data(j_decompress_ptr dinfo,
					// I - Decompressor info
                        long         num_bytes)
					// I - Number of bytes to skip
{
  jpeg_mem_mgr_t	*mm;		// Memory manager


  mm = (jpeg_mem_mgr_t *)dinfo->src;

  if (num_bytes <= (long)mm->src.bytes_in_buffer)
  {
    mm->src.bytes_in_buffer -= num_bytes;
    mm->src.next_input_byte += num_bytes;
  }
  else
  {
    mm->src.next_input_byte = (JOCTET *)"\377\331";
    mm->src.bytes_in_buffer = 2;
  }
}


//
// 'fl_jpeg_term_source()' - Terminate source.
//

static void
fl_jpeg_term_source(j_decompress_ptr)
{
}


//
// 'fl_add_huff_table()' - Add a Huffman compression table.
//
// Copied from mjpegtools 1.6.2...
//

void
fl_add_huff_table(j_decompress_ptr dinfo,
					// I - Decompressor info
		  JHUFF_TBL        **htblptr,
					// O - Huffman tale 
		  const UINT8      *bits,
					// I - Bit allocations
		  const UINT8      *val)// I - Values
{
  int	nsymbols,			// Number of symbols
	len;				// Length of symbols


  // Make sure we have a huffman table...
  if (!*htblptr)
    *htblptr = jpeg_alloc_huff_table((j_common_ptr)dinfo);

  // Copy the number-of-symbols-of-each-code-length counts
  memcpy((*htblptr)->bits, bits, sizeof((*htblptr)->bits));

  // Validate the counts.  We do this here mainly so we can copy the right
  // number of symbols from the val[] array, without risking marching off
  // the end of memory.  jchuff.c will do a more thorough test later.
  for (nsymbols = 0, len = 1; len <= 16; len ++)
    nsymbols += bits[len];

  if (nsymbols > 0 && nsymbols <= 256)
    memcpy((*htblptr)->huffval, val, nsymbols * sizeof(UINT8));
}


//
// 'fl_std_huff_tables()' - Load the standard Huffman tables...
//
// Copied from mjpegtools 1.6.2...
//

void
fl_std_huff_tables(j_decompress_ptr dinfo)
					// I - Decompressor info
{
  static const UINT8	bits_dc_luminance[17] =
    { /* 0-base */ 0, 0, 1, 5, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0 };
  static const UINT8	val_dc_luminance[] =
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
  static const UINT8	bits_dc_chrominance[17] =
    { /* 0-base */ 0, 0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0 };
  static const UINT8	val_dc_chrominance[] =
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
  static const UINT8	bits_ac_luminance[17] =
    { /* 0-base */ 0, 0, 2, 1, 3, 3, 2, 4, 3, 5, 5, 4, 4, 0, 0, 1, 0x7d };
  static const UINT8	val_ac_luminance[] =
    { 0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12,
      0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07,
      0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xa1, 0x08,
      0x23, 0x42, 0xb1, 0xc1, 0x15, 0x52, 0xd1, 0xf0,
      0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0a, 0x16,
      0x17, 0x18, 0x19, 0x1a, 0x25, 0x26, 0x27, 0x28,
      0x29, 0x2a, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
      0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
      0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
      0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
      0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
      0x7a, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
      0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
      0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
      0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6,
      0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5,
      0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4,
      0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe1, 0xe2,
      0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea,
      0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
      0xf9, 0xfa };
  static const UINT8	bits_ac_chrominance[17] =
    { /* 0-base */ 0, 0, 2, 1, 2, 4, 4, 3, 4, 7, 5, 4, 4, 0, 1, 2, 0x77 };
  static const UINT8	val_ac_chrominance[] =
    { 0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21,
      0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71,
      0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91,
      0xa1, 0xb1, 0xc1, 0x09, 0x23, 0x33, 0x52, 0xf0,
      0x15, 0x62, 0x72, 0xd1, 0x0a, 0x16, 0x24, 0x34,
      0xe1, 0x25, 0xf1, 0x17, 0x18, 0x19, 0x1a, 0x26,
      0x27, 0x28, 0x29, 0x2a, 0x35, 0x36, 0x37, 0x38,
      0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
      0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
      0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
      0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
      0x79, 0x7a, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
      0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95, 0x96,
      0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5,
      0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4,
      0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3,
      0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2,
      0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda,
      0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9,
      0xea, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
      0xf9, 0xfa };

  
  fl_add_huff_table(dinfo, &dinfo->dc_huff_tbl_ptrs[0],
	 	    bits_dc_luminance, val_dc_luminance);
  fl_add_huff_table(dinfo, &dinfo->ac_huff_tbl_ptrs[0],
		    bits_ac_luminance, val_ac_luminance);
  fl_add_huff_table(dinfo, &dinfo->dc_huff_tbl_ptrs[1],
		    bits_dc_chrominance, val_dc_chrominance);
  fl_add_huff_table(dinfo, &dinfo->ac_huff_tbl_ptrs[1],
		    bits_ac_chrominance, val_ac_chrominance);
}


//
// 'Fl_AVI_Image::Fl_AVI_Image()' - Load an AVI image from a filename.
//

Fl_AVI_Image::Fl_AVI_Image(const char *filename)
					// I - Filename
  : Fl_RGB_Image(0,0,0)
{
  FILE	*fp;				// File pointer


  if ((fp = fopen(filename, "rb")) != NULL)
  {
    load_image(fp);
    fclose(fp);
  }
}


//
// 'Fl_AVI_Image::Fl_AVI_Image()' - Load an AVI image from a FILE *.
//

Fl_AVI_Image::Fl_AVI_Image(FILE *fp)	// I - File pointer
  : Fl_RGB_Image(0,0,0)
{
  load_image(fp);
}


//
// 'Fl_AVI_Image::check()' - Check for an AVI file.
//

Fl_Image *				// O - New image, if any
Fl_AVI_Image::check(const char *name,	// I - Filename
                    uchar      *header,	// I - Header data
		    int        headerlen)
					// I - Length of header data
{
  if (!memcmp(header, "RIFF", 4) && !memcmp(header + 8, "AVI ", 4))
    return (new Fl_AVI_Image(name));
  else
    return (0);
}


//
// 'Fl_AVI_Image::get_chunk()' - Get a chunk from the file.
//

char *					// O - Chunk name or NULL on EOF
Fl_AVI_Image::get_chunk(FILE     *fp,	// I - File pointer
                        char     name[5],
					// O - Chunk name
			unsigned &length)
					// O - Chunk length
{
  int	i,				// Looping var
	ch;				// Character from file


  name[4] = '\0';
  length  = 0;

  if (fread(name, 4, 1, fp) < 1)
    return (NULL);

  for (i = 0; i < 4; i ++)
    if ((ch = getc(fp)) == EOF)
      return (NULL);
    else
      length |= ch << (i * 8);

  return (name);
}


//
// 'Fl_AVI_Image::load_frame()' - Load a frame of image data...
//

void
Fl_AVI_Image::load_frame(FILE     *fp,	// I - File pointer
                         unsigned length)
					// I - Length of chunk
{
  unsigned char	*data;			// Image data
  struct jpeg_decompress_struct dinfo;	// Decompressor info
  struct jpeg_error_mgr jerr;		// Error handler info
  jpeg_mem_mgr_t jsrc;			// Source handler info
  JSAMPROW	row;			// Sample row pointer


#ifdef DEBUG
  unsigned	i;			// Looping var

  printf("Fl_AVI_Image::load_frame(fp=%p, length=%u)\n", fp, length);
#endif // DEBUG

  data = new unsigned char[length];

  if (fread(data, length, 1, fp) < 1)
  {
#ifdef DEBUG
    puts("    read error(00dc)!");
#endif // DEBUG
    return;
  }

#ifdef DEBUG
  for (i = 0; i < length; i ++)
  {
    if ((i & 15) == 0)
      printf("%08x ", i);

    printf(" %02x", data[i]);

    if ((i & 15) == 15)
      putchar('\n');
  }

  if (i & 15)
    putchar('\n');
#endif // DEBUG

  dinfo.err       = jpeg_std_error(&jerr);
  jerr.error_exit = fl_jpeg_error_exit;

  jpeg_create_decompress(&dinfo);

  memset(&jsrc, 0, sizeof(jsrc));
  jsrc.data                  = data;
  jsrc.length                = length;
  jsrc.src.init_source       = fl_jpeg_init_source;
  jsrc.src.fill_input_buffer = fl_jpeg_fill_input_buffer;
  jsrc.src.skip_input_data   = fl_jpeg_skip_input_data;
  jsrc.src.resync_to_restart = jpeg_resync_to_restart;
  jsrc.src.term_source       = fl_jpeg_term_source;

  dinfo.src = (struct jpeg_source_mgr *)&jsrc;

  jpeg_read_header(&dinfo, 1);

  dinfo.quantize_colors      = (boolean)FALSE;
  dinfo.out_color_space      = JCS_RGB;
  dinfo.out_color_components = 3;
  dinfo.output_components    = 3;

  jpeg_calc_output_dimensions(&dinfo);

  w(dinfo.output_width);
  h(dinfo.output_height);
  d(dinfo.output_components);

  array = new uchar[w() * h() * d()];
  alloc_array = 1;

  if (!dinfo.dc_huff_tbl_ptrs[0] && !dinfo.dc_huff_tbl_ptrs[1] &&
      !dinfo.ac_huff_tbl_ptrs[0] && !dinfo.ac_huff_tbl_ptrs[1])
    fl_std_huff_tables(&dinfo);

  jpeg_start_decompress(&dinfo);

  while (dinfo.output_scanline < dinfo.output_height)
  {
    row = (JSAMPROW)(array +
                     dinfo.output_scanline * dinfo.output_width *
                     dinfo.output_components);
    jpeg_read_scanlines(&dinfo, &row, (JDIMENSION)1);
  }

  jpeg_finish_decompress(&dinfo);
  jpeg_destroy_decompress(&dinfo);

  delete[] data;
}


//
// 'Fl_AVI_Image::load_image()' - Load an AVI file.
//

void
Fl_AVI_Image::load_image(FILE *fp)	// I - File pointer
{
  char		name[5];		// Chunk name
  unsigned	length;			// Chunk length
  long		chunkpos;		// File position


#ifdef DEBUG
  printf("Fl_AVI_Image::load_image(fp=%p)\n", fp);
#endif // DEBUG

  if (get_chunk(fp, name, length) == NULL)
  {
#ifdef DEBUG
    puts("    read error (RIFF)!");
#endif // DEBUG
    return;
  }
#ifdef DEBUG
  else
    printf("    '%s', %u bytes...\n", name, length);
#endif // DEBUG

  if (fread(name, 4, 1, fp) < 1)
  {
#ifdef DEBUG
    puts("    read error (AVI )!");
#endif // DEBUG
    return;
  }

  while (get_chunk(fp, name, length))
  {
#ifdef DEBUG
    printf("%08x: %s, %u bytes...\n", (unsigned)ftell(fp) - 8, name, length);
#endif // DEBUG

    chunkpos = ftell(fp) + length;

    if (!strcmp(name, "LIST"))
      load_list(fp, chunkpos);

    if (ftell(fp) != chunkpos)
      fseek(fp, chunkpos, SEEK_SET);

    if (array)
      break;
  }
}


//
// 'Fl_AVI_Image::load_list()' - Load a LIST chunk from an AVI file.
//

void
Fl_AVI_Image::load_list(FILE *fp,	// I - File pointer
                        long pos)	// I - End file position
{
  char		name[5];		// Chunk name
  unsigned	length;			// Chunk length
  long		chunkpos;		// File position


#ifdef DEBUG
  printf("Fl_AVI_Image::load_list(fp=%p, pos=%ld)\n", fp, pos);
#endif // DEBUG

  name[4] = '\0';

  if (fread(name, 4, 1, fp) < 1)
  {
#ifdef DEBUG
    puts("    read error (LIST)!");
#endif // DEBUG
    return;
  }

#ifdef DEBUG
  printf("name='%s'...\n", name);
#endif // DEBUG

  while (get_chunk(fp, name, length))
  {
#ifdef DEBUG
    printf("%08x: %s, %u bytes...\n", (unsigned)ftell(fp) - 8, name, length);
#endif // DEBUG

    chunkpos = ftell(fp) + length;

    if (!strcmp(name, "LIST"))
      load_list(fp, chunkpos);
    else if (!strcmp(name, "00dc"))
      load_frame(fp, length);

    if (ftell(fp) != chunkpos)
      fseek(fp, chunkpos, SEEK_SET);

    if (chunkpos >= pos || array)
      break;
  }
}


//
// End of "$Id: Fl_AVI_Image.cxx 387 2006-05-22 00:04:53Z mike $".
//
