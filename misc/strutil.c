/*
THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.  
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/
 
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "mem.h"
#include "error.h"

void strlwr( char *s1 )
{
	while( *s1 )	{
		*s1 = tolower(*s1);
		s1++;
	}
}

void strupr( char *s1 )
{
	while( *s1 )	{
		*s1 = toupper(*s1);
		s1++;
	}
}

void strrev( char *s1 )
{
	int i,l;
	char *s2;
	
	s2 = (char *)malloc(strlen(s1) + 1);
	strcpy(s2, s1);
	l = strlen(s2);
	for (i = 0; i < l; i++)
		s1[l-1-i] = s2[i];
	free(s2);
}

#if 0
void main()
{
	char drive[10], path[50], name[16], ext[5];
	
	drive[0] = path[0] = name[0] = ext[0] = '\0';
	_splitpath("f:\\tmp\\x.out", drive, path, name, ext);
	drive[0] = path[0] = name[0] = ext[0] = '\0';
	_splitpath("tmp\\x.out", drive, path, name, ext);
	drive[0] = path[0] = name[0] = ext[0] = '\0';
	_splitpath("f:\\tmp\\a.out", NULL, NULL, name, NULL);
	drive[0] = path[0] = name[0] = ext[0] = '\0';
	_splitpath("tmp\\*.dem", drive, path, NULL, NULL);
	drive[0] = path[0] = name[0] = ext[0] = '\0';
	_splitpath(".\\tmp\\*.dem", drive, path, NULL, NULL);
}
#endif
