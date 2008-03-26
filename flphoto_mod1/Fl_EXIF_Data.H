//
// "$Id: Fl_EXIF_Data.H 322 2005-01-23 03:55:19Z easysw $"
//
// EXIF data class header file for the Fast Light Tool Kit (FLTK).
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

#ifndef Fl_EXIF_Data_H
#  define Fl_EXIF_Data_H

class Fl_EXIF_Data
{
  public:

  struct IFD
  {
    unsigned short	tag;
    unsigned short	type;
    unsigned		count;
    unsigned		offset;
  };

  enum
  {
    TYPE_BYTE = 1,
    TYPE_ASCII = 2,
    TYPE_SHORT = 3,
    TYPE_LONG = 4,
    TYPE_RATIONAL = 5,
    TYPE_UNDEFINED = 7,
    TYPE_SLONG = 9,
    TYPE_SRATIONAL = 10
  };

  enum
  {
    ORIENT_TOP_LEFT = 1,		// Normal
    ORIENT_TOP_RIGHT = 2,
    ORIENT_BOTTOM_RIGHT = 3,		// Upside-down
    ORIENT_BOTTOM_LEFT = 4,
    ORIENT_LEFT_TOP = 5,
    ORIENT_RIGHT_TOP = 6,		// Rotated 90 counter-clockwize
    ORIENT_RIGHT_BOTTOM = 7,
    ORIENT_LEFT_BOTTOM = 8		// Rotated 90 clockwise
  };

  enum
  {
    // Additional directories...
    TAG_EXIF_IFD = 0x8769,
    TAG_GPS_IFD = 0x8825,
    TAG_INTEROPERABILITY_IFD = 0xA005,

    // Image data structure tags...
    TAG_IMAGE_WIDTH = 0x0100,
    TAG_IMAGE_HEIGHT = 0x0101,
    TAG_BITS_PER_SAMPLE = 0x0102,
    TAG_COMPRESSION = 0x0103,
    TAG_PHOTOMETRIC_INTERPRETATION = 0x0106,
    TAG_ORIENTATION = 0x0112,
    TAG_SAMPLES_PER_PIXEL = 0x0115,
    TAG_PLANAR_CONFIGURATION = 0x011C,
    TAG_YCBCR_SUB_SAMPLING = 0x0212,
    TAG_YCBCR_POSITIONING = 0x0213,
    TAG_X_RESOLUTION = 0x011A,
    TAG_Y_RESOLUTION = 0x011B,
    TAG_RESOLUTION_UNIT = 0x0128,

    // Recording offset tags...
    TAG_STRIP_OFFSETS = 0x0111,
    TAG_ROWS_PER_STRIP = 0x0116,
    TAG_STRIP_BYTE_COUNTS = 0x0117,
    TAG_JPEG_INTERCHANGE_FORMAT = 0x0201,
    TAG_JPEG_INTERCHANGE_FORMAT_LENGTH = 0x0202,

    // Image data characteristic tags...
    TAG_TRANSFER_FUNCTION = 0x012D,
    TAG_WHITE_POINT = 0x013E,
    TAG_PRIMARY_CHROMATICITIES = 0x013F,
    TAG_YCBCR_COEFFICIENTS = 0x0211,
    TAG_REFERENCE_BLACK_WHITE = 0x0214,

    // Other tags...
    TAG_DATE_TIME = 0x0132,
    TAG_IMAGE_DESCRIPTION = 0x010E,
    TAG_MAKE = 0x010F,
    TAG_MODEL = 0x0110,
    TAG_SOFTWARE = 0x0131,
    TAG_ARTIST = 0x013B,
    TAG_COPYRIGHT = 0x8298,

    // Kodak EXIF info...
    TAG_KODAK_DATE_TIME_ORIGINAL = 0x9003,
    TAG_KODAK_DATE_TIME_DIGITIZED = 0x9004,

    // Canon EXIF info...
    TAG_CANON_CUSTOM_RENDERED = 0xa401,
    TAG_CANON_EXPOSURE_MODE = 0xa402,
    TAG_CANON_WHITE_BALANCE = 0xa403,
    TAG_CANON_DIGITAL_ZOOM_RATIO = 0xa404,
    TAG_CANON_FOCAL_LENGTH_35MM = 0xa405,
    TAG_CANON_SCENE_CAPTURE_TYPE = 0xa406,
    TAG_CANON_GAIN_CONTROL = 0xa407,
    TAG_CANON_CONTRAST = 0xa408,
    TAG_CANON_SATURATION = 0xa409,
    TAG_CANON_SHARPNESS = 0xa40a,
    TAG_CANON_DEVICE_SETTING_DESCRIPTION = 0xa40b,
    TAG_CANON_SUBJECT_DISTANCE_RANGE = 0xa40c,
    TAG_CANON_IMAGE_UNIQUE_ID = 0xa420,

