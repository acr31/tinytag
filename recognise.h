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

struct Point {
  int x;
  int y;
};

void global_threshold(unsigned char* image);
void adaptive_threshold(unsigned char* image);

enum curvature_result_t { NOCORNER, MOVENEXT, CORNER };

enum curvature_result_t check_curvature(long long* previous_curvature_fp, long long* current_max_curvature_fp, int length);

int findquad(unsigned char* image, int* raster_x, int* raster_y, struct Point* point);


void sample_code(unsigned char* image, char sampledcode[EDGE_CELLS*EDGE_CELLS], struct Point quadtangle[4], int* read_order);

int check_checksum(int quadrant, const char sampledcode[EDGE_CELLS*EDGE_CELLS]);
void write_checksum(int quadrant, char sampledcode[EDGE_CELLS*EDGE_CELLS]);
int decode(const char sampledcode[EDGE_CELLS*EDGE_CELLS], char data[PAYLOAD_SIZE_BYTES]);
void encode(const char data[PAYLOAD_SIZE_BYTES], char writecode[EDGE_CELLS*EDGE_CELLS]);
void parse_code(char* text, unsigned char code[PAYLOAD_SIZE_BYTES]);

int process_image(unsigned char* data, unsigned char code[PAYLOAD_SIZE_BYTES], int* read_order);

extern struct Point bbox_top_left;
extern struct Point bbox_bottom_right;
extern int tracked_tag;
