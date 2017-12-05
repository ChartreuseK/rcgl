# RCGL C Graphics Library

**This library is currently unstable and may experience ABI changes**

A simple graphics library wrapper for SDL2 providing a simple linear
frame-buffer interface with palettized 256-color.  Designed to allow quick
prototyping with simple pixel graphics routines much like back in the DOS
VGA days.

With one initialization call you now have an window and frame buffer where you
can immediatly set a palette and start plotting pixels. Pixels are flushed to
the screen using the rcgl_update method.

## Features

* SDL2 based for cross platform support.
* 8-bit indexed linear frame-buffer for ease of coding.
* Scaling of arbitrary sized buffer to arbitrary window sizes.
* Minimal set-up - start prototyping immediatly


## Methods

### rcgl_init

    int rcgl_init(int w, int h, int ww, int wh, const char *title, int wflags);

Initializes library and create window, with the buffer scaled to fit the window
size.

Parameter | Purpose
--------- | -------
w         | buffer width
h         | buffer height
ww        | window width
wh        | window height
title     | window title
wflags    | Window behaviour flags, see below.

#### wflags

Flag            | Purpose
--------------- | -------
RCGL_RESIZE     | Window can be freely resized
RCGL_FULLSCREEN | Window starts off fullscreen, using the resolution specified 
RCGL_MAXIMIZED  | Window starts off maximized, must be combined with RCGL_RESIZE
RCGL_FULLSCREEN_NATIVE | Window starts off fullscreen at the desktop resolution
RCGL_INTSCALE	  | Only scale to integer multiples, letter/pillarbox differences

### rcgl_quit

    void rcgl_quit(void);

Frees up used resources and gracefully closes down the window.

### rcgl_update

    int rcgl_update(void);
    
Renders the current buffer to the window, applying the current palette to the
rendered pixels.

Returns -1 on error writing to the window.

### rcgl_setbuf

    void rcgl_setbuf(uint8_t *b);

Changes the buffer pointer to a new specified buffer. If b is *NULL* then set
the buffer pointer back to the internal buffer.

### rcgl_getbuf

    uint8_t *rcgl_getbuf(void);

Returns a pointer to the current buffer. Call this after initialization to get
a pointer to the internal buffer.
