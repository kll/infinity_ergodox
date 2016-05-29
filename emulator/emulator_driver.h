/*
 * emulator_driver.h
 *
 *  Created on: 29 maj 2016
 *      Author: Fred Wales
 */

#ifndef EMULATOR_EMULATOR_DRIVER_H_
#define EMULATOR_EMULATOR_DRIVER_H_

#include "src/gdisp/gdisp_driver.h"

typedef struct pixmap {
	color_t			pixels[1];			// We really want pixels[0] but some compilers don't allow that even though it is C standard.
} pixmap;

#if !defined(GDISP_DRIVER_VMT)
static pixel_t	*getEmulatorPixmap(GDisplay *g) {
	return ((pixmap *)g->priv)->pixels;
}
#endif

#endif /* EMULATOR_EMULATOR_DRIVER_H_ */
