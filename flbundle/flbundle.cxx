/******************************************************************************
* FILE NAME	      : flbundle.cxx
* PURPOSE         :
*
*      MacOS X application bundle fixer utility. Creates an application
*      bundle out of an executable and some (optional) icon image files.
*
*      Inspired by the Allegro version written by Angelo Mottola.
*
* AUTHOR          : IMM
* DATE WRITTEN    : 28th April 2004
* COMPILATION CMD : fltk-config --use-images --compile flbundle.cxx
* ADDITIONAL FILES:
* LAST EDITED     : 15-05-2004 IMM
*
* AMD NO      DATE      DESCRIPTION                                    INITLS
* ------      ----      -----------                                    ------
*
******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <Carbon/Carbon.h>

#include <FL/Fl.H>
#include <FL/x.H>
#include <FL/Fl_Shared_Image.H>

/* Debug services... */
//#define DEBUG

#ifdef DEBUG
#define DBprintf(params...)  fprintf(stderr, ##params)
#warning Building in Debug Mode
#else
#define DBprintf(params...)  {if (noisy) fprintf(stderr, ##params);}
#endif

/* Local Defines... */
/* Possible exit status codes */
#define XIT_OK		 0		/* Exit OK */
#define XIT_BAD		-1		/* Exit due to a malloc failure or etc. */
#define X_RM_DIR	-2		/* Exit, removing any created subdirectories and files */
#define X_PHASE_3	-3		/* Exit, because of something really bad. */

/* icon tyoe / size index */
#define ICON_SMALL			0 	/* 16x16 - "Small" in Apple speak */
#define ICON_MEDIUM			1	/* 32x32 - "Large" in Apple speak */
#define ICON_LARGE			2	/* 48x48 - "Huge" This seems to be what shows up in the Finder */
#define ICON_THUMBNAIL		3	/* 128x128 - This seems to be what shows up in the Dock */

/* Flag states for actions */
#define F_SMALL_DEFINED		0x1
#define F_LARGE_DEFINED		0x2
#define F_HUGE_DEFINED		0x4
#define F_THUMBNAIL_DEFINED	0x8
#define F_ICONS_MASK		0xf
#define F_DELETE			0x10
#define F_GOT_VERSION		0x20
#define F_GOT_LONG_VERSION	0x40

/* Forward declarations */
static void exit_clean (int exit_status);

/* Global Vars */
static int flags = 0;   // flags to hold icon state
static int noisy = 0;   // makes program verbose

static char *bundle_exe = NULL;          // Path to the original exe
static char *bundle = NULL;              // name given for bundle
static char *bundle_dir = NULL;          // Path for the bundle.app to be created

static char *Contents_dir = NULL;        // Path to the bundle Contents dir
static char *Resources_dir = NULL;       // Path to the bundle Contents/Resources dir
static char *MacOS_dir = NULL;           // Path to the bundle Contents/MacOS dir

static char *MacOS_dir_exe = NULL;       // Path to the bundle exe
static char *bundle_plist = NULL;        // Path to the bundle plist file
static char *bundle_pkginfo = NULL;      // Path to the PkgInfo file
static char *bundle_icns = NULL;         // Path to the bundle icons file

static char *bundle_version = NULL;      // holds the "short" version string, if set
static char *bundle_long_version = NULL; // holds the "long" version string, when set

static char *buffer = NULL;	// Temporary string handling buffer

static Fl_Shared_Image *img; /* Holds the icon images whilst converting them into icons */
static const char * datap ;  /* Used to access the raw image data */

/* Declare the local structs, etc... */
typedef struct IMAGE_DATA
{
	unsigned long *image;
	char * name;
} IMAGEDATA;

static IMAGEDATA * imagedata;

typedef struct ICON_DATA
{
	IMAGEDATA *rightsized;
	int    size;
	OSType data, mask8, mask1;
	int    defined;
} ICON_DATA;

static ICON_DATA icon_data[4] =
{
	{NULL, 16, kSmall32BitData, kSmall8BitMask, kSmall1BitMask, F_SMALL_DEFINED},
	{NULL, 32, kLarge32BitData, kLarge8BitMask, kLarge1BitMask, F_LARGE_DEFINED},
	{NULL, 48, kHuge32BitData, kHuge8BitMask, kHuge1BitMask, F_HUGE_DEFINED},
	{NULL, 128, kThumbnail32BitData, kThumbnail8BitMask, 0, F_THUMBNAIL_DEFINED}
};

/* Functions... */

/******************************************************************************
* Name:        tidy
*
* Checks for any still malloc'd memory and releases it.
* Easier to do as a macro than a function!
*******************************************************************************/
#define tidy(mem)  {if(mem){free(mem); mem = NULL;}} /* tidy */

/******************************************************************************
* Name:        file_exists
******************************************************************************/
static int file_exists (const char * name)
{
	return (access(name, 0) == 0);
} // file_exists

