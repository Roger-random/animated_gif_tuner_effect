/*

Example for ESP_8_BIT color composite video generator library on ESP32.
Connect GPIO25 to signal line, usually the center of composite video plug.

Animated GIF Tuner Effect

Using the following Arduino libraries:
* AnimatedGIF library by Larry Bank:
  https://github.com/bitbank2/AnimatedGIF
* ESP_8_BIT Color Composite Video by Roger Cheng:
  https://github.com/Roger-random/ESP_8_BIT_composite

An animated GIF is decoded into an intermediate buffer, which is then copied
into the display buffer. The copy is done with a variable image stride width
intentionally not always the correct width value. When the width used is not
the correct width, the bitmap is drawn with an error that superficially
resembles analog TV tuning. This error is the intended effect of this sketch.

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

ESP_8_BIT_composite videoOut(true /* = NTSC */);
AnimatedGIF gif;

// The most recently used vertical roll offset.
int verticalRoll;

// Vertical offset to use when picture is "in tune"
int verticalOffset;

// Intermediate buffer filled with data from AnimatedGIF
// to copy into display buffer.
uint8_t* intermediateBuffer;
int gif_height;
int gif_width;

// When to display the next frame
long millisNextFrame;

// Convert RGB565 to RGB332
uint8_t convertRGB565toRGB332(uint16_t color)
{
  // Extract most significant 3 red, 3 green and 2 blue bits.
  return (uint8_t)(
        (color & 0xE000) >> 8 |
        (color & 0x0700) >> 6 |
        (color & 0x0018) >> 3
      );
}

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
            intermediateBuffer[y*gif_width + pDraw->iX + x + xOffset] = convertRGB565toRGB332(usTemp[xOffset]);
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
        intermediateBuffer[y*gif_width + x] = convertRGB565toRGB332(usPalette[*s++]);
      }
    }
} /* GIFDraw() */

void setup() {
  Serial.begin(115200);

  videoOut.begin();

  gif.begin(LITTLE_ENDIAN_PIXELS);

  millisNextFrame = millis();

  intermediateBuffer = NULL;
  if (gif.open((uint8_t *)cat_and_galactic_squid_gif, cat_and_galactic_squid_gif_len, GIFDraw))
  {
    uint8_t* allocated = NULL;
    bool allocateSuccess = true;

    gif_width = gif.getCanvasWidth();
    gif_height = gif.getCanvasHeight();
    Serial.print("Successfully opened GIF data ");
    Serial.print(gif_width);
    Serial.print(" wide and ");
    Serial.print(gif_height);
    Serial.println(" high.");

    allocated = new uint8_t[gif_height*gif_width];
    if (NULL==allocated)
    {
      Serial.println("Allocation failed: buffer line array");
      allocateSuccess = false;
    }
    if (allocateSuccess)
    {
      intermediateBuffer = allocated;
      allocated = NULL;
      Serial.println("Successfully allocated intermediate buffer");
    }
  }
  else
  {
    gif_width = 0;
    gif_height = 0;
    Serial.println("Failed to open GIF data");
  }

  verticalRoll = 0;
  verticalOffset = (240-gif_height)/2;
}

void clearFrame()
{
  uint8_t** pFrameBuffer = videoOut.getFrameBufferLines();
  for(int i = 0; i < 240; i++)
  {
    memset(pFrameBuffer[i],0, 256);
  }
}

void copyIntermediateToFrame(int offset_h, int offset_v)
{
  uint8_t** pFrameBuffer = videoOut.getFrameBufferLines();
  for(int i = 0; i < gif_height; i++)
  {
    memcpy(pFrameBuffer[(i+offset_v)%240],&(intermediateBuffer[i*offset_h]),gif_width);
  }
}

void loop() {
  int horizontalOffset = map(analogRead(13), 0, 4095, gif_width+25, gif_width-25);

  if (horizontalOffset < gif_width-3 || horizontalOffset > gif_width+3)
  {
    verticalRoll += horizontalOffset-gif_width;
    if (verticalOffset + verticalRoll < 0)
    {
      // Keep verticalOffset + verticalRoll within range of screen.
      verticalRoll += 240;
    }
    else if (verticalOffset + verticalRoll > 240)
    {
      // Keep verticalOffset + verticalRoll within range of screen.
      verticalRoll -= 240;
    }
  }
  else
  {
    verticalRoll = 0;
  }

  if (millis() > millisNextFrame)
  {
    int millisFrame;

    if(!gif.playFrame(false, &millisFrame))
    {
      // No more frames, reset the loop to start again.
      gif.reset();
    }

    millisNextFrame = millis() + millisFrame;
  }

  clearFrame();
  copyIntermediateToFrame(horizontalOffset, verticalOffset + verticalRoll);
  videoOut.waitForFrame();
}
