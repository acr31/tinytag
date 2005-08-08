/*
 * Copyright 2005 Andrew Rice
 * Copyright 2005 Intel Corporation (Author: Richard Sharp)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *  
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 * Email: acr31@cam.ac.uk
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "crc.h"

#define PROTECT_SIZE ((QUADRANT_SIZE<<2)-CHECKSUM_BITS)
#include "polydef.h"

#ifndef POLYNOMIAL
# error No suitable polynomial found!
#endif 

#define TOPBIT   (1 << (CHECKSUM_BITS - 1))

int computecrc(unsigned char message[], int nBits, int check) {
  int i;
  unsigned long remainder = 0;

  nBits -= CHECKSUM_BITS;

  for (i = 0; i<nBits ; ++i) {
    if (remainder & TOPBIT) {
      remainder ^= POLYNOMIAL;
    }
    remainder <<= 1;
    remainder &= (1<<CHECKSUM_BITS)-1;
    remainder |= message[i] & 0x1;
  }

  if (check) {
    for(i = 0; i<CHECKSUM_BITS;++i) {
      if ((remainder & 0x1u) != (message[nBits+i] & 0x1u)) return 0;
      remainder >>= 1;
    }
    return 1;
  }
  else {
    for(i=0;i<CHECKSUM_BITS;++i) {
      message[nBits+i] = remainder & 0x1;
      remainder >>= 1;
    }
  }
  return 1;
}

