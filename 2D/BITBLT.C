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
 COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
 */
#include "gr.h"
#include "grdef.h"
#include "error.h"
#include "byteswap.h"
#include "rle.h"
#ifdef OGLES
#include "bitbltogles.h"
#include <stdlib.h>
#include <string.h>
#endif

int gr_bitblt_dest_step_shift = 0;
int gr_bitblt_double = 0;
ubyte *gr_bitblt_fade_table = NULL;

#define THRESHOLD	8

#if !( defined(__MWERKS__) && defined(__MC68K__) && defined(USE_2D_ASM) )
void gr_linear_movsd(ubyte * src, ubyte * dest, uint num_pixels) {
#ifdef __powerc
	int i;
	uint n, r;
	double *d, *s;
	ubyte *d1, *s1;
	
	// check to see if we are starting on an even byte boundry
	// if not, move appropriate number of bytes to even
	// 8 byte boundry
	
	if (num_pixels < THRESHOLD) {
		for (i = 0; i < num_pixels; i++)
			*dest++ = *src++;
		return;
	}
	
	i = 0;
	if ((r = (int)src & 0x7)) {
		for (i = 0; i < 8 - r; i++)
			*dest++ = *src++;
	}
	num_pixels -= i;
	
	n = num_pixels / 8;
	r = num_pixels % 8;
#if 0
	// ALEX DID THIS
	r = num_pixels - (n * 8);
#endif
	s = (double *)src;
	d = (double *)dest;
	for (i = 0; i < n; i++)
		*d++ = *s++;
	s1 = (ubyte *)s;
	d1 = (ubyte *)d;
	for (i = 0; i < r; i++)
		*d1++ = *s1++;
#else
	int i;
	for (i = 0; i < num_pixels; i++)
		*dest++ = *src++;
#endif
}
#endif	//!( defined(__MWERKS__) && defined(__MC68K__) && defined(USE_2D_ASM) )

void gr_linear_movsd_double(ubyte *src, ubyte *dest, int num_pixels) {
#ifdef __powerc
	double	*d = (double *)dest;
	uint	*s = (uint *)src;
	uint	doubletemp[2];
	uint	temp, work;
	int		i;
	
	for (i = 0; i < num_pixels / 4; i++) {
		temp = work = *s++;
		
		temp = ((temp >> 8) & 0x00FFFF00) | (temp & 0xFF0000FF); // 0xABCDEFGH -> 0xABABCDEF
		temp = ((temp >> 8) & 0x000000FF) | (temp & 0xFFFFFF00); // 0xABABCDEF -> 0xABABCDCD
		doubletemp[0] = temp;
		
		work = ((work << 8) & 0x00FFFF00) | (work & 0xFF0000FF); // 0xABCDEFGH -> 0xABEFGHGH
		work = ((work << 8) & 0xFF000000) | (work & 0x00FFFFFF); // 0xABEFGHGH -> 0xEFEFGHGH
		doubletemp[1] = work;
		
		*d = *(double *)&(doubletemp[0]);
		d++;
	}
	
#else
	int i;
	for (i = 0; i<num_pixels; i++) {
		*dest++ = *src;
		*dest++ = *src++;
	}
#endif
}

void gr_linear_rep_movsdm(ubyte * src, ubyte * dest, int num_pixels) {
	int i;
	for (i=0; i<num_pixels; i++ )	{
		if (*src != TRANSPARENCY_COLOR )
			*dest = *src;
		dest++;
		src++;
	}
}

void gr_linear_rep_movsdm_faded(ubyte * src, ubyte * dest, uint num_pixels, ubyte fade_value) {
	int i;
	ubyte source;
	ubyte *fade_base;
	
	fade_base = gr_fade_table + (fade_value * 256);
	
	for (i=num_pixels; i != 0; i-- )
	{
		source = *src;
		if (source != (ubyte)TRANSPARENCY_COLOR )
			*dest = *(fade_base + source);
		dest++;
		src++;
	}
}

