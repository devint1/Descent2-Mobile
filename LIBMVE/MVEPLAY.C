#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#ifndef __MSDOS__
#define AUDIO
#endif
//#define DEBUG

#include <OpenAL/OpenAL.h>
#include <string.h>
#ifdef _WIN32
# include <windows.h>
#else
# include <errno.h>
# include <time.h>
# include <fcntl.h>
# ifdef macintosh
#  include <types.h>
#  include <OSUtils.h>
# else
#  include <sys/time.h>
#  include <sys/types.h>
#  include <sys/stat.h>
#  include <unistd.h>
# endif // macintosh
#endif // _WIN32

#include "mvelib.h"
#include "mveaudio.h"
#include "gr.h"

#include "decoders.h"

#include "libmve.h"

#define MVE_OPCODE_ENDOFSTREAM          0x00
#define MVE_OPCODE_ENDOFCHUNK           0x01
#define MVE_OPCODE_CREATETIMER          0x02
#define MVE_OPCODE_INITAUDIOBUFFERS     0x03
#define MVE_OPCODE_STARTSTOPAUDIO       0x04
#define MVE_OPCODE_INITVIDEOBUFFERS     0x05

#define MVE_OPCODE_DISPLAYVIDEO         0x07
#define MVE_OPCODE_AUDIOFRAMEDATA       0x08
#define MVE_OPCODE_AUDIOFRAMESILENCE    0x09
#define MVE_OPCODE_INITVIDEOMODE        0x0A

#define MVE_OPCODE_SETPALETTE           0x0C
#define MVE_OPCODE_SETPALETTECOMPRESSED 0x0D

#define MVE_OPCODE_SETDECODINGMAP       0x0F

#define MVE_OPCODE_VIDEODATA            0x11

#define MVE_AUDIO_FLAGS_STEREO     1
#define MVE_AUDIO_FLAGS_16BIT      2
#define MVE_AUDIO_FLAGS_COMPRESSED 4

int g_spdFactorNum=0;
static int g_spdFactorDenom=10;
static int g_frameUpdated = 0;
static ALuint source;

static short get_short(unsigned char *data)
{
	short value;
	value = data[0] | (data[1] << 8);
	return value;
}

static unsigned short get_ushort(unsigned char *data)
{
	unsigned short value;
	value = data[0] | (data[1] << 8);
	return value;
}

static int get_int(unsigned char *data)
{
	int value;
	value = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
	return value;
}

static unsigned int unhandled_chunks[32*256];

static int default_seg_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
{
	unhandled_chunks[major<<8|minor]++;
	//fprintf(stderr, "unknown chunk type %02x/%02x\n", major, minor);
	return 1;
}


/*************************
 * general handlers
 *************************/
static int end_movie_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
{
	return 0;
}

/*************************
 * timer handlers
 *************************/

/*
 * timer variables
 */
static int timer_created = 0;
static int micro_frame_delay=0;
static int timer_started=0;
static struct timeval timer_expire = {0, 0};

#if defined(_WIN32) || defined(macintosh)
int gettimeofday(struct timeval *tv, void *tz)
{
	static int counter = 0;
#ifdef _WIN32
	DWORD now = GetTickCount();
#else
	long now = TickCount();
#endif
	counter++;

	tv->tv_sec = now / 1000;
	tv->tv_usec = (now % 1000) * 1000 + counter;

	return 0;
}
#endif //  defined(_WIN32) || defined(macintosh)


static int create_timer_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
{

#if !defined(_WIN32) && !defined(macintosh) // FIXME
	__extension__ long long temp;
#else
	long temp;
#endif

	if (timer_created)
		return 1;
	else
		timer_created = 1;

	micro_frame_delay = get_int(data) * (int)get_short(data+4);
	if (g_spdFactorNum != 0)
	{
		temp = micro_frame_delay;
		temp *= g_spdFactorNum;
		temp /= g_spdFactorDenom;
		micro_frame_delay = (int)temp;
	}

	return 1;
}

