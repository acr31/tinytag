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

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "options.h"
#include "image.h"
#include "recognise.h"
#include "crc.h"
#include "codegen_util.h"

#define MASK(x) ((x) & ((1<<LOG_CURVATURE_WINDOW)-1))


/*
 * X0 +------+ X1
 *    |      | 
 * X3 +------+ X2
 */
#define X0 0
#define X1 1
#define X2 2
#define X3 3

/* min and max functions: */

static int min(int x, int y) { return x<y?x:y; }
static int max(int x, int y) { return x>y?x:y; }

/* globals to record bounding box of last tag decode */
struct Point bbox_top_left;
struct Point bbox_bottom_right;
int tracked_tag;

long long COS_CURVATURE_THRESHOLD_FP;

void global_threshold(unsigned char* image) {
  int i,j;
  unsigned char* data_pointer;
  for(i=0;i<IMAGE_HEIGHT;++i) {
    data_pointer = image + IMAGE_BYTES_PER_LINE * i;
    for(j=0;j<IMAGE_WIDTH;++j,++data_pointer) {
      *data_pointer = (*data_pointer > GLOBAL_THRESHOLD) ? 0x0 : 0x1;      
    }
  }
#ifdef IMAGE_DEBUG
  save_image(image,"debug-globalthreshold.pnm",1);
#endif
}

/*
 * Adaptive threshold
 *
 * Apply an adaptive threshold to the image.  For each pixel a region
 * of 2^window_size pixels around it is averaged.  If the pixel is
 * greater than this average+offset then it is changed to 0 and
 * otherwise changed to 1.  Note that this method is also inverting
 * the image in addition to binarizing it.  We use 2^window_size and
 * strength reduce the divisions in the moving average.
 *
 * See: Pierre Wellner, "Adaptive Thresholding for the DigitalDesk",
 * EuroPARC Technical Report Number EPC-93-110
 */
void adaptive_threshold(unsigned char* image) {
  int moving_average = 127;
  const int useoffset = 255-ADAPTIVE_THRESHOLD_OFFSET;
  unsigned char* data_pointer;
  int current_thresh, pixel, i,j;

  int previous_line[IMAGE_WIDTH]={0};  /* intentionally uninitialised */
  /*  for(i=0;i<IMAGE_WIDTH;++i) { previous_line[i] = 127; } */
  
  for(i=0;i<IMAGE_HEIGHT-1;) { /* use height-1 so we dont overrun the image if its height is an odd number */
    data_pointer = image + IMAGE_BYTES_PER_LINE * i;
    for(j=0;j<IMAGE_WIDTH;++j) {
      pixel = *data_pointer;
      moving_average = pixel + moving_average - (moving_average >> LOG_ADAPTIVE_THRESHOLD_WINDOW);
      current_thresh = (moving_average + previous_line[j])>>1;
      previous_line[j] = moving_average;      
      *data_pointer = (pixel << (LOG_ADAPTIVE_THRESHOLD_WINDOW+8)) < (current_thresh * useoffset) ? 0x1 : 0x0;
      ++data_pointer;
    }
    
    ++i;
    data_pointer = image + IMAGE_BYTES_PER_LINE * i + IMAGE_WIDTH - 1;
    for(j=IMAGE_WIDTH-1;j>=0;--j) {
      pixel = *data_pointer;
      moving_average = pixel + moving_average - (moving_average >> LOG_ADAPTIVE_THRESHOLD_WINDOW);
      current_thresh = (moving_average + previous_line[j])>>1;
      previous_line[j] = moving_average;
      *data_pointer = (pixel << (LOG_ADAPTIVE_THRESHOLD_WINDOW+8)) < (current_thresh * useoffset)  ? 0x1 : 0x0;
      --data_pointer;
    }
    ++i;
  }

#ifdef IMAGE_DEBUG
  save_image(image,"debug-adaptivethreshold.pnm",1);
#endif
}


/*
 * When we walk the contour we label the region around the pixel as follows
 *
 * +---+---+---+
 * | 7 | 6 | 5 |
 * +---+---+---+
 * | 0 |   | 4 |
 * +---+---+---+
 * | 1 | 2 | 3 |
 * +---+---+---+
 *
 * Define the array offset to give us the offset from our current
 * position in the image for the relevant pixel
 */