/******************************************************************************
* Name:        basename
*
* Find the last directory seperator, return whatever is left...
* Emulates the "real" basename() function, that doesn't seem to be present in 
* OSX 10.1.5
******************************************************************************/
static char * basename (const char * name)
{
	char * c;
	c = strrchr(name, '/');
	if (c != NULL) // we found something
	{
		c++; // point to the next char AFTER the dir sep
	}
	else
	{
		c = (char *)name; // point at the whole path...
	}
	return c; /* root of named file from path. Probably. */
} // basename

/******************************************************************************
* Name:        get_filesize
******************************************************************************/
static unsigned long get_filesize (const char * name)
{
	struct stat Statbuf;
	unsigned long size = 0;

	/* if file exists, get it's size */
	if (stat(name, &Statbuf) == 0)
	{
		size = (unsigned long)Statbuf.st_size; // size of named file
	}

	return size;
} // get_filesize

/******************************************************************************
* Name:        set_extension
******************************************************************************/
static int set_extension (char ** new_name, const char * base,
						  const char * ext)
{
	int len1, len2, len;
	char * last_dir;
	char * last_dot;

	len1 = strlen(base);
	len2 = strlen(ext);
	len = len1 + len2; // should be big enough for the worst case name length that can result...

	*new_name = (char *)malloc(len + 8); // plus a wee bit extra just in case...
	if (*new_name == NULL)
	{
		fprintf(stderr, "Failed to allocate space for set_extension string\n");
		return -1;
	}

	strcpy(*new_name, base);

	/* Now check in case the passed name already has an extension set...*/
	last_dir = strrchr(*new_name, '/');
	if (last_dir == NULL) // there is no dir_sep in the string
	{
		last_dir = *new_name;
	}

	last_dot = strrchr(last_dir, '.');
	if (last_dot != NULL) // there is an extension present
	{
		*last_dot = 0; // terminate the base string at the extension
	}
	/* And add the new extension */
	strcat(*new_name, ext);
	return 0; // all's well
} // set_extension

/******************************************************************************
* Name:        build_path
******************************************************************************/
static int build_path (char ** new_name, const char * base,
						  const char * ext)
{
	int len1, len2, len;

	len1 = strlen(base);
	len2 = strlen(ext);
	len = len1 + len2; // should be big enough for the worst case name length that can result...

	*new_name = (char *)malloc(len + 8); // plus a wee bit extra just in case...
	if (*new_name == NULL)
	{
		fprintf(stderr, "Failed to allocate space for build_path string\n");
		return -1;
	}

	strcpy(*new_name, base);
	/* And add the new path section */
	strcat(*new_name, ext);
	return 0; // all's well
} // build_path

/******************************************************************************
* Name:        get_alpha_chan
******************************************************************************/
int get_alpha_chan (unsigned long c)
{
   return ((c >> 24) & 0xFF);
} // get_alpha_chan

/******************************************************************************
* Name:        load_image
******************************************************************************/
static void load_image(const char *name)
{
	if (img) img->release();

	img = Fl_Shared_Image::get(name);
	if (!img)
	{
		fprintf(stderr, "Image file format not recognized!: %s\n", name);
		exit_clean(XIT_BAD);
	}
} // load_image

