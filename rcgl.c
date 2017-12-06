/* RCGL C Graphics Library
 *******************************************************************************
BSD 3-Clause License

Copyright (c) 2017, Hayden Kroepfl
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

* Neither the name of the copyright holder nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ******************************************************************************/

#include "rcgl.h"
#include <SDL2/SDL.h>
#include <stdint.h>
#include <stdlib.h>

/* Temporary testing main routine */
int main(void)
{
	uint8_t *buffer;
	#define WID 160
	#define HGT 120
	if (rcgl_init(WID, HGT, WID*8, HGT*8,
	              "RCGL Test Window",
	              RCGL_INTSCALE | RCGL_RESIZE) < 0)
		return -1;

	// 16 color CGA palette
	rcgl_palette[0] = 0xFF000000;
	rcgl_palette[1] = 0xFF0000AA;
	rcgl_palette[2] = 0xFF00AA00;
	rcgl_palette[3] = 0xFF00AAAA;
	rcgl_palette[4] = 0xFFAA0000;
	rcgl_palette[5] = 0xFFAA00AA;
	rcgl_palette[6] = 0xFFAA5500;
	rcgl_palette[7] = 0xFFAAAAAA;
	rcgl_palette[8] = 0xFF555555;
	rcgl_palette[9] = 0xFF5555FF;
	rcgl_palette[10] = 0xFF55FF55;
	rcgl_palette[11] = 0xFF55FFFF;
	rcgl_palette[12] = 0xFFFF5555;
	rcgl_palette[13] = 0xFFFF55FF;
	rcgl_palette[14] = 0xFFFFFF55;
	rcgl_palette[15] = 0xFFFFFFFF;

	buffer = rcgl_getbuf();

	for (int j = 0; j < HGT; j++) {
		for (int i = 0; i < WID; i++) {
			buffer[j * WID + i] = ((i)+j)%16;
		}
	}
	rcgl_update();

	// To add to library
	int quit = 0;
	SDL_Event event;

	int frames = 0;
	uint32_t starttick = SDL_GetTicks();
	while (!rcgl_hasquit()) {
		rcgl_update();
		frames++;
		
		if (frames % 6 == 0)
		{
			printf("Avg frame rate: %f\n", (float)frames / ((float)(SDL_GetTicks() - starttick)/1000.0));

			rcgl_palette[16] = rcgl_palette[0];
			for (int i = 1; i < 17; i++)
				rcgl_palette[i-1] = rcgl_palette[i];
		}

		
		rcgl_delay(10);
			
	}

	rcgl_quit();
	return 0;
}

/* LIBRARY STATE */
static SDL_Window *wind;
static SDL_Renderer *rend;
static SDL_Texture *tx;
static SDL_Thread *thread;

static SDL_cond *initcond;
static SDL_cond *waitdrawcond;
static SDL_mutex *mutex;
static int initstatus;
static int drawstatus;

static SDL_atomic_t status;

static uint32_t EVENT_TERM;
static uint32_t EVENT_REDRAW;

uint32_t rcgl_palette[256];
static int bw;                  // Buffer width
static int bh;                  // Buffer height
static uint8_t *buf;                   // Pointer to user buffer
static uint8_t *ibuf;                  // Internal/Default user buffer
static int running;				// Is the video thread still alive

static struct CARGS {
	int w, h, ww, wh;
	const char *title;
	int wflags;
} cargs;

static void blit(uint8_t *src, uint32_t *dst);
static int videothread(void *data);

#define EVENT_TIMEOUT	100		// Wait 100ms for an event

/* EXPORTED LIBRARY ROUTINES */
/*
 * rcgl_init - Initialize library and create window
 * Buffer will be scaled to fill window size
 * 
 * w - buffer width
 * h - buffer height
 * ww - window width
 * wh - window height
 * title - title
 * sc - integer pixel scale (window size is w*sc by h*sc)
 * wflags:  1 = RESIZABLE, 2 = FULLSCREEN, 4 = MAXIMIZED,
 *          8 = FULLSCREEN_NATIVE, 16 = INTEGER SCALING
 */


