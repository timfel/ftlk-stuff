//
// "$Id: md5.h 321 2005-01-23 03:52:44Z easysw $"
//
// MD5 support code for flPhoto.
//
// Copyright 2002-2005 by Michael Sweet.
// Copyright 1999 Aladdin Enterprises.  All rights reserved.
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//
// L. Peter Deutsch
// ghost@aladdin.com
//

//
// Independent implementation of MD5 (RFC 1321).
//
// This code implements the MD5 Algorithm defined in RFC 1321.
// It is derived directly from the text of the RFC and not from the
// reference implementation.
//
// The original and principal author of md5.h is L. Peter Deutsch
// <ghost@aladdin.com>.  Other authors are noted in the change history
// that follows (in reverse chronological order):
//
// 1999-11-04 lpd Edited comments slightly for automatic TOC extraction.
// 1999-10-18 lpd Fixed typo in header comment (ansi2knr rather than md5);
//	added conditionalization for C++ compilation from Martin
//	Purschke <purschke@bnl.gov>.
// 1999-05-03 lpd Original version.
//

#ifndef _HTMLDOC_MD5_H_
#  define _HTMLDOC_MD5_H_

//
// This code has some adaptations for the Ghostscript environment, but it
// will compile and run correctly in any environment with 8-bit chars and
// 32-bit ints.  Specifically, it assumes that if the following are
// defined, they have the same meaning as in Ghostscript: P1, P2, P3,
// ARCH_IS_BIG_ENDIAN.
//

typedef unsigned char	MD5Byte;
typedef unsigned int	MD5Word;

// Define the state of the MD5 Algorithm.
struct MD5
{
  MD5Word count[2];			// message length in bits, lsw first
  MD5Word abcd[4];			// digest buffer
  MD5Byte buf[64];			// accumulate block

  void	append(const MD5Byte *data, int nbytes);
  void	finish(MD5Byte *digest);
  void	init();
  void	process(const MD5Byte *data);
};

#endif // !_HTMLDOC_MD5_H_


//
// End of "$Id: md5.h 321 2005-01-23 03:52:44Z easysw $".
//
