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

#include <CoreGraphics/CoreGraphics.h>
#include <OpenGLES/ES1/glext.h>
#include "pa_enabl.h"                   //$$POLY_ACC
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "types.h"
#include "mem.h"
#include "gr.h"
#include "grdef.h"
#include "error.h"
#include "mono.h"
#include "fix.h"

CGContextRef bitmap_context;

#if defined(POLY_ACC)
#include "poly_acc.h"
#endif

#if !defined(WINDOWS) && !defined(MACINTOSH)
#include "dpmi.h"
#endif

#include "palette.h"

char gr_pal_default[768];

int gr_installed = 0;

int gr_show_screen_info = 0;

unsigned char * gr_video_memory = NULL;
fix Scale_factor = -1;
fix Scale_x, Scale_y;

//	Functions for GR.C

int gr_close_screen()
{
	if (grd_curscreen) {
		free(grd_curscreen);
		grd_curscreen = NULL;
	}
	
	return 0;
}

void gr_close()
{
	if (gr_installed==1)
	{
		gr_installed = 0;
		free(grd_curscreen);
	}
}

int gr_init(int w, int h)
{
	// Only do this function once!
	if (gr_installed==1)
		return 1;

	grd_curscreen=(grs_screen*)malloc(1*sizeof(grs_screen));
	memset( grd_curscreen, 0, sizeof(grs_screen));
	
	// Set all the screen, canvas, and bitmap variables that
	// aren't set by the gr_set_mode call:
	grd_curscreen->sc_canvas.cv_color = 0;
	grd_curscreen->sc_canvas.cv_drawmode = 0;
	grd_curscreen->sc_canvas.cv_font = NULL;
	grd_curscreen->sc_canvas.cv_font_fg_color = 0;
	grd_curscreen->sc_canvas.cv_font_bg_color = 0;
	grd_curscreen->sc_canvas.cv_bitmap.bm_data = (unsigned char *)gr_video_memory;
	grd_curscreen->sc_aspect = fl2f(1.0f);
	grd_curscreen->sc_canvas.cv_bitmap.bm_w = w;
	grd_curscreen->sc_canvas.cv_bitmap.bm_h = h;
	grd_curscreen->sc_canvas.cv_bitmap.bm_rowsize = w;
#ifdef OGLES
	grd_curscreen->sc_canvas.cv_bitmap.bm_type = BM_OGLES;
#else
	grd_curscreen->sc_canvas.cv_bitmap.bm_type = BM_LINEAR;
#endif
	grd_curscreen->sc_w = w;
	grd_curscreen->sc_h = h;
	gr_set_current_canvas( &grd_curscreen->sc_canvas );

	Scale_x = fixdiv(i2f(grd_curscreen->sc_w), i2f(640));
	Scale_y = fixdiv(i2f(grd_curscreen->sc_h), i2f(480));
	Scale_factor = fmin(Scale_x, Scale_y);
	
	// Set flags indicating that this is installed.
	gr_installed = 1;

	atexit(gr_close);

	return 0;
}

int gr_init_screen(int bitmap_type,int w,int h,int x, int y, int rowsize,ubyte *screen_addr)
{
	if (!gr_installed)
		return 1;

	gr_video_memory = screen_addr;

	if (grd_curscreen == NULL)
		MALLOC( grd_curscreen,grs_screen,1 );

	memset( grd_curscreen, 0, sizeof(grs_screen));

	grd_curscreen->sc_mode = bitmap_type;
	grd_curscreen->sc_w = w;
	grd_curscreen->sc_h = h;
	grd_curscreen->sc_aspect = fixdiv(grd_curscreen->sc_w*3,grd_curscreen->sc_h*4);

	grd_curscreen->sc_canvas.cv_bitmap.bm_x = x;
	grd_curscreen->sc_canvas.cv_bitmap.bm_y = y;
	grd_curscreen->sc_canvas.cv_bitmap.bm_w = w;
	grd_curscreen->sc_canvas.cv_bitmap.bm_h = h;
	grd_curscreen->sc_canvas.cv_bitmap.bm_rowsize = rowsize;
	grd_curscreen->sc_canvas.cv_bitmap.bm_type = bitmap_type;
#if defined(POLY_ACC)
    grd_curscreen->sc_canvas.cv_bitmap.bm_data = screen_addr;
#else
    grd_curscreen->sc_canvas.cv_bitmap.bm_data = (bitmap_type==BM_LINEAR)?screen_addr:NULL;
#endif

	grd_curscreen->sc_canvas.cv_color = 0;
	grd_curscreen->sc_canvas.cv_drawmode = 0;
	grd_curscreen->sc_canvas.cv_font = NULL;
	grd_curscreen->sc_canvas.cv_font_fg_color = 0;
	grd_curscreen->sc_canvas.cv_font_bg_color = 0;

	gr_set_current_canvas( &grd_curscreen->sc_canvas );

	return 0;
}

#ifdef OGLES
void ogles_draw_saved_screen(GLuint saved_screen_tex) {
	GLfloat vertices[] = { -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f };
	GLfloat texCoords[] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f };
	
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, saved_screen_tex);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glVertexPointer(2, GL_FLOAT, 0, vertices);
	glTexCoordPointer(2, GL_FLOAT, 0, texCoords);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}

GLuint ogles_save_screen() {
	GLuint tex;
	GLint viewWidth, viewHeight;
	
	glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES, GL_RENDERBUFFER_WIDTH_OES, &viewWidth);
	glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES, GL_RENDERBUFFER_HEIGHT_OES, &viewHeight);
	glEnable(GL_TEXTURE_2D);
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 0, 0, viewWidth, viewHeight, 0);
	return tex;
}

void ogles_map_bitmap(unsigned char *dest, GLubyte *src, GLuint width, GLuint height) {
	int i, j;
	GLubyte r, g, b, *srcRow;
	unsigned char *destRow;
	GLuint npixels;
	
	npixels = width * height;
	for (i = 0; i < height; ++i) {
		srcRow = &src[i * width * 4];
		destRow = &dest[npixels - i * width - width];
		
		for (j = 0; j < width; ++j) {
			r = srcRow[j * 4] / 4;
			g = srcRow[j * 4 + 1] / 4;
			b = srcRow[j * 4 + 2] / 4;
			destRow[j] = gr_find_closest_color(r, g, b);
		}
	}
}
#endif