int rcgl_init(int w, int h, int ww, int wh, const char *title, int wflags)
{
	int rval = 0;
	int istat;

	bw = w;
	bh = h;

	cargs.w = w;
	cargs.h = h;
	cargs.ww = ww;
	cargs.wh = wh;
	cargs.title = title;
	cargs.wflags = wflags;

	// Create internal framebuffer
	if ((ibuf = calloc(w*h, sizeof(uint8_t))) == NULL) {
		fprintf(stderr, "RCGL: Failed to allocate internal framebuffer\n");
		rval = -1;
		goto failalloc;
	}
	buf = ibuf;

	mutex = SDL_CreateMutex();
	if (mutex == NULL) {
		fprintf(stderr, "RCGL: Failed to create mutex\n");
		rval = -2;
		goto failmutex;
	}
	initcond = SDL_CreateCond();
	if (initcond == NULL) {
		fprintf(stderr, "RCGL: Failed to create init condition variable\n");
		rval = -2;
		goto failcond;
	}
	waitdrawcond = SDL_CreateCond();
	if (waitdrawcond == NULL) {
		fprintf(stderr, "RCGL: Failed to create wdraw condition variable\n");
		rval = -2;
		goto failcond2;
	}

	// Create user defined events
	EVENT_TERM = SDL_RegisterEvents(2);
	if (EVENT_TERM == (uint32_t)-1) {
		fprintf(stderr, "RCGL: Failed to create user events\n");
		rval = -2;
		goto failevent;
	}
	EVENT_REDRAW = EVENT_TERM+1;

	
	
	// Start-up video thread
	thread = SDL_CreateThread(videothread, "RCGLWindowThread", NULL);
	if (thread == NULL) {
		fprintf(stderr, "RCGL: Failed to create RCGLWindowThread: %s\n",
		        SDL_GetError());
		rval = -3;
		goto failthread;
	}

	// Block till video thread has been initialized, or till an error occurs
	SDL_LockMutex(mutex);
	while (!initstatus) {
		SDL_CondWait(initcond, mutex);
	}
	istat = initstatus;
	SDL_UnlockMutex(mutex);
	if (istat < 0) { // Failure to init
		fprintf(stderr, "RCGL: Error intializing in video thread\n");
		SDL_WaitThread(thread, &rval);
		goto failthread;
	}
	// Otherwise video thread has been launched successfully
	return rval;
	// Failure path
failthread:
failevent:
	SDL_DestroyCond(waitdrawcond);
failcond2:
	SDL_DestroyCond(initcond);
failcond:
	SDL_DestroyMutex(mutex);
failmutex:
	free(ibuf);
	ibuf = NULL;
	buf = NULL;
failalloc:
	return rval;
}

void rcgl_quit(void)
{
	int rval = 0;
	// Signal to video thread to close down shop
	SDL_Event event;
	SDL_zero(event);
	event.type = EVENT_TERM;
	SDL_PushEvent(&event);

	// Wait for video thread to quit
	SDL_WaitThread(thread, &rval);
	
	// Finally destroy our buffer
	if (ibuf)
		free(ibuf);
	ibuf = NULL;
}

/*
 * rcgl_update - Render buffer to screen
 */
int rcgl_update(void)
{
	void *rbuf;
	int rval = 0;
	

	SDL_Event event;
	SDL_zero(event);
	event.type = EVENT_REDRAW;
	SDL_PushEvent(&event);

	// Wait for thread to draw changes before returning
	SDL_LockMutex(mutex);
	SDL_CondWait(waitdrawcond, mutex);

	rval = drawstatus;
	SDL_UnlockMutex(mutex);

	return rval;
}

/*
 * rcgl_setbuf - Change buffer to b
 * If b is NULL, set buffer to internal buffer
 */
void rcgl_setbuf(uint8_t *b)
{
	if (b)
		buf = b;
	else
		buf = ibuf;
}

/*
 * rcgl_getbuf - Get pointer to the current buffer
 */
uint8_t *rcgl_getbuf(void)
{
	return buf;
}

/*
 * rcgl_hasquit - Test if program has quit
 */
int rcgl_hasquit(void)
{
	return SDL_AtomicGet(&status) == 0;
}

/*
 * rcgl_delay - Delay for ms milliseconds
 */
void rcgl_delay(uint32_t ms)
{
	SDL_Delay(ms);
}

/* INTERNAL LIBRARY HELPER ROUTINES */

/*
 * blit - Render 8-bit bitmap to 32-bit bitmap using palette
 */
static void blit(uint8_t *src, uint32_t *dst)
{
	for (int y = 0; y < bh; y++)
		for (int x = 0; x < bw; x++)
			*(dst++) = rcgl_palette[*(src++)];
}

/*
 * Background and screen update handler
 *
 * NOTE: This is the only thread allowed to call WaitEvent/PollEvent/PumpEvents
 * Though to do so we need to move the SDL init code here :/
 */
