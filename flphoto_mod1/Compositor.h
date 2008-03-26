//
// "$Id: Compositor.h 326 2005-01-25 07:29:17Z easysw $"
//
// Compositor class definitions for flPhoto.
//
// Copyright 2005 by Michael Sweet
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

//
// Include necessary headers...
//



//
// Image compositor class...
//

class Compositor
{
  unsigned char	rgb_[3];		// Background color
  int		w_;			// Width
  int		h_;			// Height
  unsigned char	*pixels_;		// RGB pixels

  public:

		Compositor(int W, int H);
		~Compositor();

  int		h() const { return (h_); }
  void		image(int dX, int dY, int dW, int dH,
		      int W, int H, int D, int A, bool r, unsigned char *p);
  unsigned char	*pixels() { return (pixels_); }
  void		rgb(unsigned char r, unsigned char g, unsigned char b) { rgb_[0] = r; rgb_[1] = g; rgb_[2] = b; }
  void		resize(int W, int H);
  int		w() const { return (w_); }
};


//
// End of "$Id: Compositor.h 326 2005-01-25 07:29:17Z easysw $".
//
