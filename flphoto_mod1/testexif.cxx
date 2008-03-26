//
// "$Id: testexif.cxx 322 2005-01-23 03:55:19Z easysw $"
//
// Test code for the Fl_EXIF_Data class.
//
// Copyright 2004-2005 by Michael Sweet.
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
//   main()      - Show EXIF information.
//   tag_name()  - Return the tag name...
//   type_name() - Return the type name...
//

//
// Include necessary headers...
//

#include <stdio.h>
#include <string.h>
#include "Fl_EXIF_Data.H"


//
// Local functions...
//

const char	*tag_name(unsigned short tag);
const char	*type_name(unsigned short tag);


//
// 'main()' - Show EXIF information.
//

int					// O - Exit status
main(int  argc,				// I - Number of command-line args
     char *argv[])			// I - Command-line arguments
{
  Fl_EXIF_Data		*exif;		// EXIF data
  const Fl_EXIF_Data::IFD *ifd;		// Image file directory
  int			count;		// Count of directory entries
  unsigned		i;		// Looping var
  const char		*s;		// Temporary string
  const unsigned char	*d;		// Temporary data
  unsigned		length;		// Length of data
  double		val;		// Rational value


  if (argc != 2)
  {
    puts("Usage: testexif filename");
    return (1);
  }

  exif = new Fl_EXIF_Data(argv[1]);

#if 0
  if ((d = exif->exif_data()) !=  NULL)
    printf("Data: %02x %02x %02x %02x %02x %02x %02x %02x\n",
	   d[0], d[1], d[2], d[3], d[4], d[5], d[6], d[7]);

  if ((s = exif->get_ascii(Fl_EXIF_Data::TAG_DATE_TIME)) != NULL)
    printf("Date Time: %s\n", s);
  if ((s = exif->get_ascii(Fl_EXIF_Data::TAG_IMAGE_DESCRIPTION)) != NULL)
    printf("Image Description: %s\n", s);
  if ((s = exif->get_ascii(Fl_EXIF_Data::TAG_MAKE)) != NULL)
    printf("Make: %s\n", s);
  if ((s = exif->get_ascii(Fl_EXIF_Data::TAG_MODEL)) != NULL)
    printf("Model: %s\n", s);
  if ((s = exif->get_ascii(Fl_EXIF_Data::TAG_SOFTWARE)) != NULL)
    printf("Software: %s\n", s);
  if ((s = exif->get_ascii(Fl_EXIF_Data::TAG_ARTIST)) != NULL)
    printf("Artist: %s\n", s);
  if ((s = exif->get_ascii(Fl_EXIF_Data::TAG_COPYRIGHT)) != NULL)
    printf("Copyright: %s\n", s);

  if ((d = exif->get_binary(Fl_EXIF_Data::TAG_EXIF_VERSION, length)) != NULL)
    printf("EXIF Version: %c%c%c%c\n", d[0], d[1], d[2], d[3]);

  printf("Width: %d\n", exif->get_integer(Fl_EXIF_Data::TAG_PIXEL_X_DIMENSION));
  printf("Height: %d\n", exif->get_integer(Fl_EXIF_Data::TAG_PIXEL_Y_DIMENSION));

  val = exif->get_rational(Fl_EXIF_Data::TAG_EXPOSURE_TIME);
  if (val < 1.0)
    printf("Exposure Time: 1/%.0fth second\n", 1.0 / val);
  else
    printf("Exposure Time: %.1f second(s)\n", val);

  printf("F Number: %.1f\n", exif->get_rational(Fl_EXIF_Data::TAG_F_NUMBER));
  printf("Exposure Program: %d\n", exif->get_integer(Fl_EXIF_Data::TAG_EXPOSURE_PROGRAM));
  if ((s = exif->get_ascii(Fl_EXIF_Data::TAG_SPECTRAL_SENSITIVITY)) != NULL)
    printf("Spectral Sensitivity: %s\n", s);
  printf("ISO Speed: %d\n", exif->get_integer(Fl_EXIF_Data::TAG_ISO_SPEED_RATINGS));
  printf("Shutter Speed: %f\n", exif->get_rational(Fl_EXIF_Data::TAG_SHUTTER_SPEED_VALUE));
  printf("Aperture: %.1f\n", exif->get_rational(Fl_EXIF_Data::TAG_APERTURE_VALUE));
  printf("Brightness: %f\n", exif->get_rational(Fl_EXIF_Data::TAG_BRIGHTNESS_VALUE));
  printf("Exposure Bias: %f\n", exif->get_rational(Fl_EXIF_Data::TAG_EXPOSURE_BIAS_VALUE));
  printf("Max Aperture: %.1f\n", exif->get_rational(Fl_EXIF_Data::TAG_MAX_APERTURE_VALUE));
  printf("Subject Distance: %f\n", exif->get_rational(Fl_EXIF_Data::TAG_SUBJECT_DISTANCE));
  printf("Metering Mode: %d\n", exif->get_integer(Fl_EXIF_Data::TAG_METERING_MODE));
  printf("Light Source: %d\n", exif->get_integer(Fl_EXIF_Data::TAG_LIGHT_SOURCE));
  printf("Flash: %d\n", exif->get_integer(Fl_EXIF_Data::TAG_FLASH));
  printf("Focal Length: %f\n", exif->get_rational(Fl_EXIF_Data::TAG_FOCAL_LENGTH));
  printf("Flash Energy: %f\n", exif->get_rational(Fl_EXIF_Data::TAG_FLASH_ENERGY));
  printf("Exposure Index: %f\n", exif->get_rational(Fl_EXIF_Data::TAG_EXPOSURE_INDEX));
  printf("Sensing Method: %d\n", exif->get_integer(Fl_EXIF_Data::TAG_SENSING_METHOD));

  puts("");

#endif // 0

  puts("Tag                             Type       Count  Value");
  puts("------------------------------  ---------  -----  -----------------------------");

  for (count = exif->exif_count(), ifd = exif->exif_dir();
       count > 0;
       count --, ifd ++)
  {
    printf("%-30s  %-9s  %5d  ", tag_name(ifd->tag), type_name(ifd->type),
           ifd->count);

    switch (ifd->type)
    {
      case Fl_EXIF_Data::TYPE_ASCII :
          if ((s = exif->get_ascii(ifd->tag)) != NULL)
	  {
	    if (strlen(s) > 27)
	      printf("\"%-25.25s...\n", s);
	    else
	      printf("\"%s\"\n", s);
	  }
	  else
	    puts("(null)");
          break;

      case Fl_EXIF_Data::TYPE_BYTE :
      case Fl_EXIF_Data::TYPE_SHORT :
      case Fl_EXIF_Data::TYPE_LONG :
      case Fl_EXIF_Data::TYPE_SLONG :
          printf("%d\n", exif->get_integer(ifd->tag));
          break;

      case Fl_EXIF_Data::TYPE_RATIONAL :
      case Fl_EXIF_Data::TYPE_SRATIONAL :
          val = exif->get_rational(ifd->tag);
	  if (val > 0.0f && val < 1.0f)
            printf("1/%.0f\n", 1.0 / val);
          else
	    printf("%f\n", val);
	  break;

      case Fl_EXIF_Data::TYPE_UNDEFINED :
          if ((d = exif->get_binary(ifd->tag, length)) != NULL)
	  {
	    for (i = 0; i < length && i < 28; i ++)
	      if (d[i] >= ' ')
	        putchar(d[i]);
	      else
	        putchar('.');

	    for (i = 0; i < length; i ++)
	    {
	      if ((i & 15) == 0)
	      {
	        if (i)
		  fputs(" |", stdout);

	        fputs("\n                                | ", stdout);
	      }

	      printf("%02x", d[i]);
            }

            while (i & 15)
	    {
	      fputs("  ", stdout);
	      i ++;
	    }

	    puts(" |");
	  }
	  else
	    puts("(null)");
          break;

      default :
          puts("???");
    }
  }

  delete exif;

  return (0);
}