static int offset[] = { -1,
			IMAGE_BYTES_PER_LINE-1,
			IMAGE_BYTES_PER_LINE,
			IMAGE_BYTES_PER_LINE+1,
			1,
			-IMAGE_BYTES_PER_LINE+1,
			-IMAGE_BYTES_PER_LINE,
			-IMAGE_BYTES_PER_LINE-1 };
static int xmove[] = { -1,-1,0,1,1,1,0,-1 };
static int ymove[] = { 0,1,1,1,0,-1,-1,-1 };

/*
 * These arrays hold a sliding window of points on our contour which
 * we use for curvature estimation
 */
static int xwindow[1<<LOG_CURVATURE_WINDOW];
static int ywindow[1<<LOG_CURVATURE_WINDOW];

enum curvature_result_t check_curvature(long long* previous_curvature_fp, long long* current_max_curvature_fp, int length) {
  enum curvature_result_t result = NOCORNER;

  /* end_index is the point we've just written (youngest pt in sliding
   window) index is the candidate corner (middle of sliding window)
   start_index is the oldest point in the sliding window */

  const int end_index = MASK(length);
  const int index = MASK(length+(1<<(LOG_CURVATURE_WINDOW-1)));
  const int start_index = MASK(end_index+1);
  const int ax = xwindow[end_index] - xwindow[index];
  const int ay = ywindow[end_index] - ywindow[index];
  const int bx = xwindow[start_index] - xwindow[index];
  const int by = ywindow[start_index] - ywindow[index];

  /*
  const float moda = ax*ax+ay*ay;
  const float modb = bx*bx+by*by;
  const float curvature = moda == 0 || modb == 0 ? 0 : -(ax*bx+ay*by)*(ax*bx+ay*by)/moda/modb;
  */

  /* and, here's the fixed point version: */

  const int moda_i = ax*ax+ay*ay;
  const int modb_i = bx*bx+by*by;
  const long long numerator_fp   = ((long long)((ax*bx+ay*by)*(ax*bx+ay*by))) << FRAC_BITS;
  const long long denominator_fp = ((long long)(moda_i*modb_i)) << FRAC_BITS;
  long long curvature_fp;

  if (moda_i==0 || modb_i==0) curvature_fp=0;
  else curvature_fp = (numerator_fp<<FRAC_BITS) / denominator_fp;

  /* printf("numerator_fp = %lld, denominator_fp = %lld, curvature_fp = %lld\n",numerator_fp,denominator_fp,curvature_fp);  

  printf("index=%d, end_index=%d, start_index=%d\n",index,end_index,start_index);
  printf("ax=%i ay=%i bx=%i by=%i moda=%f modb=%f curvature=%f\n",ax,ay,bx,by,moda,modb,curvature); */

  if (curvature_fp > COS_CURVATURE_THRESHOLD_FP) {
    if (*previous_curvature_fp < COS_CURVATURE_THRESHOLD_FP) {
      *current_max_curvature_fp = 10LL << FRAC_BITS;
      result = MOVENEXT;
    }
  }
  else {
    if (curvature_fp < *current_max_curvature_fp) {
      *current_max_curvature_fp = curvature_fp;
      result = CORNER;
    }
  }

  *previous_curvature_fp = curvature_fp;
  return result;
}

#ifdef IMAGE_DEBUG
static unsigned char* contour_image_debug = NULL;
static unsigned char* corner_image_debug = NULL;
static unsigned char* sample_image_debug = NULL;
#endif

/*
 * Walk this contour marking every contour as "seen".  If this is an
 * outer border then maintain a sliding window with which we measure
 * the contour curvature.  If this contour turns out to have four good
 * corners and covers a good proportion of the image then we accept
 * it as a quadrangle and return true.  
 */