void gr_bm_ubitblt_rle(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest) {
	unsigned char * dbits;
	unsigned char * sbits;
	short dest_rowsize = dest->bm_rowsize;
	int i;
	
#ifdef OGLES
	if(dest->bm_type == BM_OGLES) {
		dbits = malloc(w * h);
		memset(dbits, 255, w*h);
		dest_rowsize = w;
	}
#endif
	
	sbits = &src->bm_data[4 + src->bm_h];
	for (i = 0; i<sy; i++)
		sbits += (int)src->bm_data[4 + i];
	
	dbits = dest->bm_data + (dest->bm_rowsize * dy) + dx;
	
	// No interlacing, copy the whole buffer.
	for (i = 0; i < h; i++) {
		gr_rle_expand_scanline(dbits, sbits, sx, sx + w - 1);
		sbits += (int)src->bm_data[4 + i + sy];
		dbits += dest_rowsize << gr_bitblt_dest_step_shift;
	}
	
#ifdef OGLES
	if (dest->bm_type == BM_OGLES) {
		grs_bitmap src_ogles = *src;
		dbits -= w * h;
		src_ogles.bm_data = dbits;
		src_ogles.bm_w = w;
		src_ogles.bm_h = h;
		gr_ubitmapm_ogles(dx, dy, &src_ogles);
		glDeleteTextures(1, &src_ogles.bm_ogles_tex_id);
		free(dbits);
		return;
	}
#endif
}

void gr_bitblt_cockpit(grs_bitmap *bm) {
	unsigned char *dbits;
	unsigned char *sbits;
	int i, h, w;
	ushort s;
	
	w = bm->bm_w;
	h = bm->bm_h;
	sbits = &bm->bm_data[4 + (2 * h)];			// integer size at beginning of bitmap
	dbits = grd_curcanv->cv_bitmap.bm_data;
	for (i = 0; i < h; i++) {
		gr_rle_expand_scanline(dbits, sbits, 0, w - 1);
		s = swapshort(*((short *)&(bm->bm_data[4 + (i * 2)])));
		sbits += (int)s;
		dbits += (int)grd_curcanv->cv_bitmap.bm_rowsize;
	}
}

void gr_bm_ubitbltm_rle(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest) {
	unsigned char * dbits;
	unsigned char * sbits;
	
	int i;
	
	sbits = &src->bm_data[4 + src->bm_h];
	for (i = 0; i<sy; i++)
		sbits += (int)src->bm_data[4 + i];
	
	dbits = dest->bm_data + (dest->bm_rowsize * dy) + dx;
	
	// No interlacing, copy the whole buffer.
	for (i = 0; i < h; i++) {
		gr_rle_expand_scanline_masked(dbits, sbits, sx, sx + w - 1);
		sbits += (int)src->bm_data[4 + i + sy];
		dbits += dest->bm_rowsize << gr_bitblt_dest_step_shift;
	}
}

void gr_ubitmap(int x, int y, grs_bitmap *bm) {
	register int y1;
	int dest_rowsize;
	
	unsigned char * dest;
	unsigned char * src;
	
	if (bm->bm_flags & BM_FLAG_RLE) {
		gr_bm_ubitblt_rle(bm->bm_w, bm->bm_h, x, y, 0, 0, bm, &grd_curcanv->cv_bitmap);
		return;
	}
	
	dest_rowsize = grd_curcanv->cv_bitmap.bm_rowsize << gr_bitblt_dest_step_shift;
	dest = &(grd_curcanv->cv_bitmap.bm_data[dest_rowsize*y + x]);
	
	src = bm->bm_data;
	
	for (y1 = 0; y1 < bm->bm_h; y1++) {
		gr_linear_movsd(src, dest, bm->bm_w);
		src += bm->bm_rowsize;
		dest += (int)(dest_rowsize);
	}
}

