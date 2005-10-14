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

#include "codegen_util.h"
#include "crc.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * Write the correct checksum for this quadrant in to the code
 */
#ifdef USE_CRC
void write_checksum(int quadrant, char sampledcode[EDGE_CELLS*EDGE_CELLS]) {
  computecrc(sampledcode+QUADRANT_SIZE*quadrant,QUADRANT_SIZE,0);
}
#else
void write_checksum(int quadrant, char sampledcode[EDGE_CELLS*EDGE_CELLS]) {
  int accumulator = 0;
  int i;
  for(i=0;i<QUADRANT_SIZE-CHECKSUM_BITS;++i) {
    accumulator += sampledcode[QUADRANT_SIZE*quadrant + i] & 1;
  }
  accumulator &= (1<<CHECKSUM_BITS) - 1;
  for(i=0;i<CHECKSUM_BITS;++i) {
    sampledcode[QUADRANT_SIZE*(quadrant+1)-CHECKSUM_BITS+i] = (accumulator >> (CHECKSUM_BITS-1))& 1;
    accumulator <<= 1;
  }
}
#endif

/*
 *  Encode data into the writecode string
 */
void encode(const char data[PAYLOAD_SIZE_BYTES], char writecode[EDGE_CELLS*EDGE_CELLS]) {
  int i;
  int pointer = 0;
  int current = data[pointer++];
  int currentShift = 0;

  for(i=0;i<EDGE_CELLS*EDGE_CELLS;++i) {
    if (i==0) {
      writecode[i] = 1;
    }
    else if (i % QUADRANT_SIZE == 0) {
      writecode[i] = 0;
    }
    else if (i % QUADRANT_SIZE < QUADRANT_SIZE - CHECKSUM_BITS ) {
      writecode[i] = (current >> 7) & 1;
      current <<= 1;
      ++currentShift;
      if (currentShift == 8) {
	current = data[pointer++];
	currentShift=0;
      }
    }
  }

  for(i=0;i<4;++i) {
    write_checksum(i,writecode);
  }
}

static void drawcell(unsigned char* image, int row, int col, int colour) {
  const int cell_size = (IMAGE_WIDTH < IMAGE_HEIGHT ? IMAGE_WIDTH : IMAGE_HEIGHT) / (EDGE_CELLS + 2);
  int i;
  int j;
  for(i=0;i<cell_size;++i) {
    for(j=0;j<cell_size;++j) {
      image[ (row*cell_size+i)*IMAGE_BYTES_PER_LINE + col*cell_size+j ] = colour;
    }
  }
}

void draw(unsigned char* image, const char writecode[EDGE_CELLS*EDGE_CELLS],
	  int* read_order) {
  int i;
  int j;
  int pointer = 0;
  for(i=0;i<EDGE_CELLS+2;++i) {
    drawcell(image,i,0,0);
    drawcell(image,i,EDGE_CELLS+1,0);
    drawcell(image,0,i,0);
    drawcell(image,EDGE_CELLS+1,i,0);
  }
  for(i=1;i<EDGE_CELLS+1;++i) {
    for(j=1;j<EDGE_CELLS+1;++j) {
      drawcell(image,i,j,writecode[read_order[pointer++]] ? 0 : 255);
    }
  }

  
#ifdef IMAGE_DEBUG
  save_image(image,"debug-encode.pnm",0);
#endif
}

void parse_code(char* text, unsigned char code[PAYLOAD_SIZE_BYTES]) {
  int len = strlen(text);
  int temp;
  int i;
  if (len > 2*PAYLOAD_SIZE_BYTES) {
    len = 2*PAYLOAD_SIZE_BYTES;
    text[len] = 0;
  }

  char* even_text = (char*)malloc((len+2) * sizeof(char));
  strncpy(even_text, text, len+1);
  if (len % 2 != 0) {
    even_text[len] = '0';
    even_text[len+1] = 0;
    ++len;
  }
  for(i=len-2;i>=0;i-=2) {
    sscanf(even_text+i,"%x",&temp);
    code[i/2] = (unsigned char)temp;
    *(even_text+i) = 0;
  }
  free(even_text);
}

void populate_read_order(int* read_order) {
  int i;
  int mod;
  int div;
  for(i=0;i<QUADRANT_SIZE;++i) {
    mod = i % (EDGE_CELLS / 2);
    div = i / (EDGE_CELLS / 2);    
    read_order[ mod + div * EDGE_CELLS ] = i;
    read_order[ mod * EDGE_CELLS + EDGE_CELLS - 1 - div ] = i+QUADRANT_SIZE;
    read_order[ EDGE_CELLS - 1 - mod + (EDGE_CELLS - 1 - div) * EDGE_CELLS ] = i+2*QUADRANT_SIZE;
    read_order[ (EDGE_CELLS - 1 - mod) * EDGE_CELLS + div ] = i+3*QUADRANT_SIZE;
  }
}