static int walk_contour(unsigned char* data_pointer, struct Point* result, int outer_border, int currentx, int currenty) {
  int position = outer_border ? 0 : 4;
  int start_position = position;
  unsigned char* contour_0;
  unsigned char* contour_n; /* pointer so we know when to stop */
  unsigned char* sample_pointer;
  int cell4_is_0;
  int length;
  int corner_counter = 0;
  enum curvature_result_t curvature_result;
  long long previous_curvature = 1LL << FRAC_BITS;
  long long current_max_curvature = 10LL << FRAC_BITS;

  /* search for the start position */
  do {
    position = (position - 1) & 0x7;
    sample_pointer = data_pointer+offset[position];
    if (*sample_pointer) { break; }    
  }
  while (position != start_position);

  if (position == start_position) { /* one pixel contour */
    *data_pointer = 3;
    return 0;
  }
  else {
    position = (position + 1) & 0x7;
    length = 1;
    /* we'll set this to true when we search a region if we pass cell4 and its a 0-element */
    cell4_is_0 = 0;

    /* data_pointer represents a point pending acceptance as part of the current contour */
    /* we move sample_pointer around data_pointer looking for the next point on the contour */ 
    /* currentx and currenty yield the co-ordinates pointed to by data_pointer */
    while(1) {
      sample_pointer = data_pointer + offset[position];
      if (*sample_pointer) { 
	/* if data_pointer lies on the contour already and
	   sample_pointer does too then we know we should stop because
	   we have come full circle 	  
	*/
	if ((length > 1<<LOG_CURVATURE_WINDOW) &&
	    (data_pointer == contour_0) &&
	    (sample_pointer == contour_n)) {
	  break;
	}

#ifdef IMAGE_DEBUG
	contour_image_debug[currentx+currenty*IMAGE_BYTES_PER_LINE] = 0;
#endif 

	/* if we have covered a curvature window of points then
	   set our stopping condition - this ensures we walk a little
	   bit past the end of the contour in order to find the
	   corners there */
	if (length == 1<<(LOG_CURVATURE_WINDOW)) {
	  contour_0 = data_pointer;
	  contour_n = sample_pointer;
	}

	/* now mark data_pointer so we know how to interpret it in future */

	/* 1) if the pixel at cell4 is a 0-element and we have
	 * examined it when looking for this 1-element (i.e. cell4_is_0 is
	 * true) then this is an exit pixel 
	 */
	if (cell4_is_0) {
	  *data_pointer = 2;
	}
	/* 2) else if this cell is unmarked write that this is not an exit pixel */
	else if (! (*data_pointer & 2)) {
	  *data_pointer = 3;
	}
	
	/* if we are looking for corners then check the curvature */
	if (outer_border && corner_counter < 5) {
	  xwindow[MASK(length)] = currentx;
	  ywindow[MASK(length)] = currenty;
	  if (length > 1<<LOG_CURVATURE_WINDOW) {
	    curvature_result = check_curvature(&previous_curvature,&current_max_curvature,length);
	    if (curvature_result == MOVENEXT) {
	      ++corner_counter;
	    }
	    else if (curvature_result == CORNER) {
	      /* the candidate corner is half-way back through sliding window!
		 must correct for this... */

	      const int index = MASK(length+(1<<(LOG_CURVATURE_WINDOW-1)));
	      result[corner_counter].x = xwindow[index];
	      result[corner_counter].y = ywindow[index];	      	      
	    }
	  }
	}

	/* otherwise, we accept data_pointer as being a point on the contour */
	++length;

	/* now move on to checking is sample_pointer is on the contour */
	data_pointer = sample_pointer; 
	currentx += xmove[position];
	currenty += ymove[position];	

	cell4_is_0 = 0;

	/*
	 * if we find the 1-element at position n anti-clockwise then we
	 * need to shift the region to be centered on the new point and
	 * resume searching one place after the previous central point
	 * i.e. if we find a point at 3 we need to resume searching from
	 * 0 (one place past the old centre at 7)
	 *
	 *    +---+---+---+---+         +---+---+---+---+   
	 *    | 7 | 6 | 5 |   |         |   |   |   |   |
	 *    +---+---+---+---+         +---+---+---+---+   
	 *    | 0 |   | 4 |   |         |   |_7_| 6 | 5 |
	 *    +---+---+---+---+   -->   +---+---+---+---+   
	 *    | 1 | 2 | X |   |         |   | 0 | X | 4 |
	 *    +---+---+---+---+         +---+---+---+---+   
	 *    |   |   |   |   |         |   | 1 | 2 | 3 |
	 *    +---+---+---+---+         +---+---+---+---+   
	 *
	 *  Found posn (anti)       |  Resume posn (anti)
	 *  -----------------------------------------------
	 *          0               |         5
	 *          1               |         6
	 *          2               |         7
	 *          3               |         0
	 *          4               |         1
	 *          5               |         2
	 *          6               |         3
	 *          7               |         4
	 */
	position = (position+5) & 0x7;
      }
      else { /* this is a 0-element */ 
	if (position == 4) { /* we are at cell 4 and its a 0-element so set the flag */
	  cell4_is_0 = 1;
	}
	
	/* advance the position */
	position = (position + 1) & 0x7;
      }
    }
    
    return corner_counter == 4;
  }
}