void gr_bm_ubitblt00m_rle(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest)
{
	unsigned char * dbits;
	unsigned char * sbits;
	int i, data_offset;
	
	data_offset = 1;
	if (src->bm_flags & BM_FLAG_RLE_BIG)
		data_offset = 2;
	
	sbits = &src->bm_data[4 + (src->bm_h*data_offset)];
	for (i=0; i<sy; i++ )
		sbits += (int)(INTEL_SHORT(src->bm_data[4+(i*data_offset)]));
	
	dbits = dest->bm_data + (dest->bm_rowsize * dy) + dx;
	
	// No interlacing, copy the whole buffer.
	for (i=0; i < h; i++ )    {
		gr_rle_expand_scanline_masked( dbits, sbits, sx, sx+w-1 );
		if ( src->bm_flags & BM_FLAG_RLE_BIG )
			sbits += (int)INTEL_SHORT(*((short *)&(src->bm_data[4+((i+sy)*data_offset)])));
		else
			sbits += (int)(src->bm_data[4+i+sy]);
		dbits += dest->bm_rowsize << gr_bitblt_dest_step_shift;
	}
}

void gr_ubitmap00m( int x, int y, grs_bitmap *bm )
{
	register int y1;
	int dest_rowsize;
	
	unsigned char * dest;
	unsigned char * src;
	
	dest_rowsize=grd_curcanv->cv_bitmap.bm_rowsize << gr_bitblt_dest_step_shift;
	dest = &(grd_curcanv->cv_bitmap.bm_data[ dest_rowsize*y+x ]);
	
	src = bm->bm_data;
	
	if (gr_bitblt_fade_table==NULL)	{
		for (y1=0; y1 < bm->bm_h; y1++ )    {
			gr_linear_rep_movsdm( src, dest, bm->bm_w );
			src += bm->bm_rowsize;
			dest+= (int)(dest_rowsize);
		}
	} else {
		for (y1=0; y1 < bm->bm_h; y1++ )    {
			gr_linear_rep_movsdm_faded( src, dest, bm->bm_w, gr_bitblt_fade_table[y1+y] );
			src += bm->bm_rowsize;
			dest+= (int)(dest_rowsize);
		}
	}
}

void gr_bm_ubitblt02m_rle(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest)
{
	int i, data_offset;
	register int y1;
	unsigned char * sbits;
	
	//mprintf( 0, "SVGA RLE!\n" );
	
	data_offset = 1;
	if (src->bm_flags & BM_FLAG_RLE_BIG)
		data_offset = 2;
	
	sbits = &src->bm_data[4 + (src->bm_h*data_offset)];
	for (i=0; i<sy; i++ )
		sbits += (int)(INTEL_SHORT(src->bm_data[4+(i*data_offset)]));
	
	for (y1=0; y1 < h; y1++ )    {
		gr_rle_expand_scanline_svga_masked( dest, dx, dy+y1,  sbits, sx, sx+w-1  );
		if ( src->bm_flags & BM_FLAG_RLE_BIG )
			sbits += (int)INTEL_SHORT(*((short *)&(src->bm_data[4+((y1+sy)*data_offset)])));
		else
			sbits += (int)src->bm_data[4+y1+sy];
	}
}

void gr_bm_ubitblt02m(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest)
{
	unsigned char * sbits;
	
	unsigned int offset, EndingOffset, VideoLocation;
	
	int sbpr, dbpr, y1, page, BytesToMove;
	
	sbpr = src->bm_rowsize;
	
	dbpr = dest->bm_rowsize << gr_bitblt_dest_step_shift;
	
	VideoLocation = (unsigned int)dest->bm_data + (dest->bm_rowsize * dy) + dx;
	
	sbits = src->bm_data + ( sbpr*sy ) + sx;
	
	for (y1=0; y1 < h; y1++ )    {
		
		page    = VideoLocation >> 16;
		offset  = VideoLocation & 0xFFFF;
		
		gr_vesa_setpage( page );
		
		EndingOffset = offset+w-1;
		
		if ( EndingOffset <= 0xFFFF )
		{
			gr_linear_rep_movsdm( (void *)sbits, (void *)(offset+0xA0000), w );
			
			VideoLocation += dbpr;
			sbits += sbpr;
		}
		else
		{
			BytesToMove = 0xFFFF-offset+1;
			
			gr_linear_rep_movsdm( (void *)sbits, (void *)(offset+0xA0000), BytesToMove );
			
			page++;
			gr_vesa_setpage(page);
			
			gr_linear_rep_movsdm( (void *)(sbits+BytesToMove), (void *)0xA0000, EndingOffset - 0xFFFF );
			
			VideoLocation += dbpr;
			sbits += sbpr;
		}
	}
}

