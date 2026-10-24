#pragma once
#include <pebble.h>

// background image from resources/data/bg_image~<platform>.bin (see tools/bg_image.py)
void BgImage_init(Window* window);
void BgImage_deinit();
