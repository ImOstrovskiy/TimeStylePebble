#include <pebble.h>
#include "bg_image.h"

#ifdef PBL_COLOR

// the image is raw GColor8 pixels in the app's resources, copied straight from
// flash into the framebuffer row by row, so it costs no heap at all.
// a resource of any other size (like the 1-byte placeholder) means no image.
#define BG_IMAGE_SIZE (PBL_DISPLAY_WIDTH * PBL_DISPLAY_HEIGHT)

static Layer* bgLayer;
static ResHandle bgImage;

static void update_bg_layer(Layer* l, GContext* ctx) {
  GBitmap* fb = graphics_capture_frame_buffer(ctx);

  if(!fb) {
    return;
  }

  // row info also takes care of the round displays, where rows are shorter
  for(int y = 0; y < PBL_DISPLAY_HEIGHT; y++) {
    GBitmapDataRowInfo row = gbitmap_get_data_row_info(fb, y);
    resource_load_byte_range(bgImage, y * PBL_DISPLAY_WIDTH + row.min_x,
                             row.data + row.min_x, row.max_x - row.min_x + 1);
  }

  graphics_release_frame_buffer(ctx, fb);
}

void BgImage_init(Window* window) {
  bgImage = resource_get_handle(RESOURCE_ID_BG_IMAGE);

  if(resource_size(bgImage) != BG_IMAGE_SIZE) {
    return;
  }

  // added first, so it sits below the sidebar and the clock
  Layer* root = window_get_root_layer(window);
  bgLayer = layer_create(layer_get_bounds(root));
  layer_set_update_proc(bgLayer, update_bg_layer);
  layer_add_child(root, bgLayer);
}

void BgImage_deinit() {
  if(bgLayer) {
    layer_destroy(bgLayer);
    bgLayer = NULL;
  }
}

#else

// ponytail: no background images on black & white watches
void BgImage_init(Window* window) {}
void BgImage_deinit() {}

#endif
