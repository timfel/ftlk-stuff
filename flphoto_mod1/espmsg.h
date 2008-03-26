/*
 * "$Id: espmsg.h 321 2005-01-23 03:52:44Z easysw $"
 *
 * ESP internationalization definitions.
 *
 * Copyright 1997-2005 by Easy Software Products.
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

#ifndef _ESPMSG_H_
#  define _ESPMSG_H_

/*
 * C++ magic...
 */

#  ifdef __cplusplus
extern "C" {
#  endif /* __cplusplus */


/*
 * Message structure...
 */

typedef struct
{
  unsigned char	digest[16];	/* MD5 digest of original text */
  const char	*text;		/* Text in current language */
} espmsg_t;


/*
 * Prototypes...
 */

extern int		espAddMessage(const char *orig, const char *text,
			              int num_msgs, espmsg_t **msgs);
extern void		espFreeMessages(int num_msgs, espmsg_t *msgs);
extern int		espLoadMessages(const char *filename, espmsg_t **msgs);
extern const char	*espLookupMessage(const char *orig, int num_msgs,
			                  espmsg_t *msgs);
extern int		espSaveMessages(const char *filename, int num_msgs,
			                espmsg_t *msgs);


/*
 * C++ magic...
 */

#  ifdef __cplusplus
}
#  endif /* __cplusplus */

#endif /* !_ESPMSG_H_ */


/*
 * End of "$Id: espmsg.h 321 2005-01-23 03:52:44Z easysw $".
 */
