# Analog TV Tuner Effect using AnimatedGIF and ESP_8_BIT_composite libraries

This repository is an Arduino project that:
1. Simulates an analog TV tuning effect, while
2. Decoding an animated GIF via 
[Larry Bank's AnimatedGIF library](https://github.com/bitbank2/AnimatedGIF), and
3. Rendered on screen via
[my ESP_8_BIT_composite library](https://github.com/Roger-random/ESP_8_BIT_composite)

This code is derived from the AnimatedGIF example of ESP_8_BIT_composite library, using the same
animated file
[Cat and Galactic Squid](https://twitter.com/MLE_Online/status/1393660363191717888)
by Emily Velasco
([CC BY-SA 4.0](https://creativecommons.org/licenses/by-sa/4.0/))

The simulated tuning dial is a potentiometer connected to ESP32 GPIO pin 13, which can be changed to
[any ADC-capable ESP32 pin](https://randomnerdtutorials.com/esp32-pinout-reference-gpios/).

At a specific potentiometer position, it is "in tune" and the animation is rendered correctly exactly
as in the original example sketch. But as the potentiometer is turned, the image goes "out of tune"
and breaks up. This effect is simulated in two parts:
* Horizontal distortion is done by copying image with intentionally incorrect image stride values.
* Vertical distortion is done by copying image with intentionally incorrect height offsets.

I first saw this digital emulation of analog tuning (via intentionally incorrect offsets) on
[Emily Velasco's Twitter feed](https://twitter.com/MLE_Online/status/1536463544475979776).
