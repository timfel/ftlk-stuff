/*
 * "$Id: debug.h 321 2005-01-23 03:52:44Z easysw $"
 *
 * Debugging macros for flPhoto.
 *
 * Copyright 2002-2005 by Michael Sweet.
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

#ifndef _DEBUG_H_
#  define _DEBUG_H_

/*
 * Include necessary headers.
 */

#  include <stdio.h>

#  ifdef DEBUG
#    ifdef __cplusplus
#      define DEBUG_printf(x) ::printf x
#      define DEBUG_puts(x)   ::puts(x)
#    else
#      define DEBUG_printf(x) printf x
#      define DEBUG_puts(x)   puts(x)
#    endif /* __cplusplus */
#  else
#    define DEBUG_printf(x)
#    define DEBUG_puts(x)
#  endif /* DEBUG */

#endif /* !_DEBUG_H_ */

/*
 * End of "$Id: debug.h 321 2005-01-23 03:52:44Z easysw $".
 */
