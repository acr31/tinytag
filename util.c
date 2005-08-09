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
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>


// A little helpfer function for saving raw images:

void save_file(char* prefix,int i,char* frame,int framesize) {
  char outfilename[256];
  snprintf(outfilename,256,"%s-%d",prefix,i);
  // printf("Trying to write: %s\n", outfilename);
  
  int out_handle = open(outfilename,O_RDWR | O_CREAT);
  // printf("out_handle = %d\n",out_handle);
  write(out_handle, frame, framesize);
  close(out_handle);
}
