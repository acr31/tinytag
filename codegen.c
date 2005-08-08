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
#include <string.h>

#include "options.h"
#include "image.h"
#include "recognise.h"
#include "codegen_util.h"

int main(int argc,char** argv) {

  unsigned char code[PAYLOAD_SIZE_BYTES] = {0};
  unsigned char samplebuffer[EDGE_CELLS*EDGE_CELLS] = {0};
  unsigned char image[IMAGE_BYTES_PER_LINE*IMAGE_WIDTH];
  int read_order[EDGE_CELLS*EDGE_CELLS];


  if (argc != 3) {
    printf("%s {hex tag data} {output filename}\n",argv[0]);
    return -1;
  }

  memset(image,255,IMAGE_BYTES_PER_LINE*IMAGE_WIDTH);
  populate_read_order(read_order);

  parse_code(argv[1],code);
  encode(code,samplebuffer);
  draw(image,samplebuffer,read_order);
  save_image(image,argv[2],0);
  return 1;
}