void modex_copy_column_m(ubyte * src, ubyte * dest, int num_pixels, int src_rowsize, int dest_rowsize )
{
	src = src;
	dest = dest;
	num_pixels = num_pixels;
	src_rowsize = src_rowsize;
	dest_rowsize = dest_rowsize;
	Int3();
}

// From Linear to ModeX masked
void gr_bm_ubitblt01m(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest)
{
	//ubyte * dbits1;
	//ubyte * sbits1;
	
	ubyte * dbits;
	ubyte * sbits;
	
	int x;
	//	int y;
	
	sbits =   src->bm_data  + (src->bm_rowsize * sy) + sx;
	dbits =   &gr_video_memory[(dest->bm_rowsize * dy) + dx/4];
	
	for (x=dx; x < dx+w; x++ )	{
		gr_modex_setplane( x&3 );
		
		//sbits1 = sbits;
		//dbits1 = dbits;
		//for (y=0; y < h; y++ )    {
		//	*dbits1 = *sbits1;
		//	sbits1 += src_bm_rowsize;
		//	dbits1 += dest_bm_rowsize;
		//	}
		modex_copy_column_m(sbits, dbits, h, src->bm_rowsize, dest->bm_rowsize << gr_bitblt_dest_step_shift );
		
		sbits++;
		if ( (x&3)==3 )
			dbits++;
	}
}

void gr_ubitmapGENERICm(int x, int y, grs_bitmap * bm)
{
	register int x1, y1;
	ubyte c;
	
	for (y1=0; y1 < bm->bm_h; y1++ )    {
		for (x1=0; x1 < bm->bm_w; x1++ )    {
			c = gr_gpixel(bm,x1,y1);
			if ( c != TRANSPARENCY_COLOR )	{
				gr_setcolor( c );
				gr_upixel( x+x1, y+y1 );
			}
		}
	}
}

void gr_ubitmap012m( int x, int y, grs_bitmap *bm )
{
	register int x1, y1;
	unsigned char * src;
	
	src = bm->bm_data;
	
	for (y1=y; y1 < (y+bm->bm_h); y1++ )    {
		for (x1=x; x1 < (x+bm->bm_w); x1++ )    {
			if ( *src != TRANSPARENCY_COLOR )	{
				gr_setcolor( *src );
				gr_upixel( x1, y1 );
			}
			src++;
		}
	}
}

