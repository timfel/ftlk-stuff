//
// "$Id: http-md5.cxx 321 2005-01-23 03:52:44Z easysw $"
//
// MD5 password support for flPhoto.
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
//   HTTP::md5()        - Compute the MD5 sum of the username:group:password.
//   HTTP::md5Nonce()   - Combine the MD5 sum of the username, group, and
//                          password with the server-supplied nonce value.
//   HTTP::md5_string() - Convert an MD5 sum to a character string.
//

/*
 * Include necessary headers...
 */

#include "http.h"
#include "flstring.h"


/*
 * 'HTTP::md5()' - Compute the MD5 sum of the username:group:password.
 */

char *					/* O - MD5 sum */
HTTP::md5(const char *username,		/* I - User name */
          const char *realm,		/* I - Realm name */
          const char *passwd,		/* I - Password string */
	  char       *md5,		/* O - MD5 string */
	  int        md5len)		/* I - Size of MD5 string */
{
  MD5		state;			/* MD5 state info */
  MD5Byte	sum[16];		/* Sum data */
  char		line[256];		/* Line to sum */


 /*
  * Compute the MD5 sum of the user name, group name, and password.
  */

  snprintf(line, sizeof(line), "%s:%s:%s", username, realm, passwd);
  state.init();
  state.append((MD5Byte *)line, strlen(line));
  state.finish(sum);

 /*
  * Return the sum...
  */

  return (HTTP::md5_string(sum, md5, md5len));
}


/*
 * 'HTTP::md5_final()' - Combine the MD5 sum of the username, group, and password
 *                       with the server-supplied nonce value, method, and
 *                       request-uri.
 */

char *					/* O - New sum */
HTTP::md5_final(const char *nonce,	/* I - Server nonce value */
                const char *method,	/* I - METHOD (GET, POST, etc.) */
	        const char *resource,	/* I - Resource path */
	        char       *md5,	/* O - MD5 string */
	        int        md5len)	/* I - Size of MD5 string */
{
  MD5		state;			/* MD5 state info */
  MD5Byte	sum[16];		/* Sum data */
  char		line[1024];		/* Line of data */
  char		a2[33];			/* Hash of method and resource */


 /*
  * First compute the MD5 sum of the method and resource...
  */

  snprintf(line, sizeof(line), "%s:%s", method, resource);
  state.init();
  state.append((MD5Byte *)line, strlen(line));
  state.finish(sum);
  HTTP::md5_string(sum, a2, sizeof(a2));

 /*
  * Then combine A1 (MD5 of username, realm, and password) with the nonce
  * and A2 (method + resource) values to get the final MD5 sum for the
  * request...
  */

  snprintf(line, sizeof(line), "%s%s:%s", md5, nonce, a2);

  state.init();
  state.append((MD5Byte *)line, strlen(line));
  state.finish(sum);

  return (md5_string(sum, md5, md5len));
}


/*
 * 'HTTP::md5_string()' - Convert an MD5 sum to a character string.
 */

char *						/* O - MD5 sum in hex */
HTTP::md5_string(const MD5Byte *sum,		/* I - MD5 sum data */
	         char         *md5,		/* O - MD5 sun in hex */
	         int          md5len)		/* I - Size of MD5 string */
{
  int		i;				/* Looping var */
  char		*md5ptr;			/* Pointer into MD5 string */
  static char	*hex = "0123456789abcdef";	/* Hex digits */


 /*
  * Convert the MD5 sum to hexadecimal...
  */

  for (i = 16, md5ptr = md5; i > 0 && md5ptr < (md5 + md5len - 2); i --, sum ++)
  {
    *md5ptr++ = hex[*sum >> 4];
    *md5ptr++ = hex[*sum & 15];
  }

  *md5ptr = '\0';

  return (md5);
}


/*
 * End of "$Id: http-md5.cxx 321 2005-01-23 03:52:44Z easysw $".
 */
