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

#include "options.h"

/**
 * Compute the crc of the message.  If the flag check is set then the
 * remainder stored at the end of this message will be compared to the
 * remainder of the crc.  If check is not set then the remainder of
 * the crc will be written into the trailing bits of this message.
 */
int computecrc(unsigned char message[], int nBytes, int check);