void gr_ubitmapm( int x, int y, grs_bitmap *bm )
{   int source, dest;
	
	source = bm->bm_type;
	dest = TYPE;

	Assert(x+bm->bm_w <= grd_curcanv->cv_w);
	Assert(y+bm->bm_h <= grd_curcanv->cv_h);

	if (source==BM_LINEAR) {
		switch( dest )
		{
			case BM_LINEAR:
				if ( bm->bm_flags & BM_FLAG_RLE )
					gr_bm_ubitblt00m_rle(bm->bm_w, bm->bm_h, x, y, 0, 0, bm, &grd_curcanv->cv_bitmap );
				else
					gr_ubitmap00m( x, y, bm );
				return;
			case BM_SVGA:
				if (bm->bm_flags & BM_FLAG_RLE)
					gr_bm_ubitblt02m_rle(bm->bm_w, bm->bm_h, x, y, 0, 0, bm, &grd_curcanv->cv_bitmap);
				//gr_bm_ubitblt0xm_rle(bm->bm_w, bm->bm_h, x, y, 0, 0, bm, &grd_curcanv->cv_bitmap);
				else
					gr_bm_ubitblt02m(bm->bm_w, bm->bm_h, x, y, 0, 0, bm, &grd_curcanv->cv_bitmap);
				//gr_ubitmapGENERICm(x, y, bm);
				return;
			case BM_MODEX:
				gr_bm_ubitblt01m(bm->bm_w, bm->bm_h, x+XOFFSET, y+YOFFSET, 0, 0, bm, &grd_curcanv->cv_bitmap);
				return;
#ifdef OGLES
            case BM_OGLES:
                gr_ubitmapm_ogles(x, y, bm);
                glDeleteTextures(1, &bm->bm_ogles_tex_id);
                bm->bm_ogles_tex_id = 0;
                return;
#endif
			default:
				gr_ubitmap012m( x, y, bm );
				return;
		}
	} else  {
		gr_ubitmapGENERICm(x, y, bm);
	}
}

extern void BlitLargeAlign(ubyte *draw_buffer, int dstRowBytes, ubyte *dstPtr, int w, int h, int modulus);

void gr_bm_ubitblt_double(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap *src, grs_bitmap *dest) {
	ubyte * dbits;
	ubyte * sbits;
	int dstep, i;
	
	sbits = src->bm_data + (src->bm_rowsize * sy) + sx;
	dbits = dest->bm_data + (dest->bm_rowsize * dy) + dx;
	dstep = dest->bm_rowsize << gr_bitblt_dest_step_shift;
#if defined(__powerc) || ( defined(__MWERKS__) && defined(__MC68K__) && defined(USE_2D_ASM) )
	BlitLargeAlign(sbits, dstep, dbits, src->bm_w, src->bm_h, src->bm_rowsize);
#else
	for (i = 0; i < h; i++) {
		gr_linear_movsd_double(sbits, dbits, w);
		dbits += dstep;
		gr_linear_movsd_double(sbits, dbits, w);
		dbits += dstep;
		sbits += src->bm_rowsize;
	}
#endif
}

// From Linear to Linear
void gr_bm_ubitblt(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest) {
#ifndef OGLES
	unsigned char * dbits;
	unsigned char * sbits;
	//int	src_bm_rowsize_2, dest_bm_rowsize_2;
	int dstep;
	
	int i;
	
	if (src->bm_flags & BM_FLAG_RLE) {
		gr_bm_ubitblt_rle(w, h, dx, dy, sx, sy, src, dest);
		return;
	}
	
	sbits = src->bm_data + (src->bm_rowsize * sy) + sx;
	dbits = dest->bm_data + (dest->bm_rowsize * dy) + dx;
	
	dstep = dest->bm_rowsize << gr_bitblt_dest_step_shift;
	
	// No interlacing, copy the whole buffer.
	for (i = 0; i < h; i++) {
		gr_linear_movsd(sbits, dbits, w);
		sbits += src->bm_rowsize;
		dbits += dstep;
	}
#endif
}

