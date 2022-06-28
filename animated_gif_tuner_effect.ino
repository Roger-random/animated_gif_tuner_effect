/*

Example for ESP_8_BIT color composite video generator library on ESP32.
Connect GPIO25 to signal line, usually the center of composite video plug.

Animated GIF Tuner Effect

Using the following Arduino libraries:
* AnimatedGIF library by Larry Bank:
  https://github.com/bitbank2/AnimatedGIF
* ESP_8_BIT Color Composite Video by Roger Cheng:
  https://github.com/Roger-random/ESP_8_BIT_composite

An animated GIF is decoded into an intermediate buffer, which is then passed
into Adafruit GFX drawBitmap() function. The bitmap is drawn with a width value
intentionally not always the correct width value. When the width used is not
the correct width, the bitmap is drawn with an error that superficially
resembles analog TV tuning. This error is the intended effect of the sketch.

Copyright (c) Roger Cheng

MIT License

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#include <AnimatedGIF.h>
#include <ESP_8_BIT_GFX.h>
#include "cat_and_galactic_squid.h"

ESP_8_BIT_GFX videoOut(true /* = NTSC */, 16 /* = RGB565 colors will be downsampled to 8-bit RGB332 */);
AnimatedGIF gif;

// Vertical margin to compensate for aspect ratio
const int margin = 29;

// Draw a line of image to ESP_8_BIT_GFX frame buffer
void GIFDraw(GIFDRAW *pDraw)
{
    uint8_t *s;
    uint16_t *d, *usPalette, usTemp[320];
    int x, y;

    usPalette = pDraw->pPalette;
    y = pDraw->iY + pDraw->y; // current line

    s = pDraw->pPixels;
    if (pDraw->ucDisposalMethod == 2) // restore to background color
    {
      for (x=0; x<pDraw->iWidth; x++)
      {
        if (s[x] == pDraw->ucTransparent)
           s[x] = pDraw->ucBackground;
      }
      pDraw->ucHasTransparency = 0;
    }
    // Apply the new pixels to the main image
    if (pDraw->ucHasTransparency) // if transparency used
    {
      uint8_t *pEnd, c, ucTransparent = pDraw->ucTransparent;
      int x, iCount;
      pEnd = s + pDraw->iWidth;
      x = 0;
      iCount = 0; // count non-transparent pixels
      while(x < pDraw->iWidth)
      {
        c = ucTransparent-1;
        d = usTemp;
        while (c != ucTransparent && s < pEnd)
        {
          c = *s++;
          if (c == ucTransparent) // done, stop
          {
            s--; // back up to treat it like transparent
          }
          else // opaque
          {
             *d++ = usPalette[c];
             iCount++;
          }
        } // while looking for opaque pixels
        if (iCount) // any opaque pixels?
        {
          for(int xOffset = 0; xOffset < iCount; xOffset++ ){
            videoOut.drawPixel(pDraw->iX + x + xOffset, margin + y, usTemp[xOffset]);
          }
          x += iCount;
          iCount = 0;
        }
        // no, look for a run of transparent pixels
        c = ucTransparent;
        while (c == ucTransparent && s < pEnd)
        {
          c = *s++;
          if (c == ucTransparent)
             iCount++;
          else
             s--;
        }
        if (iCount)
        {
          x += iCount; // skip these
          iCount = 0;
        }
      }
    }
    else
    {
      s = pDraw->pPixels;
      // Translate the 8-bit pixels through the RGB565 palette (already byte reversed)
      for (x=0; x<pDraw->iWidth; x++)
      {
        videoOut.drawPixel(x,margin + y, usPalette[*s++]);
      }
    }
} /* GIFDraw() */

void setup() {
  videoOut.begin();
  videoOut.copyAfterSwap = true; // gif library depends on data from previous buffer

  // Clear screen with black
  videoOut.fillScreen(0);
  videoOut.waitForFrame();

  gif.begin(LITTLE_ENDIAN_PIXELS);
}

void loop() {
  if (gif.open((uint8_t *)cat_and_galactic_squid_gif, cat_and_galactic_squid_gif_len, GIFDraw))
  {
    while (gif.playFrame(true, NULL))
    {
      videoOut.waitForFrame();
    }
    videoOut.waitForFrame();
    gif.close();
  }
}
