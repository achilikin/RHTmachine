/* Support for bitmap fonts generated by https://github.com/achilikin/bdfe

   Copyright (c) 2015 Andrey Chilikin (https://github.com/achilikin)

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
   THE SOFTWARE.
   https://opensource.org/licenses/MIT
*/
#ifndef BITMAP_FONT_H
#define BITMAP_FONT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#if 0
} // dummy bracket for Visual Assist
#endif
#endif

#define USE_BMFONT_6x8  0

#if USE_BMFONT_6x8
	#define BMFONT_6x8  0
	#define BMFONT_8x8  1
	#define BMFONT_8x16 2
	#define BMFONT_USER 3
#else
	#define BMFONT_8x8  0
	#define BMFONT_8x16 1
	#define BMFONT_USER 2
#endif

#define BMFONT_MAX  BMFONT_USER

typedef struct bmfont_s
{
	uint8_t gw; /*< glyph width  */
	uint8_t gh; /*< glyph height */
	uint8_t go; /*< font offset, first glyph index */
	uint8_t gn; /*< number of glyphs presented */
	const uint8_t *font;
} bmfont_t;

/** text attributes */
#define TEXT_REVERSE   0x01
#define TEXT_UNDERLINE 0x02
#define TEXT_OVERLINE  0x04

/** select one of three fonts for following text drawing calls */
uint8_t bmfont_select(uint8_t font);

/** get currently selected font */
bmfont_t *bmfont_get(void);

/** 
 set user font selectable by BMFONT_USER to nfont
 store current user font in ofont (if not NULL)
 */
void bmfont_set(bmfont_t *nfont, bmfont_t *ofont);

#ifdef __cplusplus
}
#endif

#endif