/******************************************************************************
* Name:        load_icon_image
******************************************************************************/
int load_icon_image(char * name, ICON_DATA * icon, int target_size)
{
	Fl_Image *img_resized; /* Used for re-sizing the image */

	DBprintf("Preparing icon for size %d\n", target_size);

	if (!file_exists(name))
	{
		fprintf(stderr, "Requested icon image file doesn't seem to exist: %s\n", name);
		exit_clean(XIT_BAD);
	}

	load_image(name);

	int width = img->w();
	int height = img->h();
	int depth = img->d();
	int count = img->count();
	int type;
	unsigned char entry, r, g, b, a;

	if (target_size == 16)
		type = ICON_SMALL;
	else
	if (target_size == 32)
		type = ICON_MEDIUM;
	else
	if (target_size == 48)
		type = ICON_LARGE;
	else
		type = ICON_THUMBNAIL;

	DBprintf("Width x Height: %d x %d Count: %d Depth: %d\n", width, height, count, depth);
//	DBprintf("Line Data: %d\n", img->ld());

	if (count == 0)
	{
		fprintf(stderr, "Icon image file seems to have no data\n");
		exit_clean(XIT_BAD);
	}

	/* Make the image the right size for the icon */
	if ((width != target_size) || (height != target_size))
	{
		DBprintf("Resizing icon image to %d x %d\n", target_size, target_size);

		img_resized = img->copy(target_size, target_size); /* Make a re-sized copy of the original */
		img->release(); /* Drop the old "wrong size" image */
		img = (Fl_Shared_Image *)img_resized;  /* And copy the resized version in its place */

		width = img->w();
		height = img->h();
		depth = img->d();
		count = img->count();

		DBprintf("Width x Height: %d x %d Count: %d Depth: %d\n", width, height, count, depth);
	}

	/* create the struct to hold the image data for this icon */
	imagedata = (IMAGEDATA *)malloc(sizeof(IMAGEDATA));
	if (!imagedata)
	{
		fprintf(stderr, "imagedata struct allocation failed\n");
		exit_clean(XIT_BAD);
	}
	/* allocate storage space for the image array */
	imagedata->image = (unsigned long *)malloc(width * height * sizeof(unsigned long) + 16);
	if (!imagedata->image)
	{
		fprintf(stderr, "imagedata->image allocation failed\n");
		exit_clean(XIT_BAD);
	}
	/* make a copy of the name of the source image */
	imagedata->name = strdup (name);
	if (!imagedata->name)
	{
		fprintf(stderr, "imagedata->name storage failed\n");
		exit_clean(XIT_BAD);
	}

	/* Now we can start to extract the image data */
	if (count == 1)
	{
		/* Read the data - byte order is R, G, B, then A, if present... */
		/* An A of 0xFF is opaque(?), an A of 0 is transparent(?) */

		/* Depth options:
		 * d = 0: bitmap... decode the bits, and set RGBA as required.
		 * d = 1: monochrome, set R = G = B = data, A = 0xFF - data ??
		 * d = 2: mono with alpha, set R = G = B = data[0], A = data[1]
		 * d = 3: colour, read R, G, B, set A = ?something?
		 * d = 4: colour with alpha, read R, G, B, A
		 */
		/* Get a pointer to the data */
		datap = *img->data();

		int cells = width * height;
		int idx = 0;
		if (depth == 0) cells = cells / 8;
		for (int i = 0; i < cells; i++)
		{
			switch (depth)
			{
			case 0: /* Read XBM bitmaps here */
				{
					unsigned char c = (unsigned char)datap[idx]; idx ++;
					unsigned char mask = 1;
					int cycle = 0;
					while (cycle < 8)
					{
						if (c & mask) /* bit is set */
						{ /* this pixel is black */
							imagedata->image[(i * 8) + cycle] = 0xFF000000;
						}
						else
						{ /* this pixel is white / transparent */
							imagedata->image[(i * 8) + cycle] = 0x00FFFFFF;
						}
						cycle ++;
						mask = mask << 1;
					}
				}
				continue;
			case 1: /* Monochrome image, no alpha channel */
				r = (unsigned char)datap[idx]; idx ++;
				g = b = r;
				a = 0xFF - r; // A - FIXME: what *should* this be?
				break;
			case 2: /* Monochrome image, with alpha channel */
				r = (unsigned char)datap[idx]; idx ++;
				a = (unsigned char)datap[idx]; idx ++;
				g = b = r;
				break;
			case 3: /* Colour image, no alpha channel */
				r = (unsigned char)datap[idx]; idx ++;
				g = (unsigned char)datap[idx]; idx ++;
				b = (unsigned char)datap[idx]; idx ++;
				a = 0xFF; // A - FIXME: what *should* this be?
				break;
			case 4: /* Colour image, with alpha channel */
				r = (unsigned char)datap[idx]; idx ++;
				g = (unsigned char)datap[idx]; idx ++;
				b = (unsigned char)datap[idx]; idx ++;
				a = (unsigned char)datap[idx]; idx ++;
				break;
			default:
				r = g = b = 0; // makes an opaque black pixel...
				a = 0xff;
				break;
			}
			/* Now form the word for the icon image... I *think* this goes like this;
			 * A:R:G:B, with A in the high byte... */
			imagedata->image[i] = (((unsigned long)a << 24) & 0xFF000000) +
			                      (((unsigned long)r << 16) & 0x00FF0000) +
			                      (((unsigned long)g <<  8) & 0x0000FF00) +
			                       ((unsigned long)b        & 0x000000FF);
		}
	}
	else
	if (count > 1)
	{
	    /* Work around GIF or pixmap issues... These are palette based...
	     * For GIF, the header is in line [0], the palette is in line [1]
		 * For Pixmaps, the header is in line[0], the palette is either:
		 * - in binary in line [1] like a GIF
		 * - in text form over several lines, one entry per line...
		 * The first case is indicated by a negative colours value. I think. */

		unsigned char hdr[257];
		unsigned long pal[256];
		int i, w, h, colours, chars_per_pix;
		int idx, row;
		int firstline = count - height;

		/* Read in the hdr line */
		datap = img->data()[0];
		for (i = 0; i < 256; i++)
		{
			hdr[i] = datap[i];
		}
		hdr[256] = '\0'; /* Make sure this is terminated... */
		sscanf((const char *)hdr,"%d %d %d %d", &w, &h, &colours, &chars_per_pix);

		if (chars_per_pix != 1)
		{
			fprintf(stderr, "Don't know how to process multi-byte-per-pixel Pixmaps\n");
			exit_clean(XIT_BAD);
		}

		/* now interpret the palette... */
		memset(pal, 0, (256 * sizeof(long)));
		for (row = 1; row < firstline; row ++)
		{
			char * str;
			datap = img->data()[row];
			idx = 0;
			
			/* check if this is a strings type palette - look for the leading "c" */
			str = strchr(datap, 'c');
			if (str && (colours > 0))
			{
				str = strchr(str, '#');
				if (str)
				{
					entry = datap[idx];
					if (entry == 0x20)
					{
						pal[entry] = 0 ; // Transparent
					}
					else
					{
						fl_parse_color(str, r, g, b);
						a = 0xFF;
						pal[entry] =	(((unsigned long)a << 24) & 0xFF000000) +
										(((unsigned long)r << 16) & 0x00FF0000) +
										(((unsigned long)g <<  8) & 0x0000FF00) +
										 ((unsigned long)b        & 0x000000FF);
					}
				}
			}
			else /* assume this is a byte coded palette */
			{
				colours = abs(colours);
				if (colours > 256) colours = 256; // just in case...!
				for (i = 0; i < colours; i++)
				{
					entry = datap[idx]; idx ++;
					r = datap[idx]; idx ++; // R
					g = datap[idx]; idx ++; // G
					b = datap[idx]; idx ++; // B
	
					if ((i == 0) && (entry == 0x20)) // have transparency
					{
						a = 0;  // A - This colour is transparent
					}
					else
					{
						a = 0xFF;  // A - This colour is opaque
					}
		
					/* Now form the word for the palette entry... I *think* this goes like this;
					*  A:R:G:B, with A in the high byte... */
					pal[entry] =	(((unsigned long)a << 24) & 0xFF000000) +
									(((unsigned long)r << 16) & 0x00FF0000) +
									(((unsigned long)g <<  8) & 0x0000FF00) +
									 ((unsigned long)b        & 0x000000FF);
//					printf("0x%02X: 0x%02X 0x%02X 0x%02X 0x%02X\n", entry, r, g, b, a);
				}
			}
		}

		/* Now interpret the image data */
		idx = 0;
		for (i = firstline; i < count; i++)
		{
			/* Get a pointer to the data */
			datap = img->data()[i];
			for (int d = 0; d < width; d++)
			{
				entry = (unsigned char)datap[d];
				imagedata->image[idx] = pal[entry];
				idx ++; /* index to next image cell */
			}
		}
	}

	if (!icon) /* Were we passed an icon struct to fill ? */
	{ /* If not, find the correct global entry... */
		icon = &icon_data[type];
		if (flags & icon->defined)
		{ /* We seem to have already set this size of icon? */
			fprintf (stderr, "Duplicate icon sizes defined\n");
			return -1;
		}
	}
	else
	{
		if (icon->rightsized) /* we must already have filled this icon entry... */
		{
			fprintf (stderr, "Multiple icon resources of the same size\n");
			return -1;
		}
	}

	/* Assign the newly formed and scaled image to the icon data */
	icon->rightsized = imagedata;
	flags |= icon->defined;

//	if (img) img->release(); /* release the source image data */

	return 0; // all is well
} // load_icon_image

