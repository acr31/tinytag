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

#define EDGE_CELLS 16


#define USE_CRC
#define CHECKSUM_BITS 3

#define MIN_TAG_BOUNDING_BOX_AREA (80*80)

#define LOG_ADAPTIVE_THRESHOLD_WINDOW 8
#define ADAPTIVE_THRESHOLD_OFFSET 30

#define GLOBAL_THRESHOLD 42

/* Fixed point stuff */
#define FRAC_BITS 16

#define LOG_CURVATURE_WINDOW 6


#define IMAGE_WIDTH 640
#define IMAGE_HEIGHT 480
#define IMAGE_BYTES_PER_LINE 640


#define PAYLOAD_SIZE (EDGE_CELLS*EDGE_CELLS)
#define PAYLOAD_SIZE_BYTES (PAYLOAD_SIZE>>3)
#define QUADRANT_SIZE ((EDGE_CELLS*EDGE_CELLS)>>2)

/*
#define IMAGE_DEBUG
#define TEXT_DEBUG
*/
#define DISPLAY_CODE
