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

#include <stdio.h>
#include <stdlib.h>

#include "options.h"
#include "v4l.h"
#include "recognise.h"
#include "codegen_util.h"
#include "image.h"
#include <string.h>

int main(int argc,char** argv) {
  unsigned char decoded[PAYLOAD_SIZE_BYTES] = {0};
  int read_order[EDGE_CELLS*EDGE_CELLS];
  struct v4l_handle* vhandle;
  unsigned char* data;
  int i;
  int counter = 0;
  if (argc != 2) {
    printf("%s [video device]\n",argv[0]);
    exit(-1);
  }

  populate_read_order(read_order);

  vhandle = v4l_open(argv[1]);
  if (vhandle == 0) {
    printf("Failed to open and initialise Video4Linux\n");
    exit(-1);
  }

  while (++counter < 2) {
    data = v4l_next(vhandle);
    
    if (data == 0) {
      printf("Failed to grab image\n");
    }
    else {
      if (process_image(data,decoded,read_order)) {
	printf("Decoded:\t");
	for(i=0;i<PAYLOAD_SIZE_BYTES;++i) {
	  printf("%.2X ",decoded[i]);
	}
	printf("\n");
      }
    }
  }
  
  v4l_close(vhandle);

  return 1;
}