/******************************************************************************
* Name:        check_icons_defined
*
* Check each icon entry in turn - if it is empty, try and fill it with a 
* resized version of one of the other sized icons...
******************************************************************************/
static void check_icons_defined(void)
{
	int i, idx;
	
	DBprintf("Checking allocated icons\n");
	for (idx = 3; idx >= 0; idx --)
	{  /* test each icon entry in turn */
		if (icon_data[idx].rightsized == NULL) /* to see if it has been set */
		{ /* This entry is empty  - try and copy something from its neighbours */
			switch (idx)
			{
			case 0: /* just work our way up the sizes... */
				DBprintf("Icon 16 missing - inserting\n");
				for (i = 1; i < 4; i++)
				{
					if (icon_data[i].rightsized)
					{ /* got something */
						if (load_icon_image(icon_data[i].rightsized->name, &icon_data[0], 16))
						{
							exit_clean (XIT_BAD);
						}
						break;
					}
				}			
				break;
			case 1: /* try the nearest neighbours first... 2, 0, 3*/
				DBprintf("Icon 32 missing - inserting\n");
				if (icon_data[2].rightsized)
					i = 2;
				else if (icon_data[0].rightsized)
					i = 0;
				else i = 3;
				if (load_icon_image(icon_data[i].rightsized->name, &icon_data[1], 32))
				{
					exit_clean (XIT_BAD);
				}
				break;
			case 2: /* try the nearest neighbours first... 3, 1, 0*/
				DBprintf("Icon 48 missing - inserting\n");
				if (icon_data[3].rightsized)
					i = 3;
				else if (icon_data[1].rightsized)
					i = 1;
				else i = 0;
				if (load_icon_image(icon_data[i].rightsized->name, &icon_data[2], 48))
				{
					exit_clean (XIT_BAD);
				}
				break;
			case 3: /* just work our way down the sizes... */
				DBprintf("Icon 128 missing - inserting\n");
				for (i = 2; i >= 0; i--)
				{
					if (icon_data[i].rightsized)
					{ /* got something */
						if (load_icon_image(icon_data[i].rightsized->name, &icon_data[3], 128))
						{
							exit_clean (XIT_BAD);
						}
						break;
					}
				}			
				break;
			default: /* can't be here - do nothing */
				break; 
			}
		}
	}
} // check_icons_defined