    // EXIF tags...
    TAG_EXIF_VERSION = 0x9000,
    TAG_FLASH_PIX_VERSION = 0xA000,
    TAG_COLOR_SPACE = 0xA001,
    TAG_COMPONENT_CONFIGURATION = 0x9101,
    TAG_COMPRESSED_BITS_PER_PIXEL = 0x9102,
    TAG_PIXEL_X_DIMENSION = 0xA002,
    TAG_PIXEL_Y_DIMENSION = 0xA003,
    TAG_MAKER_NOTE = 0x927C,
    TAG_USER_COMMENTS = 0x9286,
    TAG_EXPOSURE_TIME = 0x829A,
    TAG_F_NUMBER = 0x829D,
    TAG_EXPOSURE_PROGRAM = 0x8822,
    TAG_SPECTRAL_SENSITIVITY = 0x8824,
    TAG_ISO_SPEED_RATINGS = 0x8827,
    TAG_OECF = 0x8828,
    TAG_SHUTTER_SPEED_VALUE = 0x9201,
    TAG_APERTURE_VALUE = 0x9202,
    TAG_BRIGHTNESS_VALUE = 0x9203,
    TAG_EXPOSURE_BIAS_VALUE = 0x9204,
    TAG_MAX_APERTURE_VALUE = 0x9205,
    TAG_SUBJECT_DISTANCE = 0x9206,
    TAG_METERING_MODE = 0x9207,
    TAG_LIGHT_SOURCE = 0x9208,
    TAG_FLASH = 0x9209,
    TAG_FOCAL_LENGTH = 0x920A,
    TAG_FLASH_ENERGY = 0xA20B,
    TAG_SPATIAL_FREQUENCY_RESPONSE = 0xA20C,
    TAG_FOCAL_PLANE_X_RESOLUTION = 0xA20E,
    TAG_FOCAL_PLANE_Y_RESOLUTION = 0xA20F,
    TAG_FOCAL_POLANE_RESOLUTION_UNIT = 0xA210,
    TAG_SUBJECT_LOCATION = 0xA214,
    TAG_EXPOSURE_INDEX = 0xA215,
    TAG_SENSING_METHOD = 0xA217,
    TAG_FILE_SOURCE = 0xA300,
    TAG_SCENE_TYPE = 0xA301,
    TAG_CFA_PATTERN = 0xA302
  };

  private:

  int		width_,
		height_;
  char		*comments_;
  unsigned char	*exif_data_;
  unsigned	exif_length_;
  int		exif_count_;
  IFD		*exif_dir_;

  void		parse_comment(const unsigned char *data, unsigned length);
  void		parse_exif(const unsigned char *data, unsigned length);

  unsigned short get_ushort(unsigned offset);
  unsigned	get_uint(unsigned offset);
  int		get_sint(unsigned offset);

  static int	compare_ifds(const IFD *a, const IFD *b);
  IFD		*find_ifd(unsigned short tag);
  IFD		*get_ifd(unsigned offset, int &num_ifds);

  public:

  Fl_EXIF_Data(const char* filename);
  ~Fl_EXIF_Data();

  const char		*comments() { return comments_; }
  const unsigned char	*exif_data() { return exif_data_; }
  unsigned		exif_length() const { return exif_length_; }
  const IFD		*exif_dir() { return exif_dir_; }
  int			exif_count() const { return exif_count_; }

  const char		*get_ascii(unsigned tag);
  const unsigned char	*get_binary(unsigned tag, unsigned &length);
  int			get_integer(unsigned tag);
  double		get_rational(unsigned tag) {
			  int numerator;
			  unsigned denominator;
			  return get_rational(tag, numerator, denominator);
			}
  double		get_rational(unsigned tag, int &numerator,
			             unsigned &denominator);

  int			width() const { return (width_); }
  int			height() const { return (height_); }
};

#endif // !Fl_EXIF_Data_H

//
// End of "$Id: Fl_EXIF_Data.H 322 2005-01-23 03:55:19Z easysw $".
//
