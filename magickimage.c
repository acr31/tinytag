/*
 * Copyright 2005 Andrew Rice
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

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <magick/magick_wand.h>

#include "image.h"
#include "options.h"

int save_image(const unsigned char* data,const char* filename, int binary) {
  MagickWand *magick_wand = NewMagickWand();
  MagickBooleanType status;

  status = MagickConstituteImage(magick_wand,IMAGE_WIDTH,IMAGE_HEIGHT,"I",CharPixel,(void*)data);
  if (!status) {
    printf("Failed to create image for saving!\n");
    DestroyMagickWand(magick_wand);
    return 0;
  }
  MagickResetIterator(magick_wand);
  if (MagickNextImage(magick_wand) != MagickFalse) {
    if (binary) MagickEqualizeImage(magick_wand);
    status = MagickWriteImage(magick_wand,filename);
    if (!status) {
      printf("Failed to save image!\n");
      DestroyMagickWand(magick_wand);
      return 0;
    }
  }
  else {
    printf("MagickWand is empty!\n");
    DestroyMagickWand(magick_wand);
    return 0;
  }
  
  DestroyMagickWand(magick_wand);
  return 1;
}


unsigned char* load_image(const char* filename) {
  MagickWand *magick_wand = NewMagickWand();
  MagickBooleanType status;
  unsigned char* result ;

  status = MagickReadImage(magick_wand,filename);
  if (!status) {
    printf("Failed to read image!\n");
    DestroyMagickWand(magick_wand);
    return NULL;
  }

  assert(IMAGE_BYTES_PER_LINE == IMAGE_WIDTH);
  result = (unsigned char*)malloc(IMAGE_BYTES_PER_LINE*IMAGE_HEIGHT);
  MagickResetIterator(magick_wand);
  if (MagickNextImage(magick_wand) != MagickFalse) {
    status = MagickGetImagePixels(magick_wand,0,0,IMAGE_WIDTH,IMAGE_HEIGHT,"I",CharPixel,result);
    DestroyMagickWand(magick_wand);
    return result;
  }
  else {
    printf("MagickWand is empty!\n");
    DestroyMagickWand(magick_wand);
    return NULL;    
  }    
}