/******************************************************************************
* Name:        show_usage
******************************************************************************/
static void show_usage (void)
{
	fprintf (stderr, "\nMacOS X application bundle fixer-upper utility for FLTK\n\n"
			 "Usage: flbundle  -i <exename> [-d] [-o bundlename] [-v version] [-V long_version]\n"
			 "\t\t[-s, m, l, t <icon>] [-n]\n"
			 "\twhere icon is a FLTK supported image file to convert to an OSX icon.\n"
			 "Options:\n"
			 "\t-i <exename>\tThe name of the exe to be made into a bundle\n"
			 "\t-d\t\tDeletes the original exe after moving it inside the bundle\n"
			 "\t-n\t\tMakes the program verbose (\"n\"oisy) about what it is doing\n"
			 "\t-o bundlename\tSpecifies a bundle name (default: exename.app)\n"
			 "\t-v version\tSets application version string (default: 1.0)\n"
			 "\t-V long_version\tSets long application version string (default: none)\n"
			 "\t-s(mall) <icon>\t\tPlaces next icon into the 16x16 resource slot\n"
			 "\t-m(edium) <icon>\tPlaces next icon into the 32x32 resource slot\n"
			 "\t-l(arge) <icon>\t\tPlaces next icon into the 48x48 resource slot\n"
			 "\t-t(humbnail) <icon>\tPlaces next icon into the 128x128 resource slot\n"
			 "\n");

	exit_clean (XIT_BAD);
} // show_usage

/******************************************************************************
* Name:        copy_file
******************************************************************************/
static int copy_file (const char *source, const char *dest)
{
	char  *buffer = NULL;
	FILE  *file;
	unsigned long size;

	/* Check there is something to copy */
	if (!file_exists (source)) return -1;

	/* Make a temporary space to copy the file into */
	size = get_filesize (source);
	buffer = (char *)malloc (size);
	if (!buffer) return -1;

	/* Open the source file */
	file = fopen (source, "r");
	if (!file)
	{
		tidy(buffer);
		return -1;
	}
	/* Read it into RAM */
	fread (buffer, size, 1, file);
	fclose (file);

	/* Open the target file */
	file = fopen (dest, "w");
	if (!file)
	{
		tidy(buffer);
		return -1;
	}
	/* Write the data out */
	fwrite (buffer, size, 1, file);
	fclose (file);

	tidy(buffer);
	return 0; /* And all's well */
} // copy_file

/******************************************************************************
* Name:        cleanup
*
* Called via atexit when it's time to quit.
* Checks for any still malloc'd memory and releases it.
*******************************************************************************/
static void cleanup (void)
{
	int i;

	tidy (bundle_exe);
	tidy (bundle);
	tidy (bundle_dir);
	tidy (Contents_dir);
	tidy (Resources_dir);
	tidy (MacOS_dir);
	tidy (MacOS_dir_exe);
	tidy (bundle_plist);
	tidy (bundle_pkginfo);
	tidy (bundle_icns);
	tidy (bundle_version);
	tidy (bundle_long_version);
	tidy (buffer);

	for (i = 0; i < 4; i++)
	{
		if (icon_data[i].rightsized)
		{
			tidy(icon_data[i].rightsized->image);
			tidy(icon_data[i].rightsized->name);
			tidy(icon_data[i].rightsized);
		}
	}

	if (img) img->release(); /* release the source image data */

} /* end of atexit clean up code */

/******************************************************************************
* Name:        exit_clean
*
* Called by user space code when it wants to expire.
* Does any tidying up required, depending on the exit_status, then exits in
* the usual way.
*******************************************************************************/
static void exit_clean (int exit_status)
{
	if (exit_status < XIT_OK)
	{
		switch(exit_status)
		{
		default: /* something strange has happened... */
			/* fall through */

		case X_PHASE_3: /* Remove all the created structures */
			DBprintf("Exit - something bad happened\n");
			/* fall through */

		case X_RM_DIR: /* Remove the files and sub-dirs we have created */
			DBprintf("Exit - rmdir\n");

			remove (MacOS_dir_exe);
			remove (bundle_plist);
			remove (bundle_pkginfo);
			remove (bundle_icns);
		
			rmdir (MacOS_dir);
			rmdir (Resources_dir);
			rmdir (Contents_dir);
			rmdir (bundle_dir);

			/* fall through */

		case XIT_BAD: /* Free any malloc'd memory, etc. */
					  /* Actually released in cleanup */
			DBprintf("Exit - Bad Status\n");
			break;
		}
	}
	/* Now call the actual exit */
	exit(exit_status);
} /* end of exit_clean code */