/*
 * Raster across the binarized image looking at pixel transistions.
 * If we see a previously unseen transition then walk the contour from
 * there.  Pass whether this is an outer border (0-element ->
 * 1-element) or an inner border so that we know whether to check for
 * four corners that indicates a quadrangle
 */
int findquad(unsigned char* image, int* raster_x, int* raster_y, struct Point* point) {
  int seen_before,previous_is_1,next_is_1;
  unsigned char* data_pointer;

#ifdef IMAGE_DEBUG
  char debug_name_buffer[255];
  char debug_corner_buffer[255];
  char debug_sample_buffer[255];
  int debug_counter;
  sprintf(debug_name_buffer,"debug-contours-%d-%d.pnm",*raster_x,*raster_y);
  sprintf(debug_corner_buffer,"debug-corner-%d-%d.pnm",*raster_x,*raster_y);
  sprintf(debug_sample_buffer,"debug-sample-%d-%d.pnm",*raster_x,*raster_y);

  contour_image_debug = (unsigned char*)malloc(IMAGE_HEIGHT*IMAGE_BYTES_PER_LINE);
  corner_image_debug = (unsigned char*)malloc(IMAGE_HEIGHT*IMAGE_BYTES_PER_LINE);
  sample_image_debug = (unsigned char*)malloc(IMAGE_HEIGHT*IMAGE_BYTES_PER_LINE);
  for(debug_counter=0;debug_counter<IMAGE_HEIGHT*IMAGE_BYTES_PER_LINE;++debug_counter) {
    contour_image_debug[debug_counter] = image[debug_counter] ? 255 : 128;
    corner_image_debug[debug_counter] = image[debug_counter] ? 50 : 0;
    sample_image_debug[debug_counter] = image[debug_counter] ? 50 : 0;
  }
#endif

  for(;*raster_y < IMAGE_HEIGHT; ++(*raster_y)) {
    data_pointer = image + IMAGE_BYTES_PER_LINE * (*raster_y);
    ++data_pointer; /* exclude the first pixel on the line */
    ++(*raster_x);
    for(;*raster_x < IMAGE_WIDTH-1;++(*raster_x), ++data_pointer) {
      if (*data_pointer) {  /* this pixel is a 1-element or it has been visited before */
	seen_before = *data_pointer & 2;   /* this will be true if we've seen this pixel before */
	previous_is_1 = *(data_pointer-1);
	next_is_1 = *(data_pointer+1);
	if (!seen_before && !previous_is_1) {
	  if (walk_contour(data_pointer,point,1,*raster_x,*raster_y)) {
#ifdef IMAGE_DEBUG
	    /* plot new corners */
	    int i;
	    for(i=0;i<4;i++) corner_image_debug[point[i].x + IMAGE_BYTES_PER_LINE * point[i].y] = 255;

	    save_image(contour_image_debug,debug_name_buffer,0);
	    save_image(corner_image_debug,debug_corner_buffer,0);
	    free(corner_image_debug);
	    free(contour_image_debug);
#endif
	    return 1;
	  }
	}
	if (!seen_before && !next_is_1) {
	  walk_contour(data_pointer,point,0,*raster_x,*raster_y); /* can't return true */
	}
      }
    }
    *raster_x = 0; /* after we've been around the loop once we reset raster_x to restart at the beginning of the next line */
  }
#ifdef IMAGE_DEBUG
  save_image(contour_image_debug,debug_name_buffer,0);
  free(contour_image_debug);
#endif
  return 0;
}


/*
 *  Incremental Linear Interpolation
 *
 *  Finds n evenly spaced points between a and b (inclusive).
 *  Uses a generalization of Bresenham's line-drawing
 *  algorithm to do this accurately and only integer arithmetic.
 * 
 *  See: Dan Field, "Incremental Linear Interpolation" in ACM
 *  Transactions on Graphics, Vol 4. No 1, January 1985, pp 1--11
 * 
 *  a = beginning of range (inclusive)
 *  b = ending of range (inclusive)
 *  n = number of points required
 *  steps = array of size n to hold the result
 */
