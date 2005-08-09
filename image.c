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

#include "image.h"
#include "options.h"

static int readline(int handle,  char* buffer, int maxlen) {
  int readcount = 0;
  int readresult = 1;
  char* start = buffer;
  while(readcount < maxlen && readresult > 0) {
    readresult = read(handle,buffer,1);

    if (readresult == -1) {
      return -1;
    }

    readcount+=readresult;

    if (*buffer == '\n') {
      *buffer = 0;
      break;
    }

    buffer+=readresult;

  }

  if (*start == '#') {
    return readline(handle,start,maxlen);
  }
  else {
    return 1;
  }
}

static int writeline(char* buffer, int handle) {
  int result;
  int length = strlen((char*)buffer);

  while(length > 0) {
    result = write(handle,buffer,length);
    if (result == -1) return 0;
    length-=result;
  }
  return 1;
}


/*
 * Save this image to the file given.  If the flag "binary" is set
 * then it is assumed that we have 1-bit data.  Files are always saved
 * as greyscale, ascii pnms
 */ 
int save_image(const unsigned char* data, const char* filename, int binary) {
  int handle = open(filename,O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR );
  char buffer[255];
  int i,j;

  if (!handle) return 0;

  snprintf(buffer,255,"P2\n%d %d\n255\n",IMAGE_WIDTH,IMAGE_HEIGHT);
  if (!writeline(buffer,handle)) {
    close(handle);
    return 0;
  }


  for(i=0;i<IMAGE_HEIGHT;++i) {
    for(j=0;j<IMAGE_WIDTH;++j) {
      int value = data[IMAGE_WIDTH*i+j];
      value = binary ? (value ? 255 : 0) : value;
      snprintf(buffer,255,"%d\n",value);

      if (!writeline(buffer,handle)) {
	close(handle);
	return 0;
      }
    }
  }


  close(handle);
  return 1;
}

/*
 * Load a greyscale ascii pnm image of size IMAGE_WIDTH x
 * IMAGE_HEIGHT, allocate a suitable buffer and return a pointer to
 * it. Will return NULL on any error.  If you get something that is
 * non-null you have to free it yourself.
 */
unsigned char* load_image(const char* filename) {
  int handle = open(filename,O_RDONLY);
  char buffer[255];
  int width;
  int height;
  int colour_count;
  unsigned char* result;
  int failed,i,j;

  if (!handle) return NULL;
  if ((readline(handle,buffer,255) < 0) ||
      (strncmp(buffer,"P2",2) != 0) ||
      (readline(handle,buffer,255) < 0) ||
      (sscanf(buffer,"%d %d",&width,&height) == 0) ||
      (width != IMAGE_WIDTH || height != IMAGE_HEIGHT) ||
      (readline(handle,buffer,255) < 0) ||
      (sscanf(buffer,"%d",&colour_count) == 0) ||
      (colour_count != 255)) {
    close(handle);
    return NULL;
  }

  result = (unsigned char*)malloc(IMAGE_HEIGHT*IMAGE_WIDTH);

  failed = 0;  
  for(i=0;i<IMAGE_HEIGHT;++i) {
    for(j=0;j<IMAGE_WIDTH;++j) {
      if ((readline(handle,buffer,255) < 0) ||
	  (sscanf(buffer,"%d",&colour_count) == 0)) { 
	failed = 1;
	break;
      }
      result[IMAGE_WIDTH*i+j] = (unsigned char)colour_count;            
    }
  }
  close(handle);

  if (failed) {
    free(result);
    return NULL;
  }


  return result;
}