/******************************************************************************
Name:        main

What we want to do:
For a given <appname> we want to create a bundle structure like this -

appname.app/
            --Contents/
			           --Info.plist (file)
                       --PkgInfo (file)
					   --MacOS/
					           --<appname_exe> (the exe file)
                       --Resources/
								   --appname.icns (the icons file)
					               --<various_resource_files>
                                   --English.lproj/
								                   --InfoPlist.strings
                                   --<OtherLanguage>.lproj/
								                           --InfoPlist.strings
                                   --etc...

However, the i18n language sub-dirs may not be required, as this may be done
in a more "conventional" or "portable" fashion in fltk apps.?

******************************************************************************/
int main (int argc, char *argv[])
{
	FILE  *file;
	CFURLRef cf_url_ref;
	FSRef  fs_ref;
	FSSpec fs_spec;
	IconFamilyHandle icon_family;
	Handle raw_data;

	int    i, c, size, x, y, mask_bit, mask_byte;
	unsigned char *data;

	atexit(cleanup); /* install normal "low-level" exit handler */

	if (argc < 2)
	{
		show_usage ();
	}

	/* Hook the fltk shared image functions */
	fl_register_images();
	Fl::visual(FL_RGB);

	/* Parse command line and load any given resources */
	while ((c = getopt(argc, argv, "i:do:v:V:s:m:l:t:hH?n")) > 0)
	{
		switch(c)
		{
			case 'i': // Sets the exe name - required.
				bundle_exe = strdup (optarg);
				if (bundle_exe == NULL)
				{
					exit_clean (XIT_BAD);
				}
				break;
			case 'd': // Delete or copy the exe - optional, defaults to copy
				flags |= F_DELETE;
				break;
			case 'n': // Enable verbose (noisy) operation
				noisy = 1;
				break;
			case 'o': // Output bundle name - optional, defaults to exename.app
				bundle = strdup (optarg);
				if (bundle == NULL)
				{
					exit_clean (XIT_BAD);
				}
				break;
			case 'v': // Short bundle version string - optional
				flags |= F_GOT_VERSION;
				bundle_version = strdup (optarg);
				if (bundle_version == NULL)
				{
					exit_clean (XIT_BAD);
				}
				break;
			case 'V': // Long bundle version string - optional
				flags |= F_GOT_LONG_VERSION;
				bundle_long_version = strdup (optarg);
				if (bundle_long_version == NULL)
				{
					exit_clean (XIT_BAD);
				}
				break;
			case 's': // Points to the image to use for the Small icon - optional
				if (load_icon_image(optarg, &icon_data[0], 16))
				{
					exit_clean (XIT_BAD);
				}
				break;
			case 'm': // Points to the image to use for the Large (medium) icon - optional
				if (load_icon_image(optarg, &icon_data[1], 32))
				{
					exit_clean (XIT_BAD);
				}
				break;
			case 'l': // Points to the image to use for the Huge (large) icon - optional
				if (load_icon_image(optarg, &icon_data[2], 48))
				{
					exit_clean (XIT_BAD);
				}
				break;
			case 't': // Points to the image to use for the Thumbnail - optional
				if (load_icon_image(optarg, &icon_data[3], 128))
				{
					exit_clean (XIT_BAD);
				}
				break;
			case '?':
			case 'h':
			case 'H':
			default:
				show_usage ();
				return 0;
		}
	}

	/* Check we have been passed a sensible value for the basic exe */
	if (bundle_exe == NULL)
		show_usage();
	
	if (!file_exists (bundle_exe))
	{
		fprintf (stderr, "Cannot locate executable file \"%s\"\n", bundle_exe);
		exit_clean (XIT_BAD);
	}

	/* Now, check if we have been passed any icons... */
	if (flags & F_ICONS_MASK)
	{
		/* At least 1 icon is defined - so if any have been left blank, use one of the ones
		 * that *have* been defined to fill in the gaps... */
		 check_icons_defined();
	}
	/* else no icons were defined, so we just use the system defaults */
	
	/* Check if we've been passed a target bundle name, and if not, make one up */
	if (bundle == NULL)
	{
		bundle = strdup (bundle_exe);
		if (bundle == NULL) exit_clean (XIT_BAD);
	}

	if (set_extension (&bundle_dir, bundle, ".app") != 0) exit_clean (XIT_BAD);
	/* Should have made ....appname.app */
	DBprintf("made %s\n", bundle_dir);

	/* Now construct the pathnames for all the bundle sub directories */
	if (build_path (&Contents_dir, bundle_dir, "/Contents") != 0) exit_clean (XIT_BAD);
	/* Should have made ....appname.app/Contents */
	DBprintf("made %s\n", Contents_dir);

	if (build_path (&Resources_dir, Contents_dir, "/Resources") != 0) exit_clean (XIT_BAD);
	/* Should have made ....appname.app/Contents/Resources */
	DBprintf("made %s\n", Resources_dir);

	if (build_path (&MacOS_dir, Contents_dir, "/MacOS") != 0) exit_clean (XIT_BAD);
	/* Should have made ....appname.app/Contents/MacOS */
	DBprintf("made %s\n", MacOS_dir);

	/* Now build the path for the exe embedded in the bundle */
	if (build_path (&buffer, MacOS_dir, "/") != 0) exit_clean (XIT_BAD);
	/* At this point the temporary buffer should hold .../MacOS/ */

	if (build_path (&MacOS_dir_exe, buffer, basename (bundle_exe)) != 0) exit_clean (XIT_BAD);
	/* Should have made ....appname.app/Contents/MacOS/exename */
	DBprintf("made %s\n", MacOS_dir_exe);
	tidy(buffer); /* release the temporary buffer */

	/* Create the bundle structure */
	if ((mkdir (bundle_dir,    0777) && (errno != EEXIST)) ||
		(mkdir (Contents_dir,  0777) && (errno != EEXIST)) ||
		(mkdir (Resources_dir, 0777) && (errno != EEXIST)) ||
		(mkdir (MacOS_dir,     0777) && (errno != EEXIST)))
	{
		fprintf (stderr, "Cannot create %s\n", bundle_dir);
		exit_clean (X_RM_DIR);
	}

	/* Copy/move executable into the bundle */
	if (copy_file (bundle_exe, MacOS_dir_exe))
	{
		fprintf (stderr, "Cannot copy exe into bundle dir %s\n", MacOS_dir);
		exit_clean (X_RM_DIR);
	}
	/* Fix the target exe permissions */
	chmod (MacOS_dir_exe, 0755);

	/* Remove the original binary if "delete" mode was selected, and successful! */
	if (flags & F_DELETE)
	{
		remove (bundle_exe);
	}

	/* Now we are ready to prepare the icons file */
	/* Setup the .icns resource */
	if (flags & F_ICONS_MASK)
	{
		if (build_path (&bundle_icns, Resources_dir, "/") != 0) exit_clean (X_RM_DIR);
		if (build_path (&buffer, bundle_icns, basename (bundle)) != 0) exit_clean (X_RM_DIR);
		/* At this point, buffer holds ....appname.app/Contents/Resources/appname */
		tidy(bundle_icns);
		if (set_extension (&bundle_icns, buffer, ".icns") != 0) exit_clean (X_RM_DIR);
		/* At this point, bundle_icns holds ....appname.app/Contents/Resources/appname.icns */
		tidy(buffer);
		DBprintf("made %s\n", bundle_icns);
	
		icon_family = (IconFamilyHandle) NewHandle (0);

		for (i = 0; i < 4; i++)
		{
			if (flags & icon_data[i].defined)
			{
				/* Set 32bit RGBA data */
				PtrToHand (icon_data[i].rightsized->image, &raw_data, icon_data[i].size * icon_data[i].size * 4);
				if (SetIconFamilyData (icon_family, icon_data[i].data, raw_data) != noErr)
				{
					DisposeHandle (raw_data);
					fprintf (stderr, "Error setting %d x %d icon resource RGBA data\n", icon_data[i].size, icon_data[i].size);
					exit_clean (X_RM_DIR);
				}

				DisposeHandle (raw_data);

				/* Set 8bit mask */
				raw_data = NewHandle (icon_data[i].size * icon_data[i].size);
				data = (unsigned char *)*raw_data;
			
				unsigned int count = 0;
				for (y = 0; y < icon_data[i].size; y++)
				{
					for (x = 0; x < icon_data[i].size; x++)
					{
						*data++ = get_alpha_chan (icon_data[i].rightsized->image[count]);
						count ++;
					}
				}

				if (SetIconFamilyData (icon_family, icon_data[i].mask8, raw_data) != noErr)
				{
					DisposeHandle (raw_data);
					fprintf (stderr, "Error setting %d x %d icon resource 8bit mask\n", icon_data[i].size, icon_data[i].size);
					exit_clean (X_RM_DIR);
				}

				DisposeHandle (raw_data);

				/* Set 1bit mask */
				if (icon_data[i].mask1)
				{
					size = ((icon_data[i].size * icon_data[i].size) + 7) / 8;
					raw_data = NewHandle (size * 2);
					data = (unsigned char *)*raw_data;
					mask_byte = 0;
					mask_bit = 7;
					count = 0;
					for (y = 0; y < icon_data[i].size; y++)
					{
						for (x = 0; x < icon_data[i].size; x++)
						{
							int val = get_alpha_chan (icon_data[i].rightsized->image[count]);
							count ++; // next image cell...
							if (val >= 0xfd)
								mask_byte |= (1 << mask_bit);
							mask_bit--;
							if (mask_bit < 0)
							{
								*data++ = mask_byte;
								mask_byte = 0;
								mask_bit = 7;
							}
						}
					}

					memcpy (*raw_data + size, *raw_data, size);

					if (SetIconFamilyData (icon_family, icon_data[i].mask1, raw_data) != noErr)
					{
						DisposeHandle (raw_data);
						fprintf (stderr, "Error setting %d x %d icon resource 1bit mask\n", icon_data[i].size,
								 icon_data[i].size);
						exit_clean (X_RM_DIR);
					}

					DisposeHandle (raw_data);
				}
			}
		}

		/* Touch bundle_icns file to make sure it exists, and is empty... */
		file = fopen (bundle_icns, "w");
		if (!file)
		{
			fprintf (stderr, "Cannot create %s\n", bundle_icns);
			exit_clean (X_RM_DIR);
		}
		fclose (file);

		cf_url_ref = CFURLCreateWithBytes (kCFAllocatorDefault, (const UInt8 *)bundle_icns, strlen (bundle_icns), 0, NULL);
		if (!cf_url_ref)
		{
			fprintf (stderr, "Cannot create %s\n", bundle_icns);
			exit_clean (X_RM_DIR);
		}

		CFURLGetFSRef (cf_url_ref, &fs_ref);
		CFRelease (cf_url_ref);

		if ((FSGetCatalogInfo (&fs_ref, kFSCatInfoNone, NULL, NULL, &fs_spec, NULL)) ||
			(WriteIconFile (icon_family, &fs_spec) != noErr))
		{
			fprintf (stderr, "Cannot create %s\n", bundle_icns);
			exit_clean (X_RM_DIR);
		}

		DisposeHandle ((Handle) icon_family);
	}
	/* Icon preparation complete */

	/* Now create the Info.plist file */
	if (build_path (&bundle_plist, Contents_dir, "/Info.plist") != 0) exit_clean (X_RM_DIR);
	/* Now bundle_plist holds .....appname.app/Contents/Info.plist */
	DBprintf("made %s\n", bundle_plist);

	file = fopen (bundle_plist, "w");
	if (!file)
	{
		fprintf (stderr, "Cannot create %s\n", bundle_plist);
		exit_clean (X_RM_DIR);
	}

	buffer = (char *)malloc (4096);
	if (!buffer) exit_clean (X_RM_DIR);

	sprintf (buffer, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
			 "<!DOCTYPE plist PUBLIC \"-//Apple Computer//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n"
			 "<plist version=\"1.0\">\n"
			 "<dict>\n"
			 "\t<key>CFBundleExecutable</key>\n"
			 "\t<string>%s</string>\n"
			 "\t<key>CFBundleInfoDictionaryVersion</key>\n"
			 "\t<string>6.0</string>\n"
			 "\t<key>CFBundlePackageType</key>\n"
			 "\t<string>APPL</string>\n"
			 "\t<key>CFBundleSignature</key>\n"
			 "\t<string>%s</string>\n"
			 "\t<key>CFBundleVersion</key>\n"
			 "\t<string>%s</string>\n"
			 "\t<key>CFBundleDocumentTypes</key>\n"
			 "\t<array>\n"
			 "\t\t<dict>\n"
			 "\t\t\t<key>CFBundleTypeExtensions</key>\n"
			 "\t\t\t<array>\n"
			 "\t\t\t\t<string>*</string>\n"
			 "\t\t\t</array>\n"
			 "\t\t\t<key>CFBundleTypeName</key>\n"
			 "\t\t\t<string>NSStringPboardType</string>\n"
			 "\t\t\t<key>CFBundleTypeOSTypes</key>\n"
			 "\t\t\t<array>\n"
			 "\t\t\t\t<string>****</string>\n"
			 "\t\t\t</array>\n"
			 "\t\t\t<key>CFBundleTypeRole</key>\n"
			 "\t\t\t<string>Viewer</string>\n"
			 "\t\t</dict>\n"
			 "\t</array>\n",
			 basename (bundle_exe), "????", (flags & F_GOT_VERSION) ? bundle_version : "1.0");

	fputs (buffer, file);
	if (flags & F_GOT_LONG_VERSION)
	{
		sprintf (buffer, "\t<key>CFBundleGetInfoString</key>\n"
				 "\t<string>%s</string>\n", bundle_long_version);
		fputs (buffer, file);
	}

	if (flags & F_ICONS_MASK)
	{
		sprintf (buffer, "\t<key>CFBundleIconFile</key>\n"
				 "\t<string>%s</string>\n", basename (bundle_icns));
		fputs (buffer, file);
	}

	fputs ("</dict>\n</plist>\n", file);

	fclose (file);

	/* Now create the PkgInfo file */
	if (build_path (&bundle_pkginfo, Contents_dir, "/PkgInfo") != 0) exit_clean (X_RM_DIR);
	/* Now bundle_pkginfo holds .....appname.app/Contents/PkgInfo */
	DBprintf("made %s\n", bundle_pkginfo);

	file = fopen (bundle_pkginfo, "w");
	if (!file)
	{
		fprintf (stderr, "Cannot create %s\n", bundle_pkginfo);
		exit_clean (X_RM_DIR);
	}

	fputs ("APPL????", file);

	fclose (file);

	exit_clean (XIT_OK);
    exit (0); /* we never actually get here, but the compiler doesn't know that! */

} // end of main

/* End of File */