static int videothread(void *data)
{
	int rval;
	SDL_Event event;
	int pitch;
	void *rbuf;
	int dstatus;

	/* Video initialization */
	SDL_Init(SDL_INIT_VIDEO);
	wind = SDL_CreateWindow(cargs.title,
	           SDL_WINDOWPOS_UNDEFINED,
	           SDL_WINDOWPOS_UNDEFINED,
	           cargs.ww,
	           cargs.wh,
	           ((cargs.wflags&RCGL_RESIZE)?SDL_WINDOW_RESIZABLE:0)
	           | ((cargs.wflags&RCGL_FULLSCREEN)?SDL_WINDOW_FULLSCREEN:0)
	           | ((cargs.wflags&RCGL_MAXIMIZED)?SDL_WINDOW_MAXIMIZED:0)
	           | ((cargs.wflags&RCGL_FULLSCREEN_NATIVE)?SDL_WINDOW_FULLSCREEN_DESKTOP:0)
	           | SDL_WINDOW_ALLOW_HIGHDPI);
	if (wind == NULL) {
		fprintf(stderr, "RCGL: Failed to create Window: %s\n",
		        SDL_GetError());
		rval = -4;
		goto failwind;
	}
	
	rend = SDL_CreateRenderer(wind, -1, 0);
	if (rend == NULL) {
		fprintf(stderr, "RCGL: Failed to create Renderer: %s\n",
		        SDL_GetError());
		rval = -4;
		goto failrend;
	}
	SDL_RenderSetLogicalSize(rend, cargs.w, cargs.h);
	SDL_RenderSetIntegerScale(rend, cargs.wflags & RCGL_INTSCALE);
	
	tx = SDL_CreateTexture(rend,
	                       SDL_PIXELFORMAT_ARGB8888,
	                       SDL_TEXTUREACCESS_STREAMING,
	                       cargs.w,
	                       cargs.h);
	if (tx == NULL) {
		fprintf(stderr, "RCGL: Failed to create Texture: %s\n",
		        SDL_GetError());
		rval = -4;
		goto failtx;
	}

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, 0);
	SDL_SetRenderDrawColor(rend, 0, 0, 0, 0);
	SDL_RenderClear(rend);
	SDL_GL_SetSwapInterval(1);

	SDL_AtomicSet(&status, 1);

	// Signal to parent thread that initialization has been successful
	SDL_LockMutex(mutex);
	initstatus = 1;
	SDL_CondBroadcast(initcond);
	SDL_UnlockMutex(mutex);
	
	running = 1;
	while (running) {
		if (SDL_WaitEvent(&event)) {
			// Handle events
			do {
				if (event.type == EVENT_REDRAW) {
					dstatus = 1;
					if (0 == SDL_LockTexture(tx, NULL, &rbuf, &pitch)) 
						blit(buf, (uint32_t *)rbuf);	  // Palettize and copy to texture
					else // Otherwise Failed to open texture, couldn't render.
						dstatus = 0;
						
					SDL_UnlockTexture(tx);
					SDL_SetRenderDrawColor(rend, 0, 0, 0, 0);
					SDL_RenderClear(rend);
					SDL_RenderCopy(rend, tx, NULL, NULL); // Render texture to entire window
					SDL_RenderPresent(rend);              // Do update

					// Let update method return now that we're done
					SDL_LockMutex(mutex);
					SDL_CondBroadcast(waitdrawcond);
					drawstatus = dstatus;
					SDL_UnlockMutex(mutex);
				}
				else if (event.type == EVENT_TERM) {
					SDL_AtomicSet(&status, 0);
					running = 0;
				}
				else switch (event.type) {
				case SDL_QUIT:
					SDL_AtomicSet(&status, 0);
					running = 0;
					break;
				case SDL_WINDOWEVENT:
					// Assume something happened to the window, so just redraw
					SDL_SetRenderDrawColor(rend, 0, 0, 0, 0);
					SDL_RenderClear(rend);
					SDL_RenderCopy(rend, tx, NULL, NULL); // Render texture to entire window
					SDL_RenderPresent(rend);              // Do update
					break;
				
				}
				
			} while (SDL_PollEvent(&event));
		}

	}

	
failalloc:
	SDL_DestroyTexture(tx);
failtx:
	SDL_DestroyRenderer(rend);
failrend:
	SDL_DestroyWindow(wind);
failwind:
	SDL_Quit();

	
	// Signal to parent thread that we failed
	if (rval < 0) {
		SDL_LockMutex(mutex);
		initstatus = rval;
		SDL_CondBroadcast(initcond);
		SDL_UnlockMutex(mutex);
	}
	
	return rval;
}