//
// 'tag_name()' - Return the tag name...
//

const char *				// O - Tag name
tag_name(unsigned short tag)		// I - Tag
{
  static char	s[255];			// Temporary string


  switch (tag)
  {
    case Fl_EXIF_Data::TAG_EXIF_IFD :
	return ("EXIF_IFD");
    case Fl_EXIF_Data::TAG_GPS_IFD :
	return ("GPS_IFD");
    case Fl_EXIF_Data::TAG_INTEROPERABILITY_IFD :
	return ("INTEROPERABILITY_IFD");
    case Fl_EXIF_Data::TAG_IMAGE_WIDTH :
	return ("IMAGE_WIDTH");
    case Fl_EXIF_Data::TAG_IMAGE_HEIGHT :
	return ("IMAGE_HEIGHT");
    case Fl_EXIF_Data::TAG_BITS_PER_SAMPLE :
	return ("BITS_PER_SAMPLE");
    case Fl_EXIF_Data::TAG_COMPRESSION :
	return ("COMPRESSION");
    case Fl_EXIF_Data::TAG_PHOTOMETRIC_INTERPRETATION :
	return ("PHOTOMETRIC_INTERPRETATION");
    case Fl_EXIF_Data::TAG_ORIENTATION :
	return ("ORIENTATION");
    case Fl_EXIF_Data::TAG_SAMPLES_PER_PIXEL :
	return ("SAMPLES_PER_PIXEL");
    case Fl_EXIF_Data::TAG_PLANAR_CONFIGURATION :
	return ("PLANAR_CONFIGURATION");
    case Fl_EXIF_Data::TAG_YCBCR_SUB_SAMPLING :
	return ("YCBCR_SUB_SAMPLING");
    case Fl_EXIF_Data::TAG_YCBCR_POSITIONING :
	return ("YCBCR_POSITIONING");
    case Fl_EXIF_Data::TAG_X_RESOLUTION :
	return ("X_RESOLUTION");
    case Fl_EXIF_Data::TAG_Y_RESOLUTION :
	return ("Y_RESOLUTION");
    case Fl_EXIF_Data::TAG_RESOLUTION_UNIT :
	return ("RESOLUTION_UNIT");
    case Fl_EXIF_Data::TAG_STRIP_OFFSETS :
	return ("STRIP_OFFSETS");
    case Fl_EXIF_Data::TAG_ROWS_PER_STRIP :
	return ("ROWS_PER_STRIP");
    case Fl_EXIF_Data::TAG_STRIP_BYTE_COUNTS :
	return ("STRIP_BYTE_COUNTS");
    case Fl_EXIF_Data::TAG_JPEG_INTERCHANGE_FORMAT :
	return ("JPEG_INTERCHANGE_FORMAT");
    case Fl_EXIF_Data::TAG_JPEG_INTERCHANGE_FORMAT_LENGTH :
	return ("JPEG_INTERCHANGE_FORMAT_LENGTH");
    case Fl_EXIF_Data::TAG_TRANSFER_FUNCTION :
	return ("TRANSFER_FUNCTION");
    case Fl_EXIF_Data::TAG_WHITE_POINT :
	return ("WHITE_POINT");
    case Fl_EXIF_Data::TAG_PRIMARY_CHROMATICITIES :
	return ("PRIMARY_CHROMATICITIES");
    case Fl_EXIF_Data::TAG_YCBCR_COEFFICIENTS :
	return ("YCBCR_COEFFICIENTS");
    case Fl_EXIF_Data::TAG_REFERENCE_BLACK_WHITE :
	return ("REFERENCE_BLACK_WHITE");
    case Fl_EXIF_Data::TAG_DATE_TIME :
	return ("DATE_TIME");
    case Fl_EXIF_Data::TAG_IMAGE_DESCRIPTION :
	return ("IMAGE_DESCRIPTION");
    case Fl_EXIF_Data::TAG_MAKE :
	return ("MAKE");
    case Fl_EXIF_Data::TAG_MODEL :
	return ("MODEL");
    case Fl_EXIF_Data::TAG_SOFTWARE :
	return ("SOFTWARE");
    case Fl_EXIF_Data::TAG_ARTIST :
	return ("ARTIST");
    case Fl_EXIF_Data::TAG_COPYRIGHT :
	return ("COPYRIGHT");
    case Fl_EXIF_Data::TAG_KODAK_DATE_TIME_ORIGINAL :
	return ("KODAK_DATE_TIME_ORIGINAL");
    case Fl_EXIF_Data::TAG_KODAK_DATE_TIME_DIGITIZED :
	return ("KODAK_DATE_TIME_DIGITIZED");
    case Fl_EXIF_Data::TAG_CANON_CUSTOM_RENDERED :
	return ("CANON_CUSTOM_RENDERED");
    case Fl_EXIF_Data::TAG_CANON_EXPOSURE_MODE :
	return ("CANON_EXPOSURE_MODE");
    case Fl_EXIF_Data::TAG_CANON_WHITE_BALANCE :
	return ("CANON_WHITE_BALANCE");
    case Fl_EXIF_Data::TAG_CANON_DIGITAL_ZOOM_RATIO :
	return ("CANON_DIGITAL_ZOOM_RATIO");
    case Fl_EXIF_Data::TAG_CANON_FOCAL_LENGTH_35MM :
	return ("CANON_FOCAL_LENGTH_35MM");
    case Fl_EXIF_Data::TAG_CANON_SCENE_CAPTURE_TYPE :
	return ("CANON_SCENE_CAPTURE_TYPE");
    case Fl_EXIF_Data::TAG_CANON_GAIN_CONTROL :
	return ("CANON_GAIN_CONTROL");
    case Fl_EXIF_Data::TAG_CANON_CONTRAST :
	return ("CANON_CONTRAST");
    case Fl_EXIF_Data::TAG_CANON_SATURATION :
	return ("CANON_SATURATION");
    case Fl_EXIF_Data::TAG_CANON_SHARPNESS :
	return ("CANON_SHARPNESS");
    case Fl_EXIF_Data::TAG_CANON_DEVICE_SETTING_DESCRIPTION :
	return ("CANON_DEVICE_SETTING_DESCRIPTION");
    case Fl_EXIF_Data::TAG_CANON_SUBJECT_DISTANCE_RANGE :
	return ("CANON_SUBJECT_DISTANCE_RANGE");
    case Fl_EXIF_Data::TAG_CANON_IMAGE_UNIQUE_ID :
	return ("CANON_IMAGE_UNIQUE_ID");
    case Fl_EXIF_Data::TAG_EXIF_VERSION :
	return ("EXIF_VERSION");
    case Fl_EXIF_Data::TAG_FLASH_PIX_VERSION :
	return ("FLASH_PIX_VERSION");
    case Fl_EXIF_Data::TAG_COLOR_SPACE :
	return ("COLOR_SPACE");
    case Fl_EXIF_Data::TAG_COMPONENT_CONFIGURATION :
	return ("COMPONENT_CONFIGURATION");
    case Fl_EXIF_Data::TAG_COMPRESSED_BITS_PER_PIXEL :
	return ("COMPRESSED_BITS_PER_PIXEL");
    case Fl_EXIF_Data::TAG_PIXEL_X_DIMENSION :
	return ("PIXEL_X_DIMENSION");
    case Fl_EXIF_Data::TAG_PIXEL_Y_DIMENSION :
	return ("PIXEL_Y_DIMENSION");
    case Fl_EXIF_Data::TAG_MAKER_NOTE :
	return ("MAKER_NOTE");
    case Fl_EXIF_Data::TAG_USER_COMMENTS :
	return ("USER_COMMENTS");
    case Fl_EXIF_Data::TAG_EXPOSURE_TIME :
	return ("EXPOSURE_TIME");
    case Fl_EXIF_Data::TAG_F_NUMBER :
	return ("F_NUMBER");
    case Fl_EXIF_Data::TAG_EXPOSURE_PROGRAM :
	return ("EXPOSURE_PROGRAM");
    case Fl_EXIF_Data::TAG_SPECTRAL_SENSITIVITY :
	return ("SPECTRAL_SENSITIVITY");
    case Fl_EXIF_Data::TAG_ISO_SPEED_RATINGS :
	return ("ISO_SPEED_RATINGS");
    case Fl_EXIF_Data::TAG_OECF :
	return ("OECF");
    case Fl_EXIF_Data::TAG_SHUTTER_SPEED_VALUE :
	return ("SHUTTER_SPEED_VALUE");
    case Fl_EXIF_Data::TAG_APERTURE_VALUE :
	return ("APERTURE_VALUE");
    case Fl_EXIF_Data::TAG_BRIGHTNESS_VALUE :
	return ("BRIGHTNESS_VALUE");
    case Fl_EXIF_Data::TAG_EXPOSURE_BIAS_VALUE :
	return ("EXPOSURE_BIAS_VALUE");
    case Fl_EXIF_Data::TAG_MAX_APERTURE_VALUE :
	return ("MAX_APERTURE_VALUE");
    case Fl_EXIF_Data::TAG_SUBJECT_DISTANCE :
	return ("SUBJECT_DISTANCE");
    case Fl_EXIF_Data::TAG_METERING_MODE :
	return ("METERING_MODE");
    case Fl_EXIF_Data::TAG_LIGHT_SOURCE :
	return ("LIGHT_SOURCE");
    case Fl_EXIF_Data::TAG_FLASH :
	return ("FLASH");
    case Fl_EXIF_Data::TAG_FOCAL_LENGTH :
	return ("FOCAL_LENGTH");
    case Fl_EXIF_Data::TAG_FLASH_ENERGY :
	return ("FLASH_ENERGY");
    case Fl_EXIF_Data::TAG_SPATIAL_FREQUENCY_RESPONSE :
	return ("SPATIAL_FREQUENCY_RESPONSE");
    case Fl_EXIF_Data::TAG_FOCAL_PLANE_X_RESOLUTION :
	return ("FOCAL_PLANE_X_RESOLUTION");
    case Fl_EXIF_Data::TAG_FOCAL_PLANE_Y_RESOLUTION :
	return ("FOCAL_PLANE_Y_RESOLUTION");
    case Fl_EXIF_Data::TAG_FOCAL_POLANE_RESOLUTION_UNIT :
	return ("FOCAL_POLANE_RESOLUTION_UNIT");
    case Fl_EXIF_Data::TAG_SUBJECT_LOCATION :
	return ("SUBJECT_LOCATION");
    case Fl_EXIF_Data::TAG_EXPOSURE_INDEX :
	return ("EXPOSURE_INDEX");
    case Fl_EXIF_Data::TAG_SENSING_METHOD :
	return ("SENSING_METHOD");
    case Fl_EXIF_Data::TAG_FILE_SOURCE :
	return ("FILE_SOURCE");
    case Fl_EXIF_Data::TAG_SCENE_TYPE :
	return ("SCENE_TYPE");
    case Fl_EXIF_Data::TAG_CFA_PATTERN :
	return ("CFA_PATTERN");
    default :
        sprintf(s, "0x%04x", tag);
        return (s);
  }
}


//
// 'type_name()' - Return the type name...
//

const char *				// O - Type name
type_name(unsigned short type)		// I - Type
{
  static char	s[255];			// Temporary string


  switch (type)
  {
    case Fl_EXIF_Data::TYPE_BYTE :
	return ("BYTE");
    case Fl_EXIF_Data::TYPE_ASCII :
	return ("ASCII");
    case Fl_EXIF_Data::TYPE_SHORT :
	return ("SHORT");
    case Fl_EXIF_Data::TYPE_LONG :
	return ("LONG");
    case Fl_EXIF_Data::TYPE_RATIONAL :
	return ("RATIONAL");
    case Fl_EXIF_Data::TYPE_UNDEFINED :
	return ("UNDEFINED");
    case Fl_EXIF_Data::TYPE_SLONG :
	return ("SLONG");
    case Fl_EXIF_Data::TYPE_SRATIONAL :
	return ("SRATIONAL");
    default :
        sprintf(s, "%d???", type);
        return (s);
  }
}


//
// End of "$Id: testexif.cxx 322 2005-01-23 03:55:19Z easysw $".
//
