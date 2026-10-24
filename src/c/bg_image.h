#pragma once
#include <pebble.h>

void BgImage_init(Window* window);
void BgImage_deinit();

// returns true if the message was a background image message (and was consumed)
bool BgImage_handleMessage(DictionaryIterator* iterator);
