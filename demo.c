#include "rcgl.h"

#include <stdio.h>
#include <stdint.h>


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

	int frames = 0;
	uint32_t starttick = rcgl_ticks();
	while (!rcgl_hasquit()) {
		rcgl_update();
		frames++;
		
		if (frames % 6 == 0)
		{
			printf("Avg frame rate: %f\n", (float)frames / ((float)(rcgl_ticks() - starttick)/1000.0));

			rcgl_palette[16] = rcgl_palette[0];
			for (int i = 1; i < 17; i++)
				rcgl_palette[i-1] = rcgl_palette[i];
		}

		
		rcgl_delay(10);
			
	}

	rcgl_quit();
	return 0;
}