static void incremental_linear_interpolation(int a, int b, int n, int* steps) {
  int i,C1,C2,C3,C4,C5,x,r;
  C1 = b-a;
  C2 = C1 / n;
  
  if (C1 >= 0) {
    C3 = (C1-C2*n) << 1;
    r = C3 - n;
    C4 = r - n;
    C5 = C2 + 1;
  }
  else {
    C3 = (C2*n - C1) << 1;
    r = C3 - n - 1;
    C4 = C3 - n -n;
    C5 = C2 - 1;
  }
  x = a;
  for(i=0;i<=n;++i) {
    steps[i] = x;
    if (r>=0) {
      x = x+C5;
      r = r+C4;
    }
    else {
      x = x+C2;
      r = r+C3;
    }
  }
}


/* 
 * We sample the code in a rotationally invariant manner
 *
 *  +-------------------------+
 *  +  1  2  3  4 29 25 21 17 |
 *  +  5  6  7  8 30 26 22 18 |
 *  +  9 10 11 12 31 27 23 19 |
 *  + 13 14 15 16 32 28 24 20 |
 *  + 52 56 60 64 48 47 46 45 |
 *  + 51 55 59 63 44 43 42 41 |
 *  + 50 54 58 62 40 39 38 37 |
 *  + 49 53 57 61 36 35 34 33 |
 *  +-------------------------+
 *
 * This layout means that we can start reading clockwise from any
 * corner and all the codes we read will by cyclic rotations of each
 * other.  This means we could use any cyclic code (with code length
 * exactly matching the number of bits on the tag) and be sure that
 * rotations of the tag will not affect the hamming distances -
 * because all cyclic rotations of a valid codeword are also valid
 * codewords.  This means we cannot store data on the tag because we
 * can't be sure which of the four rotations is correct unless we
 * impose some extra structure.
 * 
 * We will use the ICC (independant chunk coding) system with a 2 bit
 * checksum. We consider each quadrant independantly.  The first bit
 * of the quadrant is an orientation bit and the trailing 2 bits are
 * the checksum - the checksum includes the orientation bit.
 *
 * See Andrew Rice, Christopher Cain, and John Fawcett, "Dependable
 * Coding of Fiducial Tags" in Proceedings of the 2nd International
 * Ubiquitous Computing Symposium 2004.
 *
 * Precompute an array of pointer which tell us the mapping from
 * rastering across the tag to the rotationally invariant reading.  We
 * will use this to indirect through when we read the data from the
 * tag.
 */

/*
 * Sample the points from the quadtangle.
 */
void sample_code(unsigned char* image, char sampledcode[EDGE_CELLS*EDGE_CELLS], struct Point quadtangle[4],
		 int* read_order) {
#ifdef IMAGE_DEBUG
  int debug_counter;
  sample_image_debug = (unsigned char*)malloc(IMAGE_HEIGHT*IMAGE_BYTES_PER_LINE);
  for(debug_counter=0;debug_counter<IMAGE_HEIGHT*IMAGE_BYTES_PER_LINE;++debug_counter) {
    sample_image_debug[debug_counter] = image[debug_counter] ? 128 : 0;
  }

  for(debug_counter=0;debug_counter<4;debug_counter++) {
    sample_image_debug[quadtangle[debug_counter].x + IMAGE_BYTES_PER_LINE*quadtangle[debug_counter].y] = 255;
  }
#endif


  /* do interpolation across opposite pairs of edges */
  const int num_interp_points = (EDGE_CELLS+2)*2;
  int edge1_xs[num_interp_points+1];
  int edge1_ys[num_interp_points+1];
  int edge2_xs[num_interp_points+1];
  int edge2_ys[num_interp_points+1];

  incremental_linear_interpolation(quadtangle[0].x, quadtangle[1].x, num_interp_points, edge1_xs);
  incremental_linear_interpolation(quadtangle[0].y, quadtangle[1].y, num_interp_points, edge1_ys);  
  incremental_linear_interpolation(quadtangle[3].x, quadtangle[2].x, num_interp_points, edge2_xs);
  incremental_linear_interpolation(quadtangle[3].y, quadtangle[2].y, num_interp_points, edge2_ys);  

  int i;
  int pointer = 0;
  int bits_edge_xs[num_interp_points+1];
  int bits_edge_ys[num_interp_points+1];

  for(i=3;i<=num_interp_points-3;i+=2) {
    /* now get a line which we can sample along to read bits... */
    incremental_linear_interpolation(edge1_xs[i], edge2_xs[i], num_interp_points, bits_edge_xs);
    incremental_linear_interpolation(edge1_ys[i], edge2_ys[i], num_interp_points, bits_edge_ys);
    
    /* and now do the sampling: */
    int j;
    for(j=3;j<=num_interp_points-3;j+=2) {
      int sample_x = bits_edge_xs[j];
      int sample_y = bits_edge_ys[j];
      sampledcode[read_order[pointer++]] = (image[sample_x + IMAGE_BYTES_PER_LINE*sample_y]) & 1;
      /*     printf("Wrote %d into %d position %d\n", (image[sample_x + IMAGE_BYTES_PER_LINE*sample_y]) & 1, read_order[pointer-1],pointer-1); */
#ifdef IMAGE_DEBUG
      sample_image_debug[sample_x + IMAGE_BYTES_PER_LINE*sample_y] = 255;
#endif      
    }      
  }
  
#ifdef IMAGE_DEBUG
  save_image(sample_image_debug,"debug-sample.pnm",0);
  free(sample_image_debug);
#endif
  
}


