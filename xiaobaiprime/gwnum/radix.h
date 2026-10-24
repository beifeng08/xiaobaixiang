/*----------------------------------------------------------------------
| radix.h
|
| This file contains the C routine definitions for radix conversion
| when required by gianttogw or gwtogiant.
| 
|  Copyright 2020 Mersenne Research, Inc.  All rights reserved.
+---------------------------------------------------------------------*/

int nonbase2_gianttogw (	
	gwhandle *gwdata,	
	giant	g,		
	gwnum	x);		

int nonbase2_gwtogiant (	
	gwhandle *gwdata,	
	gwnum	x,		
	giant	g);		

