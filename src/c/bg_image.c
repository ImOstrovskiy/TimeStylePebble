#include <pebble.h>
#include "bg_image.h"

#ifdef PBL_COLOR

// the image is only kept in RAM (persistent storage is way too small for it),
// so the phone re-sends it every time the watchface starts.
// until it has fully arrived, the regular background color is shown instead.
#define BG_IMAGE_SIZE (PBL_DISPLAY_WIDTH * PBL_DISPLAY_HEIGHT)

static Layer* bgLayer;
static GBitmap* bgBitmap;
static uint32_t bgReceived;

static void update_bg_layer(Layer* l, GContext* ctx) {
  if(bgBitmap && bgReceived == BG_IMAGE_SIZE) {
    graphics_draw_bitmap_in_rect(ctx, bgBitmap, layer_get_bounds(l));
  }
}

void BgImage_init(Window* window) {
  Layer* root = window_get_root_layer(window);
  bgLayer = layer_create(layer_get_bounds(root));
  layer_set_update_proc(bgLayer, update_bg_layer);
  layer_add_child(root, bgLayer);
}

void BgImage_deinit() {
  layer_destroy(bgLayer);
  bgLayer = NULL;

  if(bgBitmap) {
    gbitmap_destroy(bgBitmap);
    bgBitmap = NULL;
  }
}

bool BgImage_handleMessage(DictionaryIterator* iterator) {
  Tuple* size_tuple = dict_find(iterator, MESSAGE_KEY_BgImageSize);
  Tuple* offset_tuple = dict_find(iterator, MESSAGE_KEY_BgImageOffset);
  Tuple* data_tuple = dict_find(iterator, MESSAGE_KEY_BgImageData);

  // a size means a new image is starting (or 0, which removes the image)
  if(size_tuple) {
    if(bgBitmap) {
      gbitmap_destroy(bgBitmap);
      bgBitmap = NULL;
    }
    bgReceived = 0;

    // an image made for another watch model is ignored. fctx allocates another
    // full-screen buffer while drawing the time, so skip the image unless both fit
    // (otherwise the app crashes). only emery has enough heap for this.
    // ponytail: 2KB margin, ~3KB was left at peak on emery with the heaviest widgets; raise it if crashes show up
    if(size_tuple->value->uint32 == BG_IMAGE_SIZE && heap_bytes_free() > 2 * BG_IMAGE_SIZE + 2048) {
      bgBitmap = gbitmap_create_blank(GSize(PBL_DISPLAY_WIDTH, PBL_DISPLAY_HEIGHT), GBitmapFormat8Bit);
    }

    if(bgLayer) {
      layer_mark_dirty(bgLayer);
    }
  }

  // chunks must arrive in order; a resent chunk is simply ignored
  if(bgBitmap && offset_tuple && data_tuple &&
     offset_tuple->value->uint32 == bgReceived &&
     bgReceived + data_tuple->length <= BG_IMAGE_SIZE) {
    uint8_t* pixels = gbitmap_get_data(bgBitmap);
    uint16_t stride = gbitmap_get_bytes_per_row(bgBitmap);

    for(uint16_t i = 0; i < data_tuple->length; i++) {
      uint32_t p = bgReceived + i;
      pixels[(p / PBL_DISPLAY_WIDTH) * stride + p % PBL_DISPLAY_WIDTH] = data_tuple->value->data[i];
    }

    bgReceived += data_tuple->length;

    if(bgReceived == BG_IMAGE_SIZE && bgLayer) {
      layer_mark_dirty(bgLayer);
    }
  }

  return size_tuple || data_tuple;
}

#else

// ponytail: no background images on black & white watches
void BgImage_init(Window* window) {}
void BgImage_deinit() {}
bool BgImage_handleMessage(DictionaryIterator* iterator) { return false; }

#endif
