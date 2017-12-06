#include "rcgl.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

int main(void)
{
	uint8_t *buffer;
	#define WID 80
	#define HGT 60
	if (rcgl_init(WID, HGT, WID*16, HGT*16,
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

	rcgl_update();

	
	rcgl_delay(5000);
	
	while (!rcgl_hasquit()) {
		rcgl_update();

		for (int i = 0; i < 512; i++)
			rcgl_plot(rand()%WID, rand()%HGT, rand()%16);

		rcgl_delay(100);
			
	}

	rcgl_quit();
	return 0;
}
