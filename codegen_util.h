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

#include "options.h"

void write_checksum(int quadrant, unsigned char sampledcode[EDGE_CELLS*EDGE_CELLS]);
void encode(const unsigned char data[PAYLOAD_SIZE_BYTES], unsigned char writecode[EDGE_CELLS*EDGE_CELLS]);
void draw(unsigned char* image, const unsigned char writecode[EDGE_CELLS*EDGE_CELLS], int* read_order);
void parse_code(char* text, unsigned char code[PAYLOAD_SIZE_BYTES]);
void populate_read_order(int* read_order);
