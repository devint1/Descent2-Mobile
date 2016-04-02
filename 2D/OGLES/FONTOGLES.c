//
//  FONTOGLES.c
//  2D
//
//  Created by Devin Tuchsen on 11/22/15.
//

#ifdef OGLES
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gr.h"
#include "fontogles.h"
#include "types.h"
#include "oglestex.h"

#define INFONT(_c) ((_c >= 0) && (_c <= grd_curcanv->cv_font->ft_maxchar-grd_curcanv->cv_font->ft_minchar))

// Used in hack to guarantee whiteness in font texture
extern ubyte gr_current_pal[256*3];

extern ubyte *gr_bitblt_fade_table;
extern int get_centered_x_scaled(char *s);
extern void get_char_width(unsigned int c,unsigned int c2,int *width,int *spacing);
extern int gr_ustring_unscaled(int x, int y, char *s);

// Draw scaled string as OpenGL ES textures
int gr_scale_string_ogles(int x, int y, fix scale_x, fix scale_y, unsigned char * s) {
	int retval;
	int base_x = x;
	int width, spacing;
	unsigned char temp_str[2];
	grs_canvas *temp_canv, *save_canv = grd_curcanv;
	grs_bitmap temp_bm;
	ubyte pal_save[3];
	ubyte *fade_table_save = NULL;
	
	// Set up temp bitmap
	if (grd_curcanv->cv_font->ft_flags & FT_COLOR) {
		temp_bm.avg_color = 0;
	} else {
		temp_bm.avg_color = 255;
	}
	temp_bm.bm_h = grd_curcanv->cv_font->ft_h;
	temp_bm.bm_w = grd_curcanv->cv_font->ft_w;
	
	// Temp string is null terminated
	temp_str[1] = 0;
	
	// Centered
	if (x==0x8000) {
		x = get_centered_x_scaled(s);
	}
	
	while (*s) {
		if (*s == '\n') {
			y += grd_curcanv->cv_font->ft_h * f2fl(scale_y);
			++s;
			
			// Either center this line or reset X
			if (base_x==0x8000) {
				x = get_centered_x_scaled(s);
			} else {
				x = base_x;
			}

			continue;
		} else if (*s == CC_COLOR) {
			gr_set_fontcolor(*++s, -1);
			++s;
		} else if (*s == CC_LSPACING) {
			printf("WARNING! Line spacing not yet supported for OGLES strings.\n");
			s += 2;
		} else if (*s == CC_UNDERLINE) {
			printf("WARNING! Underline not yet supported for OGLES strings.\n");
			s += 2;
		}
		get_char_width(s[0], s[1], &width, &spacing);
		if (!grd_curcanv->cv_font->ft_ogles_texes[*s - grd_curcanv->cv_font->ft_minchar] && INFONT(*s - grd_curcanv->cv_font->ft_minchar)) {
			
			// Render the character to a temp canvas
			temp_canv = gr_create_canvas(width, grd_curcanv->cv_font->ft_h);
			gr_set_current_canvas(temp_canv);
			gr_clear_canvas(TRANSPARENCY_COLOR);
			gr_set_curfont(save_canv->cv_font);
			gr_set_fontcolor(255, -1);
			temp_str[0] = *s;
			fade_table_save = gr_bitblt_fade_table;
			gr_bitblt_fade_table = NULL;
			retval = gr_ustring_unscaled(0, 0, temp_str);
			gr_bitblt_fade_table = fade_table_save;
			gr_set_current_canvas(save_canv);
			
			// Save off last 3 bytes of the palette and set it to white
			memcpy(pal_save, &gr_current_pal[765], sizeof(ubyte) * 3);
			memset(&gr_current_pal[765], 63, 3);
			
			ogles_bm_bind_teximage_2d(&temp_canv->cv_bitmap);
			
			// Restore last 3 bytes of palette
			memcpy(&gr_current_pal[765], pal_save, sizeof(ubyte) * 3);
			
			grd_curcanv->cv_font->ft_ogles_texes[*s - grd_curcanv->cv_font->ft_minchar] = temp_canv->cv_bitmap.bm_ogles_tex_id;
			free(temp_canv->cv_bitmap.bm_data);
			free(temp_canv);
		}
		
		// Render the character to the screen
		if (INFONT(*s - grd_curcanv->cv_font->ft_minchar)) {
			temp_bm.bm_ogles_tex_id = grd_curcanv->cv_font->ft_ogles_texes[*s - grd_curcanv->cv_font->ft_minchar];
			grs_point scale_pts[] = {
				{ i2f(x), i2f(y) },
				{ i2f(width) + i2f(x), i2f(grd_curcanv->cv_font->ft_h) + i2f(y) },
				{ width * scale_x + i2f(x), grd_curcanv->cv_font->ft_h * scale_y + i2f(y) }
			};
			scale_bitmap(&temp_bm, scale_pts, 0);
		}
		
		x += spacing * f2fl(scale_x);
		++s;
	}

	return retval;
}
#endif