// From Linear to Linear Masked
void gr_bm_ubitbltm(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest) {
	unsigned char * dbits;
	unsigned char * sbits;
	int i;
	short dest_rowsize = dest->bm_rowsize;
	
	sbits = src->bm_data + (src->bm_rowsize * sy) + sx;
	dbits = dest->bm_data + (dest->bm_rowsize * dy) + dx;
	
#ifdef OGLES
	if(dest->bm_type == BM_OGLES) {
		dbits = malloc(w * h);
		memset(dbits, TRANSPARENCY_COLOR, w*h);
		dest_rowsize = w;
	}
#endif
	
	// No interlacing, copy the whole buffer.
	
	if (gr_bitblt_fade_table == NULL) {
		for (i = 0; i < h; i++) {
			gr_linear_rep_movsdm(sbits, dbits, w);
			sbits += src->bm_rowsize;
			dbits += dest_rowsize;
		}
	}
	else {
		for (i = 0; i < h; i++) {
			gr_linear_rep_movsdm_faded(sbits, dbits, w, gr_bitblt_fade_table[dy + i]);
			sbits += src->bm_rowsize;
			dbits += dest_rowsize;
		}
	}
	
#ifdef OGLES
	if (dest->bm_type == BM_OGLES) {
		grs_bitmap src_ogles = *src;
		dbits -= w * h;
		src_ogles.bm_data = dbits;
		src_ogles.bm_w = w;
		src_ogles.bm_h = h;
		gr_ubitmapm_ogles(dx, dy, &src_ogles);
		glDeleteTextures(1, &src_ogles.bm_ogles_tex_id);
		free(dbits);
		return;
	}
#endif
}

void gr_bm_bitblt(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest) {
#ifndef OGLES
	int dx1 = dx, dx2 = dx + dest->bm_w - 1;
	int dy1 = dy, dy2 = dy + dest->bm_h - 1;
	
	int sx1 = sx, sx2 = sx + src->bm_w - 1;
	int sy1 = sy, sy2 = sy + src->bm_h - 1;
	
	if ((dx1 >= dest->bm_w) || (dx2 < 0)) return;
	if ((dy1 >= dest->bm_h) || (dy2 < 0)) return;
	if (dx1 < 0) { sx1 += -dx1; dx1 = 0; }
	if (dy1 < 0) { sy1 += -dy1; dy1 = 0; }
	if (dx2 >= dest->bm_w) { dx2 = dest->bm_w - 1; }
	if (dy2 >= dest->bm_h) { dy2 = dest->bm_h - 1; }
	
	if ((sx1 >= src->bm_w) || (sx2 < 0)) return;
	if ((sy1 >= src->bm_h) || (sy2 < 0)) return;
	if (sx1 < 0) { dx1 += -sx1; sx1 = 0; }
	if (sy1 < 0) { dy1 += -sy1; sy1 = 0; }
	if (sx2 >= src->bm_w) { sx2 = src->bm_w - 1; }
	if (sy2 >= src->bm_h) { sy2 = src->bm_h - 1; }
	
	// Draw bitmap bm[x,y] into (dx1,dy1)-(dx2,dy2)
	if (dx2 - dx1 + 1 < w)
		w = dx2 - dx1 + 1;
	if (dy2 - dy1 + 1 < h)
		h = dy2 - dy1 + 1;
	if (sx2 - sx1 + 1 < w)
		w = sx2 - sx1 + 1;
	if (sy2 - sy1 + 1 < h)
		h = sy2 - sy1 + 1;
	
	gr_bm_ubitblt(w, h, dx1, dy1, sx1, sy1, src, dest);
#endif
}


// Clipped bitmap ...

void gr_bitmap(int x, int y, grs_bitmap *bm) {
#ifdef OGLES
	if (grd_curcanv->cv_bitmap.bm_type == BM_OGLES) {
		return;
	}
#endif
	int dx1 = x, dx2 = x + bm->bm_w - 1;
	int dy1 = y, dy2 = y + bm->bm_h - 1;
	int sx = 0, sy = 0;
	
	if ((dx1 >= grd_curcanv->cv_bitmap.bm_w) || (dx2 < 0)) return;
	if ((dy1 >= grd_curcanv->cv_bitmap.bm_h) || (dy2 < 0)) return;
	if (dx1 < 0) { sx = -dx1; dx1 = 0; }
	if (dy1 < 0) { sy = -dy1; dy1 = 0; }
	if (dx2 >= grd_curcanv->cv_bitmap.bm_w) { dx2 = grd_curcanv->cv_bitmap.bm_w - 1; }
	if (dy2 >= grd_curcanv->cv_bitmap.bm_h) { dy2 = grd_curcanv->cv_bitmap.bm_h - 1; }
	
	// Draw bitmap bm[x,y] into (dx1,dy1)-(dx2,dy2)
	
	gr_bm_ubitblt(dx2 - dx1 + 1, dy2 - dy1 + 1, dx1, dy1, sx, sy, bm, &grd_curcanv->cv_bitmap);
}