/*
 * Check if this quadrant has a valid checksum.  We assume that the code
 * is systematic and so there is no need to do an explicit decode.
 */
#ifdef USE_CRC
int check_checksum(int quadrant, const char sampledcode[EDGE_CELLS*EDGE_CELLS]) {
  /* if we set check = 1 computecrc will not change the data so just
     cast away the const to remove the compiler warning */ 
  char* nonconst = (char*)sampledcode; 
  return computecrc(nonconst+QUADRANT_SIZE*quadrant,QUADRANT_SIZE,1);
}
#else
int check_checksum(int quadrant, const char sampledcode[EDGE_CELLS*EDGE_CELLS]) {
  /*  printf("Checking %d\n",quadrant); */
  int accumulator = 0;
  int i;
  for(i=0;i<QUADRANT_SIZE-CHECKSUM_BITS;++i) {
    accumulator += sampledcode[QUADRANT_SIZE*quadrant + i] & 1;
  }
  accumulator &= (1<<CHECKSUM_BITS) - 1;
  for(i=0;i<CHECKSUM_BITS;++i) {
    if ((accumulator & 1) ^ (sampledcode[QUADRANT_SIZE*(quadrant+1) - 1 - i] & 1)) return 0;
    accumulator>>=1;
  }
  return 1;
}
#endif

/*
 * Check all the quadrants and then pull out the data and pack it into
 * the result array. Return false if the checksum fails 
 */
int decode(const char sampledcode[EDGE_CELLS*EDGE_CELLS], char data[PAYLOAD_SIZE_BYTES]) {
  int i;
  int j;
  int start_quad = -1;
  int data_pointer = 0;
  int byte_counter = 0;
  char current = 0;
  int current_quadrant;
  for(i=0;i<4;++i) {
    if (!check_checksum(i,sampledcode)) return 0;
    if (sampledcode[QUADRANT_SIZE*i]) start_quad = i;
  }
  
  if (start_quad == -1) return 0;
  
  for(i=0;i<4;++i) {
    current_quadrant = (i + start_quad) & 3;
    for(j=1;j<QUADRANT_SIZE-CHECKSUM_BITS;++j) {
      current <<= 1;
      current |= sampledcode[ current_quadrant * QUADRANT_SIZE + j ] & 1;
      ++byte_counter;
      if (byte_counter == 8) {
	data[data_pointer++] = current;
	current = 0;
	byte_counter = 0;
      }
    }
  }
  return 1;
}


