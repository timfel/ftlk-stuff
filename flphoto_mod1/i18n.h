/*
 * "$Id: i18n.h 182 2003-03-24 17:58:45Z easysw $"
 *
 * Internationalization header file for flPhoto.
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
 */

#ifndef i18n_h
#  define i18n_h

#  include "config.h"

#  define _(x) flphoto_gettext(x)

#  ifdef __cplusplus
extern "C" {
#  endif /* __cplusplus */
extern const char	*flphoto_gettext(const char *s);
#  ifdef __cplusplus
}
#  endif /* __cplusplus */
#endif /* !i18n_h */


/*
 * End of "$Id: i18n.h 182 2003-03-24 17:58:45Z easysw $".
 */
