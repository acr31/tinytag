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

#include <libv4l1-videodev.h> /* for Video4Linux*/

struct v4l_handle {
  int filehandle;
  unsigned char* mmap_start;
  int image_width;
  int image_height;
  int current_frame;
  struct video_mmap* slots;
  struct video_mbuf mbuf;
  unsigned char* working_image;
};

struct v4l_handle* v4l_open(const char* videodev);
unsigned char* v4l_next(struct v4l_handle* handle);
void v4l_close(struct v4l_handle* handle);
