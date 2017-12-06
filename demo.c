#include "rcgl.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define WID 320
#define HGT 200

int main(void)
{
	uint8_t *buffer;

	if (rcgl_init(WID, HGT, WID*4, HGT*4,
	              "RCGL Test Window",
	              RCGL_INTSCALE | RCGL_RESIZE) < 0)
		return -1;
	// Use default Mode 13H VGA Palette

	// Get a pointer to the framebuffer
	buffer = rcgl_getbuf();
	
	while (!rcgl_hasquit()) {
		rcgl_update();

		for (int i = 0; i < 512; i++)
			rcgl_plot(rand()%WID, rand()%HGT, rand()%256);

		rcgl_delay(100);
			
	}

	rcgl_quit();
	return 0;
}
