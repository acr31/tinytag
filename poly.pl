#!/usr/bin/perl

#
# Copyright 2005 Andrew Rice
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of the
# License, or (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#  
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA.
#
# Email: acr31@cam.ac.uk
#

# tabulates CRC polynomials. See Philip Koopman and Tridib
# Chakravarty, "Cyclic Redundancy Code (CRC) Polynomial Selection Form
# Embedded Networks", in The International Conference on Dependable
# Systems and Networks 2004.

use strict;

my @poly = ([3 => [ [2,2048,0x5] ]],

	    [4 => [ [2,2048,0x9],
		    [3,11,0x9] ]],

	    [5 => [ [2,2048,0x12],
		    [3,26,0x12],
		    [4,10,0x15] ]],

	    [6 => [ [2,2048,0x21],
		    [3,57,0x12],
		    [4,25,0x2C] ]],

	    [7 => [ [2,2048,0x48],
		    [3,120,0x48],
		    [4,56,0x5B] ]],

	    [8 => [ [2,2048,0xA6],
		    [3,247,0xA6],
		    [4,119,0x97],
		    [5,9,0x9C] ]],
	     
	    [9 => [ [2,2048,0x167],
		    [3,502,0x167],
		    [4,246,0x14B],
		    [5,13,0x185],
		    [6,8,0x13C] ]],

	    [10=> [ [2,2048,0x327],
		    [3,1013,0x327],
		    [4,501,0x319],
		    [5,21,0x2B9],
		    [6,12,0x28E] ]],

	    [11=> [ [2,2048,0x64D],
		    [3,2036,0x64D],
		    [4,1012,0x583],
		    [5,25,0x5D7],
		    [6,22,0x532],
		    [7,12,0x571] ]],
	    
	    [12=> [ [3,2048,0xB75],
		    [4,2035,0xC07],
		    [5,53,0x8F8],
		    [6,27,0xB41],
		    [8,11,0xA4F] ]],
	    
	    [13=> [ [4,2048,0x102A],
		    [6,52,0x1909],
		    [7,12,0x12A5],
		    [8,11,0x10B7] ]],
	    
	    [14=> [ [4,2048,0x21E8],
		    [5,113,0x212D],
		    [6,57,0x372B],
		    [7,13,0x28A9],
		    [8,11,0x2371] ]],

	    [15=> [ [4,2048,0x4976],
		    [5,136,0x6A8D],
		    [6,114,0x573A],
		    [7,16,0x5BD5],
		    [8,12,0x630B] ]],
	     
	    [16=> [ [4,2048,0xBAAD],
		    [5,241,0xAC9A],
		    [6,135,0xC86C],
		    [7,19,0x968B],
		    [8,15,0x8FDB] ]] );

foreach my $pair (@poly) {
    my $crc_size = $pair->[0];
    my $options = $pair->[1];
    print "#if (CHECKSUM_BITS == $crc_size)\n"; 
    my $first = 1;
    foreach my $option (sort { $a->[1] <=> $b->[1] } @$options) {
	my $hamming_distance = $option->[0];
	my $max_length = $option->[1];
	my $poly = $option->[2];
	if ($first == 1) {
	    print "# if (PROTECT_SIZE < $max_length)\n";
	    $first = 0;
	}
	else {
	    print "# elif (PROTECT_SIZE < $max_length)\n";	    
	}
	print "#  define POLYNOMIAL $poly\n";
	print "#  define HAMMING_DISTANCE $hamming_distance\n";
    }
    print "# endif\n";
    print "#endif\n";
}

		   