static void timer_stop(void)
{
	timer_expire.tv_sec = 0;
	timer_expire.tv_usec = 0;
	timer_started = 0;
}

static void timer_start(void)
{
	int nsec=0;
	gettimeofday(&timer_expire, NULL);
	timer_expire.tv_usec += micro_frame_delay;
	if (timer_expire.tv_usec > 1000000)
	{
		nsec = timer_expire.tv_usec / 1000000;
		timer_expire.tv_sec += nsec;
		timer_expire.tv_usec -= nsec*1000000;
	}
	timer_started=1;
}

static void do_timer_wait(void)
{
	int nsec=0;
	struct timespec ts;
	struct timeval tv;
	if (! timer_started)
		return;

	gettimeofday(&tv, NULL);
	if (tv.tv_sec > timer_expire.tv_sec)
		goto end;
	else if (tv.tv_sec == timer_expire.tv_sec  &&  tv.tv_usec >= timer_expire.tv_usec)
		goto end;

	ts.tv_sec = timer_expire.tv_sec - tv.tv_sec;
	ts.tv_nsec = 1000 * (timer_expire.tv_usec - tv.tv_usec);
	if (ts.tv_nsec < 0)
	{
		ts.tv_nsec += 1000000000UL;
		--ts.tv_sec;
	}
#ifdef _WIN32
	Sleep(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
#elif defined(macintosh)
	Delay(ts.tv_sec * 1000 + ts.tv_nsec / 1000000, NULL);
#else
	if (nanosleep(&ts, NULL) == -1  &&  errno == EINTR)
		exit(1);
#endif

 end:
	timer_expire.tv_usec += micro_frame_delay;
	if (timer_expire.tv_usec > 1000000)
	{
		nsec = timer_expire.tv_usec / 1000000;
		timer_expire.tv_sec += nsec;
		timer_expire.tv_usec -= nsec*1000000;
	}
}

/*************************
 * audio handlers
 *************************/
#ifdef AUDIO
#define TOTAL_AUDIO_BUFFERS 64

static int audiobuf_created = 0;
static ALuint mve_audio_buffers[TOTAL_AUDIO_BUFFERS];
static int    mve_audio_curbuf_curpos=0;
static int mve_audio_bufhead=0;
static int mve_audio_buftail=0;
static int mve_audio_playing=0;
static int mve_audio_canplay=0;
static int mve_audio_compressed=0;
static int mve_audio_enabled = 1;
static int mve_audio_samplerate=0;
static ALenum mve_audio_format=0;

#endif

static int create_audiobuf_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
{
#ifdef AUDIO
	int flags;
	int desired_buffer;

	int stereo;
	int bitsize;
	int compressed;

	if (!mve_audio_enabled)
		return 1;

	if (audiobuf_created)
		return 1;
	else
		audiobuf_created = 1;

	flags = get_ushort(data + 2);
	mve_audio_samplerate = get_ushort(data + 4);
	desired_buffer = get_int(data + 6);

	stereo = (flags & MVE_AUDIO_FLAGS_STEREO) ? 1 : 0;
	bitsize = (flags & MVE_AUDIO_FLAGS_16BIT) ? 1 : 0;

	if (minor > 0) {
		compressed = flags & MVE_AUDIO_FLAGS_COMPRESSED ? 1 : 0;
	} else {
		compressed = 0;
	}

	mve_audio_compressed = compressed;

	if (bitsize == 1) {
		if (stereo) {
			mve_audio_format = AL_FORMAT_STEREO16;
		} else {
			mve_audio_format = AL_FORMAT_MONO16;
		}
	} else {
		if (stereo) {
			mve_audio_format = AL_FORMAT_STEREO8;
		} else {
			mve_audio_format = AL_FORMAT_MONO8;
		}
	}

	fprintf(stderr, "setting up audio:\n");
	fprintf(stderr, "sample rate = %d, stereo = %d, bitsize = %d, compressed = %d\n",
			mve_audio_samplerate, stereo, bitsize ? 16 : 8, compressed);

	// Assumes OpenAL already set up through digi_init_digi()
	alGenSources(1, &source);
	memset(mve_audio_buffers, 0, sizeof(ALint) * TOTAL_AUDIO_BUFFERS);

	mve_audio_canplay = 1;
#endif

	return 1;
}

static int play_audio_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
{
#ifdef AUDIO
	if (mve_audio_canplay  &&  !mve_audio_playing  &&  mve_audio_bufhead != mve_audio_buftail)
	{
		mve_audio_playing = 1;
	}
#endif
	return 1;
}

static int audio_data_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
{
#ifdef AUDIO
	static const int selected_chan=1;
	int chan;
	int nsamp;
	short *temp_data;
	ALint state, buffers_processed = 0;
	if (mve_audio_canplay)
	{
		chan = get_ushort(data + 2);
		nsamp = get_ushort(data + 4);
		if (chan & selected_chan)
		{
			/* HACK: +4 mveaudio_uncompress adds 4 more bytes */
			if (major == MVE_OPCODE_AUDIOFRAMEDATA) {
				if (mve_audio_compressed) {
					nsamp += 4;
					temp_data = mve_alloc(nsamp);
					mveaudio_uncompress(temp_data, data, -1); /* XXX */
				} else {
					nsamp -= 8;
					data += 8;
					temp_data = mve_alloc(nsamp);
					memcpy(temp_data, data, nsamp);
				}
			} else {
				temp_data = mve_alloc(nsamp);
				memset(temp_data, 0, nsamp); /* XXX */
			}
			
			if (mve_audio_buffers[mve_audio_buftail] == 0) {
				alGenBuffers(1, &mve_audio_buffers[mve_audio_buftail]);
			}
			alBufferData(mve_audio_buffers[mve_audio_buftail], mve_audio_format, temp_data, nsamp, mve_audio_samplerate);
			alSourceQueueBuffers(source, 1, &mve_audio_buffers[mve_audio_buftail]);
			alGetSourcei(source, AL_SOURCE_STATE, &state);
			if (state != AL_PLAYING) {
				alSourcePlay(source);
			}
			
			if (++mve_audio_buftail == TOTAL_AUDIO_BUFFERS)
				mve_audio_buftail = 0;

			if (mve_audio_buftail == mve_audio_bufhead)
				fprintf(stderr, "d'oh!  buffer ring overrun (%d)\n", mve_audio_bufhead);

			alGetSourcei(source, AL_BUFFERS_PROCESSED, &buffers_processed);
			mve_audio_buftail -= buffers_processed;
			alSourceUnqueueBuffers(source, buffers_processed, &mve_audio_buffers[mve_audio_buftail]);
			
			mve_free(temp_data);
		}
	}
#endif
	
	return 1;
}

/*************************
 * video handlers
 *************************/

static int videobuf_created = 0;
static int video_initialized = 0;
int g_width, g_height;
int g_scaledWidth, g_scaledHeight;
void *g_vBuffers = NULL, *g_vBackBuf1, *g_vBackBuf2;

static int g_destX, g_destY;
static int g_screenWidth, g_screenHeight;
static unsigned char *g_pCurMap=NULL;
static int g_nMapLength=0;
static int g_truecolor;

static int create_videobuf_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
{
	short w, h;
	short count, truecolor;

	if (videobuf_created)
		return 1;
	else
		videobuf_created = 1;

	w = get_short(data);
	h = get_short(data+2);

	if (minor > 0) {
		count = get_short(data+4);
	} else {
		count = 1;
	}

	if (minor > 1) {
		truecolor = get_short(data+6);
	} else {
		truecolor = 0;
	}

	g_width = w << 3;
	g_height = h << 3;
	
	g_scaledWidth = g_width * f2fl(Scale_factor);
	g_scaledHeight = g_height * f2fl(Scale_factor);
	
	/* TODO: * 4 causes crashes on some files */
	/* only malloc once */
	if (g_vBuffers == NULL)
		g_vBackBuf1 = g_vBuffers = mve_alloc(g_width * g_height * 8);
	if (truecolor) {
		g_vBackBuf2 = (unsigned short *)g_vBackBuf1 + (g_width * g_height);
	} else {
		g_vBackBuf2 = (unsigned char *)g_vBackBuf1 + (g_width * g_height);
	}

	memset(g_vBackBuf1, 0, g_width * g_height * 4);

#ifdef DEBUG
	fprintf(stderr, "DEBUG: w,h=%d,%d count=%d, tc=%d\n", w, h, count, truecolor);
#endif

	g_truecolor = truecolor;

	return 1;
}

static int display_video_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
{
	float scale_factor = (float)g_screenWidth / (float)g_width;
	
	if (g_destX == -1) // center it
	{
		g_scaledWidth = g_width * scale_factor;
		g_destX = (g_screenWidth - g_scaledWidth) >> 1;
	}
	else if (g_destY == -1) // center it
	{
		g_scaledHeight = g_height * scale_factor;
		g_destY = (g_screenHeight - g_scaledHeight) >> 1;
	}

	mve_showframe(g_vBackBuf1, g_width, g_height, 0, 0,
	              g_scaledWidth, g_scaledHeight, g_destX, g_destY);

	g_frameUpdated = 1;

	return 1;
}

static int init_video_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
{
	short width, height;

	if (video_initialized)
		return 1; /* maybe we actually need to change width/height here? */
	else
		video_initialized = 1;

	width = get_short(data);
	height = get_short(data+2);
	g_screenWidth = grd_curscreen->sc_w;
	g_screenHeight = grd_curscreen->sc_h;

	return 1;
}

static int video_palette_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
{
	short start, count;
	unsigned char *p;

	start = get_short(data);
	count = get_short(data+2);

	p = data + 4;

	mve_setpalette(p - 3*start, start, count);

	return 1;
}

static int video_codemap_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
{
	g_pCurMap = data;
	g_nMapLength = len;
	return 1;
}

static int video_data_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
{
	short nFrameHot, nFrameCold;
	short nXoffset, nYoffset;
	short nXsize, nYsize;
	unsigned short nFlags;
	unsigned char *temp;

	nFrameHot  = get_short(data);
	nFrameCold = get_short(data+2);
	nXoffset   = get_short(data+4);
	nYoffset   = get_short(data+6);
	nXsize     = get_short(data+8);
	nYsize     = get_short(data+10);
	nFlags     = get_ushort(data+12);

	if (nFlags & 1)
	{
		temp = (unsigned char *)g_vBackBuf1;
		g_vBackBuf1 = g_vBackBuf2;
		g_vBackBuf2 = temp;
	}

	/* convert the frame */
	if (g_truecolor) {
		decodeFrame16((unsigned char *)g_vBackBuf1, g_pCurMap, g_nMapLength, data+14, len-14);
	} else {
		decodeFrame8(g_vBackBuf1, g_pCurMap, g_nMapLength, data+14, len-14);
	}

	return 1;
}

static int end_chunk_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
{
	g_pCurMap=NULL;
	return 1;
}


static MVESTREAM *mve = NULL;

void MVE_ioCallbacks(mve_cb_Read io_read)
{
	mve_read = io_read;
}

void MVE_memCallbacks(mve_cb_Alloc mem_alloc, mve_cb_Free mem_free)
{
	mve_alloc = mem_alloc;
	mve_free = mem_free;
}

void MVE_sfCallbacks(mve_cb_ShowFrame showframe)
{
	mve_showframe = showframe;
}

void MVE_palCallbacks(mve_cb_SetPalette setpalette)
{
	mve_setpalette = setpalette;
}

int MVE_rmPrepMovie(void *src, int x, int y, int track)
{
	int i;

	if (mve) {
		mve_reset(mve);
		return 0;
	}

	mve = mve_open(src);

	if (!mve)
		return 1;

	g_destX = x;
	g_destY = y;

	for (i = 0; i < 32; i++)
		mve_set_handler(mve, i, default_seg_handler);

	mve_set_handler(mve, MVE_OPCODE_ENDOFSTREAM,          end_movie_handler);
	mve_set_handler(mve, MVE_OPCODE_ENDOFCHUNK,           end_chunk_handler);
	mve_set_handler(mve, MVE_OPCODE_CREATETIMER,          create_timer_handler);
	mve_set_handler(mve, MVE_OPCODE_INITAUDIOBUFFERS,     create_audiobuf_handler);
	mve_set_handler(mve, MVE_OPCODE_STARTSTOPAUDIO,       play_audio_handler);
	mve_set_handler(mve, MVE_OPCODE_INITVIDEOBUFFERS,     create_videobuf_handler);

	mve_set_handler(mve, MVE_OPCODE_DISPLAYVIDEO,         display_video_handler);
	mve_set_handler(mve, MVE_OPCODE_AUDIOFRAMEDATA,       audio_data_handler);
	mve_set_handler(mve, MVE_OPCODE_AUDIOFRAMESILENCE,    audio_data_handler);
	mve_set_handler(mve, MVE_OPCODE_INITVIDEOMODE,        init_video_handler);

	mve_set_handler(mve, MVE_OPCODE_SETPALETTE,           video_palette_handler);
	mve_set_handler(mve, MVE_OPCODE_SETPALETTECOMPRESSED, default_seg_handler);

	mve_set_handler(mve, MVE_OPCODE_SETDECODINGMAP,       video_codemap_handler);

	mve_set_handler(mve, MVE_OPCODE_VIDEODATA,            video_data_handler);

	mve_play_next_chunk(mve); /* video initialization chunk */
	if (mve_audio_enabled)
		mve_play_next_chunk(mve); /* audio initialization chunk */

	return 0;
}


void MVE_getVideoSpec(MVE_videoSpec *vSpec)
{
	vSpec->screenWidth = g_screenWidth;
	vSpec->screenHeight = g_screenHeight;
	vSpec->width = g_width;
	vSpec->height = g_height;
	vSpec->truecolor = g_truecolor;
}


int MVE_rmStepMovie()
{
	static int init_timer=0;
	int cont=1;

	if (!timer_started)
		timer_start();

	while (cont && !g_frameUpdated) // make a "step" be a frame, not a chunk...
		cont = mve_play_next_chunk(mve);
	g_frameUpdated = 0;

	if (!cont)
		return MVE_ERR_EOF;

	if (micro_frame_delay  && !init_timer) {
		timer_start();
		init_timer = 1;
	}

	do_timer_wait();

	return 0;
}

void MVE_rmEndMovie()
{
	timer_stop();
	timer_created = 0;

#ifdef AUDIO
	if (mve_audio_canplay) {
		// only close audio if we opened it
		mve_audio_canplay = 0;
		alDeleteBuffers(TOTAL_AUDIO_BUFFERS, mve_audio_buffers);
		alDeleteSources(1, &source);
	}
	mve_audio_curbuf_curpos=0;
	mve_audio_bufhead=0;
	mve_audio_buftail=0;
	mve_audio_playing=0;
	mve_audio_canplay=0;
	mve_audio_compressed=0;
	audiobuf_created = 0;
#endif

	mve_free(g_vBuffers);
	g_vBuffers = NULL;
	g_pCurMap=NULL;
	g_nMapLength=0;
	videobuf_created = 0;
	video_initialized = 0;

	mve_close(mve);
	mve = NULL;
}


void MVE_rmHoldMovie()
{
	timer_started = 0;
}


void MVE_sndInit(int x)
{
#ifdef AUDIO
	if (x == -1)
		mve_audio_enabled = 0;
	else
		mve_audio_enabled = 1;
#endif
}
