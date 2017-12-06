# RCGL C Graphics Library

**This library is currently unstable and may experience API changes**

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
* Minimal set-up - start prototyping immediatly. See demo.c to see how fast you can be plotting pixels.
* Built-in preset palettes. Mode 13h VGA (EGA/CGA) and Greyscale

## Planned Features

* Simple graphics primative functions. Lines, Rectangles, Circles, Arcs.
* Text rendering to buffer.
* Built in fonts for text rendering. 8x8 CGA, Apple ][, 9x16 VGA, and more.
* More built-in palettes
* Simple mouse and keyboard routines, polling based. Get clicked pixel as buffer coords.
* Togglable vsync. (Currently always on)
* Non-square pixel scaling. For emulating old compure aspect ratios. (eg. 320x200 as 4:3)

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

Changes the buffer pointer to a new specified buffer. If **b** is *NULL* then
set the buffer pointer back to the internal buffer.

### rcgl_getbuf

    uint8_t *rcgl_getbuf(void);

Returns a pointer to the current buffer. Call this after initialization to get
a pointer to the internal buffer.

### rcgl_hasquit

    int rcgl_hasquit(void);

Returns *true* if a close event has occured and the video system has terminated


### rcgl_delay

    void rcgl_delay(uint32_t ms);

Waits for the given number of milliseconds before returning.

### rcgl_ticks

    uint32_t rcgl_ticks(void);

Return number of milliseconds since start

### rcgl_plot

    void rcgl_plot(int x, int y, uint8_t c);

Plot a pixel to the coordinates x,y with color c

### rcgl_setpalette

    void rcgl_setpalette(const uint32_t palette[256]);
    
Copy the specified palette into the palette registers. Can be used to swap
quickly between multiple palettes, or to return to one of the built-in palettes.
eg:

    rcgl_setpalette(RCGL_PALETTE_VGA);

#### Built in palettes

 Palette name     | Description
 ---------------- | -------
RCGL_PALETTE_VGA  | The default palette for Mode 13h VGA
RCGL_PALETTE_GREY | A linear greyscale palette. 0 = #000000, 256 = #FFFFFF

## The Palette

The 256-color palette can be directly manipulated by the program to allow for
palette swapping effects. It is present as a global variable:

    extern uint32_t rcgl_palette[256];

The values are stored as ARGB (0xaarrggbb) where the alpha channel is ignored
by the library, allowing for 24-bit color palettes. The palette defaults to
the values from RCGL_PALETTE_VGA - the Mode 13h VGA palette.
