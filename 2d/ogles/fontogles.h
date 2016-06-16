//
//  FONTOGLES.h
//  2D
//
//  Created by Devin Tuchsen on 11/22/15.
//

#ifndef FONTOGLES_h
#define FONTOGLES_h

#ifdef OGLES
int gr_scale_string_ogles(int x, int y, fix scale_x, fix scale_y, unsigned char * s);
#endif

#endif /* FONTOGLES_h */
