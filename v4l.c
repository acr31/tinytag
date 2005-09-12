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

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>  // for open()
#include <sys/stat.h>   // for open()
#include <fcntl.h>      // for open()
#include <sys/ioctl.h>  // for ioctl()
#include <unistd.h>     // for mmap()
#include <sys/mman.h>   // for mmap()
#include <errno.h>
#include <string.h> // for memcpy()
#include "v4l.h"
#include "options.h"

struct v4l_handle* v4l_open(const char* videodev) {
  struct v4l_handle* handle = (struct v4l_handle*)malloc(sizeof(struct v4l_handle));
  struct video_capability capability;
  struct video_picture p;
  int i;

  handle->filehandle = -1;
  handle->mmap_start = 0;
  handle->slots = 0;
  handle->working_image = 0;
  handle->filehandle = open(videodev,O_RDWR);
  if (handle->filehandle < 0) {
    v4l_close(handle);
    return 0;
  }

  if (ioctl(handle->filehandle,VIDIOCGCAP,&capability) < 0) {
    v4l_close(handle);
    return 0;
  }
  
  if ((capability.type & VID_TYPE_CAPTURE) == 0) {
    v4l_close(handle);
    return 0;
  }
 
  if (ioctl(handle->filehandle,VIDIOCGMBUF,&handle->mbuf) < 0) {
    v4l_close(handle);
    return 0;
  }
  
  void* mmapvoid = mmap(0,handle->mbuf.size,PROT_READ|PROT_WRITE,MAP_SHARED,handle->filehandle,0);
  if (mmapvoid == MAP_FAILED) {
    v4l_close(handle);
    return 0;
  }

  handle->mmap_start = (unsigned char*)mmapvoid;
  handle->image_width = capability.maxwidth;
  handle->image_height = capability.maxheight;
  printf("Image size (%d,%d)\n",handle->image_width,handle->image_height);
  
  if (handle->image_width != IMAGE_WIDTH ||
      handle->image_height != IMAGE_HEIGHT) {
    printf("ERROR: Camera image size does not match precompiled image size");
    v4l_close(handle);
    return 0;
  }
  handle->working_image = (unsigned char*)malloc(IMAGE_BYTES_PER_LINE*IMAGE_HEIGHT);

  if (ioctl(handle->filehandle,VIDIOCGPICT,&p) < 0) {
    v4l_close(handle);
    return 0;
  }

  p.palette = VIDEO_PALETTE_YUV420P;
  p.depth = 8;
  if (ioctl(handle->filehandle,VIDIOCSPICT,&p) < 0) {
    v4l_close(handle);
    return 0;
  } 
  
  printf("Fetching into %d slots\n",handle->mbuf.frames);
  handle->slots = (struct video_mmap*)malloc(sizeof(struct video_mmap) * handle->mbuf.frames);
  for(i=0;i<handle->mbuf.frames;++i) {
    handle->slots[i].format = p.palette;
    handle->slots[i].frame = i;
    handle->slots[i].width = handle->image_width;
    handle->slots[i].height = handle->image_height;
    // start the device asynchronously fetching the frame
    if (i>0) { 
      // dont start capturing for the first one because we'll start it
      // when we first call next
      if (ioctl(handle->filehandle,VIDIOCMCAPTURE,&(handle->slots[i])) < 0) {
	v4l_close(handle);
	return 0;
      }
    }
  }
  handle->current_frame = 0;

  return handle;
}

void v4l_close(struct v4l_handle* handle) {
  if (handle->filehandle != -1) close(handle->filehandle);
  if (handle->mmap_start != 0) munmap(handle->mmap_start,handle->mbuf.size);
  if (handle->slots != 0) free(handle->slots);
  if (handle->working_image != 0) free(handle->working_image);
  free(handle);
}

unsigned char* v4l_next(struct v4l_handle* handle) {
  int ioctl_result;
  int ioctl_retry = 0;

  while( (ioctl_result = ioctl(handle->filehandle,VIDIOCMCAPTURE,&(handle->slots[handle->current_frame]))) == EBUSY && ioctl_retry++<=5);
  if (ioctl_result < 0) {
    return 0;
  }

  ++(handle->current_frame);
  handle->current_frame%=handle->mbuf.frames;
 
  // collect the next image - block until it's there
  while( (ioctl_result = ioctl(handle->filehandle,VIDIOCSYNC,&(handle->slots[handle->current_frame].frame))) == EINTR);
  if (ioctl_result < 0) {
    return 0;
  }  
  
  memcpy(handle->working_image,handle->mmap_start + handle->mbuf.offsets[handle->current_frame],IMAGE_BYTES_PER_LINE*IMAGE_HEIGHT);
  
  return handle->working_image;
}