// From Linear to Linear Masked
void gr_bm_ubitblt00m(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest)
{
	unsigned char * dbits;
	unsigned char * sbits;
	//int	src_bm_rowsize_2, dest_bm_rowsize_2;
	
	int i;
	
	sbits =   src->bm_data  + (src->bm_rowsize * sy) + sx;
	dbits =   dest->bm_data + (dest->bm_rowsize * dy) + dx;
	
	// No interlacing, copy the whole buffer.
	
	if (gr_bitblt_fade_table==NULL)	{
		for (i=0; i < h; i++ )    {
			gr_linear_rep_movsdm( sbits, dbits, w );
			sbits += src->bm_rowsize;
			dbits += dest->bm_rowsize;
		}
	} else {
		for (i=0; i < h; i++ )    {
			gr_linear_rep_movsdm_faded( sbits, dbits, w, gr_bitblt_fade_table[dy+i] );
			sbits += src->bm_rowsize;
			dbits += dest->bm_rowsize;
		}
	}
}

void gr_bitmapm(int x, int y, grs_bitmap *bm) {
#ifdef OGLES
	if (grd_curcanv->cv_bitmap.bm_type == BM_OGLES) {
		gr_ubitmapm_ogles(x + grd_curcanv->cv_bitmap.bm_x, y + grd_curcanv->cv_bitmap.bm_y, bm);
		return;
	}
#endif
	int dx1=x, dx2=x+bm->bm_w-1;
	int dy1=y, dy2=y+bm->bm_h-1;
	int sx=0, sy=0;
	
	if ((dx1 >= grd_curcanv->cv_bitmap.bm_w ) || (dx2 < 0)) return;
	if ((dy1 >= grd_curcanv->cv_bitmap.bm_h) || (dy2 < 0)) return;
	if ( dx1 < 0 ) { sx = -dx1; dx1 = 0; }
	if ( dy1 < 0 ) { sy = -dy1; dy1 = 0; }
	if ( dx2 >= grd_curcanv->cv_bitmap.bm_w )	{ dx2 = grd_curcanv->cv_bitmap.bm_w-1; }
	if ( dy2 >= grd_curcanv->cv_bitmap.bm_h )	{ dy2 = grd_curcanv->cv_bitmap.bm_h-1; }
	
	// Draw bitmap bm[x,y] into (dx1,dy1)-(dx2,dy2)
	
	if ( (bm->bm_type == BM_LINEAR) && (grd_curcanv->cv_bitmap.bm_type == BM_LINEAR ))
	{
		if ( bm->bm_flags & BM_FLAG_RLE )
			gr_bm_ubitblt00m_rle(dx2-dx1+1,dy2-dy1+1, dx1, dy1, sx, sy, bm, &grd_curcanv->cv_bitmap );
		else
			gr_bm_ubitblt00m(dx2-dx1+1,dy2-dy1+1, dx1, dy1, sx, sy, bm, &grd_curcanv->cv_bitmap );
		return;
	}
	else if ( (bm->bm_type == BM_LINEAR) && (grd_curcanv->cv_bitmap.bm_type == BM_SVGA ))
	{
		gr_bm_ubitblt02m(dx2-dx1+1,dy2-dy1+1, dx1, dy1, sx, sy, bm, &grd_curcanv->cv_bitmap );
		return;
	}
	
	gr_bm_ubitbltm(dx2-dx1+1,dy2-dy1+1, dx1, dy1, sx, sy, bm, &grd_curcanv->cv_bitmap );
}
