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
	while (!quit) {
		rcgl_update();
		frames++;
		while (SDL_PollEvent(&event) != 0) {
			if (event.type == SDL_QUIT)
				quit = 1;
		}

		if (frames % 6 == 0)
		{
			printf("Avg frame rate: %f\n", (float)frames / ((float)(SDL_GetTicks() - starttick)/1000.0));

			palette[16] = palette[0];
			for (int i = 1; i < 17; i++)
				rcgl_palette[i-1] = rcgl_palette[i];
		}

		
		
			
	}

	rcgl_quit();
	return 0;
}

/* LIBRARY STATE */
static SDL_Window *wind;
static SDL_Renderer *rend;
static SDL_Texture *tx;
static SDL_Thread *thread;
uint32_t rcgl_palette[256];
static int bw;                  // Buffer width
static int bh;                  // Buffer height
static uint8_t *buf;                   // Pointer to user buffer
static uint8_t *ibuf;                  // Internal/Default user buffer
static int running;				// Is the video thread still alive

static void blit(uint8_t *src, uint32_t *dst);


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
	
	SDL_Init(SDL_INIT_VIDEO);
	wind = SDL_CreateWindow(title,
	           SDL_WINDOWPOS_UNDEFINED,
	           SDL_WINDOWPOS_UNDEFINED,
	           ww,
	           wh,
	           ((wflags&RCGL_RESIZE)?SDL_WINDOW_RESIZABLE:0)
	           | ((wflags&RCGL_FULLSCREEN)?SDL_WINDOW_FULLSCREEN:0)
	           | ((wflags&RCGL_MAXIMIZED)?SDL_WINDOW_MAXIMIZED:0)
	           | ((wflags&RCGL_FULLSCREEN_NATIVE)?SDL_WINDOW_FULLSCREEN_DESKTOP:0)
	           | SDL_WINDOW_ALLOW_HIGHDPI);
	if (wind == NULL) {
		fprintf(stderr, "RCGL: Failed to create Window: %s\n",
		        SDL_GetError());
		rval = -1;
		goto failwind;
	}
	
	rend = SDL_CreateRenderer(wind, -1, 0);
	if (rend == NULL) {
		fprintf(stderr, "RCGL: Failed to create Renderer: %s\n",
		        SDL_GetError());
		rval = -1;
		goto failrend;
	}
	SDL_RenderSetLogicalSize(rend, w, h);
	SDL_RenderSetIntegerScale(rend, wflags & RCGL_INTSCALE);
	
	tx = SDL_CreateTexture(rend,
	                       SDL_PIXELFORMAT_ARGB8888,
	                       SDL_TEXTUREACCESS_STREAMING,
	                       w,
	                       h);GA
	if (tx == NULL) {
		fprintf(stderr, "RCGL: Failed to create Texture: %s\n",
		        SDL_GetError());
		rval = -1;
		goto failtx;
	}

	if ((ibuf = calloc(w*h, sizeof(uint8_t))) == NULL) {
		fprintf(stderr, "RCGL: Failed to allocate internal framebuffer\n");
		rval = -2;
		goto failalloc;
	}
	buf = ibuf;

	bw = w;
	bh = h; 

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, 0);
	SDL_SetRenderDrawColor(rend, 0, 0, 0, 0);
	SDL_RenderClear(rend);
	SDL_GL_SetSwapInterval(1);
	rcgl_update();


	running = 1;
	// Start-up event handler thread
	thread = SDL_CreateThread(videothread, "RCGLWindowThread", NULL);
	if (thread == NULL) {
		fprintf(stderr, "RCGL: Failed to create RCGLWindowThread: %s\n",
		        SDL_GetError());
		rval = -3;
		goto failthread;
	}
	

	return rval;
failthread:
failalloc:
	SDL_DestroyTexture(tx);
failtx:
	SDL_DestroyRenderer(rend);
failrend:
	SDL_DestroyWindow(wind);
failwind:
	SDL_Quit();
	return rval;
}

void rcgl_quit(void)
{
	if (ibuf)
		free(ibuf);
	ibuf = NULL;

	SDL_DestroyTexture(tx);
	SDL_DestroyRenderer(rend);
	SDL_DestroyWindow(wind);
	SDL_Quit();
}

/*
 * rcgl_update - Render buffer to screen
 */
int rcgl_update(void)
{
	void *rbuf;
	int rval = 0;
	int pitch;
	
	if (0 == SDL_LockTexture(tx, NULL, &rbuf, &pitch))
		blit(buf, (uint32_t *)rbuf);	  // Palettize and copy to texture
	else // Failed to open texture, couldn't render
		rval = -1;
	
	SDL_UnlockTexture(tx);
	SDL_SetRenderDrawColor(rend, 0, 0, 0, 0);
	SDL_RenderClear(rend);
	SDL_RenderCopy(rend, tx, NULL, NULL); // Render texture to entire window
	SDL_RenderPresent(rend);              // Do update

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
	SDL_Event event;
	while (running) {
		if (SDL_WaitEventTimeout(&event, EVENT_TIMEOUT)) {
			// Handle event
		}
		

	}
}
