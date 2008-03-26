/*
 * "$Id: i18n.c 288 2004-02-22 20:22:22Z easysw $"
 *
 * Internationalization function for flPhoto.
 *
 * Copyright 2003 by Michael Sweet.
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
 *
 * Contents:
 *
 *   flphoto_gettext() - Wrapper for ESP I18N library.
 */

#include "flstring.h"
#include <stdlib.h>
#ifdef WIN32
#  include <windows.h>
#  include <io.h>
#else
#  include <unistd.h>
#endif /* WIN32 */
#include "i18n.h"
#include "espmsg.h"


/*
 * 'flphoto_gettext()' - Wrapper for gettext().
 */

const char *				/* O - Translated message */
flphoto_gettext(const char *s)		/* I - Message string */
{
  const char		*locale,	/* FLPHOTO_LOCALE */
			*language;	/* LANG setting */
  char			filename[1024],	/* Message filename */
			langname[16],	/* Language name */
			*temp;		/* Temporary pointer */
  static int		num_msgs = -1;	/* Number of messages */
  static espmsg_t	*msgs;		/* Messages */
#ifdef WIN32				/**** Do registry magic... ****/
  HKEY			key;		/* Registry key */
  DWORD			size;		/* Size of string */
  char			tempdir[1024];	/* Locale directory */
#endif /* WIN32 */


  if (num_msgs < 0)
  {
    /* Load the message catalog... */
#ifdef WIN32
    /* Open the registry... */
    if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                      "SOFTWARE\\com.easysw\\flPhoto", 0,
                      KEY_READ, &key))
    {
      /* Grab the installed directories... */
      size = sizeof(tempdir);

      if (!RegQueryValueEx(key, "InstallDir", NULL, NULL, (unsigned char *)tempdir,
                           &size))
      {
        strlcat(tempdir, "locale", sizeof(tempdir));
        locale = tempdir;
      }

      RegCloseKey(key);
    }
    else
#endif /* WIN32 */
    if ((locale = getenv("FLPHOTO_LOCALE")) == NULL)
      locale = FLPHOTO_LOCALE;

    if ((language = getenv("LANG")) == NULL ||
        strcmp(language, "POSIX") == 0 ||
	strcmp(language, "C") == 0)
    {
      // Undefined or POSIX locales don't need localization...
      num_msgs = 0;
    }
    else
    {
      // Convert full language names to proper abbreviations...
      if (strcasecmp(language, "english") == 0)
        language = "en";
      else if (strcasecmp(language, "french") == 0)
        language = "fr";
      else if (strcasecmp(language, "italian") == 0)
        language = "it";
      else if (strcasecmp(language, "german") == 0)
        language = "de";
      else if (strcasecmp(language, "spanish") == 0)
        language = "es";

      // Try looking up the message file by locale...
      strncpy(langname, language, sizeof(langname) - 1);
      langname[sizeof(langname) - 1] = '\0';

      // Strip charset from "locale.charset"...
      if ((temp = strchr(langname, '.')) != NULL)
	*temp = '\0';

      if (strlen(langname) < 2)
	strcpy(langname, "C");
      else
      {
	langname[0] = tolower(langname[0]);
	langname[1] = tolower(langname[1]);

	if (langname[2] == '_' || langname[2] == '-')
	{
	  langname[2] = '_';
	  langname[3] = toupper(langname[3]);
	  langname[4] = toupper(langname[4]);
	  langname[5] = '\0';
	}
	else
	  langname[2] = '\0';
      }

      // See if the message file exists...
      snprintf(filename, sizeof(filename), "%s/%s/flphoto_%s", locale,
               langname, langname);

      if (access(filename, 0) && strlen(langname) > 2)
      {
        // Nope, see if we can open a generic language file...
	langname[2] = '\0';
	snprintf(filename, sizeof(filename), "%s/%s/flphoto_%s", locale,
	         langname, langname);
      }

      num_msgs = espLoadMessages(filename, &msgs);
    }
  }

  return (espLookupMessage(s, num_msgs, msgs));
}


/*
 * End of "$Id: i18n.c 288 2004-02-22 20:22:22Z easysw $".
 */