int process_image(unsigned char* data, unsigned char code[PAYLOAD_SIZE_BYTES], int* read_order) {

  int i;
  int rasterx = 0;
  int rastery = 0;
  int bounding_minx;
  int bounding_miny;
  int bounding_maxx;
  int bounding_maxy;
  struct Point quad[4];
  char samplebuffer[EDGE_CELLS*EDGE_CELLS] = {0};

  /* This could be written in as a constant if floats are not supported at all! */
  COS_CURVATURE_THRESHOLD_FP = (long long) (0.64*((float)(1<<FRAC_BITS)));
#ifdef TEXT_DEBUG
  printf("COS_CURVATURE_THRESHOLD = %lld\n",COS_CURVATURE_THRESHOLD_FP);
#endif

  /* COS_CURVATURE_THRESHOLD_FP = 41943LL; */

#ifdef TEXT_DEBUG
  printf("Thresholding... ");
#endif
  adaptive_threshold(data);
#ifdef TEXT_DEBUG
  printf("[DONE]\n");
#endif

#ifdef TEXT_DEBUG
  printf("Contours... ");
#endif
  for(i=0;i<IMAGE_WIDTH;++i) {
    data[i] =0;
    data[(IMAGE_HEIGHT-1)*IMAGE_BYTES_PER_LINE + i] = 0;
  }
  for(i=0;i<IMAGE_HEIGHT;++i) {
    data[IMAGE_BYTES_PER_LINE*i]=0;
    data[IMAGE_BYTES_PER_LINE*i+IMAGE_WIDTH-1] = 0;
  }
  
  while(rasterx<IMAGE_WIDTH && rastery<IMAGE_HEIGHT) {
    if (findquad(data,&rasterx,&rastery,quad)) {
#ifdef TEXT_DEBUG
      printf("Found rectangle:\n");
#endif      

      bounding_minx = IMAGE_WIDTH+1;
      bounding_miny = IMAGE_HEIGHT+1;
      bounding_maxx = 0;
      bounding_maxy = 0;
      
      for(i=0;i<4;i++) {
	bounding_minx = min(bounding_minx, quad[i].x);
	bounding_maxx = max(bounding_maxx, quad[i].x);
	bounding_miny = min(bounding_miny, quad[i].y);
	bounding_maxy = max(bounding_maxy, quad[i].y);
#ifdef TEXT_DEBUG
	printf("   x=%d, y=%d\n",quad[i].x,quad[i].y);
#endif
      }

      int bounding_box_area = (bounding_maxx - bounding_minx)*(bounding_maxy - bounding_miny);
#ifdef TEXT_DEBUG
      printf("Bounding box area = %d\n", bounding_box_area);
#endif
      
      tracked_tag = 0;

      /* if bounding_box is too small to be a tag then go onto the next one...*/
      if (bounding_box_area < MIN_TAG_BOUNDING_BOX_AREA) continue;

      /* if bounding box similar to tracked bbox then update tracked bbox */
      int diffs = abs(bbox_top_left.x - bounding_minx) + abs(bbox_top_left.y - bounding_miny)
	+ abs(bbox_bottom_right.x - bounding_maxx) + abs(bbox_bottom_right.y - bounding_maxy);

      int diff_threshold = 8 * 4; /* 8 pixels out for all 4 corners... */
      if (diffs < diff_threshold) {
	bbox_top_left.x = bounding_minx;
	bbox_top_left.y = bounding_miny;
	bbox_bottom_right.x = bounding_maxx;
	bbox_bottom_right.y = bounding_maxy;
	tracked_tag = 1;
      }

#ifdef TEXT_DEBUG      
      printf("Sampling... ");
#endif
      sample_code(data,samplebuffer,quad,read_order);
#ifdef TEXT_DEBUG
      printf("[DONE]\n");
#endif

#ifdef TEXT_DEBUG
      printf("Decoding... "); 
#endif
      i = decode(samplebuffer,code);
#ifdef TEXT_DEBUG
      printf("[DONE]\n");
#endif      

      if (i) {
	/* update tracked bounding box with actual tag sighting */

	bbox_top_left.x = bounding_minx;
	bbox_top_left.y = bounding_miny;
	bbox_bottom_right.x = bounding_maxx;
	bbox_bottom_right.y = bounding_maxy;
	tracked_tag = 1;

#ifdef DISPLAY_CODE
	printf("Data: ");
	for(i=0;i<PAYLOAD_SIZE_BYTES;++i) {
	  printf("%.2X",code[i]& 0xFF);
	}
	printf("\n");
#endif
	return 1;
      }
      else {

#ifdef TEXT_DEBUG
	printf("Failed to decode\n");
#endif
      }
    }
  }
  return 0;
}


