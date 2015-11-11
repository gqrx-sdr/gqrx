/*
 *      filter.h -- optimized filter routines
 *
 *      Copyright (C) 1996  
 *          Thomas Sailer (sailer@ife.ee.ethz.ch, hb9jnx@hb9w.che.eu)
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/* ---------------------------------------------------------------------- */

#ifndef _FILTER_H
#define _FILTER_H

/* ---------------------------------------------------------------------- */

#ifdef ARCH_I386
#include "filter-i386.h"
#endif /* ARCH_I386 */

/* ---------------------------------------------------------------------- */

extern inline unsigned int hweight32(unsigned int w)
        __attribute__ ((unused));
extern inline unsigned int hweight16(unsigned short w)
        __attribute__ ((unused));
extern inline unsigned int hweight8(unsigned char w)
        __attribute__ ((unused));

extern inline unsigned int hweight32(unsigned int w) 
{
        unsigned int res = (w & 0x55555555) + ((w >> 1) & 0x55555555);
        res = (res & 0x33333333) + ((res >> 2) & 0x33333333);
        res = (res & 0x0F0F0F0F) + ((res >> 4) & 0x0F0F0F0F);
        res = (res & 0x00FF00FF) + ((res >> 8) & 0x00FF00FF);
        return (res & 0x0000FFFF) + ((res >> 16) & 0x0000FFFF);
}

extern inline unsigned int hweight16(unsigned short w)
{
        unsigned short res = (w & 0x5555) + ((w >> 1) & 0x5555);
        res = (res & 0x3333) + ((res >> 2) & 0x3333);
        res = (res & 0x0F0F) + ((res >> 4) & 0x0F0F);
        return (res & 0x00FF) + ((res >> 8) & 0x00FF);
}

extern inline unsigned int hweight8(unsigned char w)
{
        unsigned short res = (w & 0x55) + ((w >> 1) & 0x55);
        res = (res & 0x33) + ((res >> 2) & 0x33);
        return (res & 0x0F) + ((res >> 4) & 0x0F);
}

extern inline unsigned int gcd(unsigned int x, unsigned int y)
        __attribute__ ((unused));
extern inline unsigned int lcm(unsigned int x, unsigned int y)
        __attribute__ ((unused));

extern inline unsigned int gcd(unsigned int x, unsigned int y)
{
        for (;;) {
                if (!x)
                        return y;
                if (!y) 
                        return x;
                if (x > y)
                        x %= y;
                else
                        y %= x;
        }
}

extern inline unsigned int lcm(unsigned int x, unsigned int y)
{
        return x * y / gcd(x, y);
}

/* ---------------------------------------------------------------------- */

//#ifndef __HAVE_ARCH_MAC
inline float mac(const float *a, const float *b, unsigned int size)
{
	float sum = 0;
	unsigned int i;

	for (i = 0; i < size; i++)
		sum += (*a++) * (*b++);
	return sum;
}
//#endif /* __HAVE_ARCH_MAC */

inline float fsqr(float f)
{
	return f*f;
}

/* ---------------------------------------------------------------------- */
#endif /* _FILTER_H */
