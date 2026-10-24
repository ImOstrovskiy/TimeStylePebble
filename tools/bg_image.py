#!/usr/bin/env python3
"""Turns a photo into TimeStyle background image resources for the color watches.

Crops it to cover each screen, reduces it to the 64 Pebble colors with
Floyd-Steinberg dithering and writes raw GColor8 pixels, which the watchface
copies straight into the framebuffer (see src/c/bg_image.c).

usage:   uv run --with pillow tools/bg_image.py photo.jpg
         pebble build
remove:  delete resources/data/bg_image~*.bin and rebuild
"""
import os
import sys

from PIL import Image, ImageOps

SIZES = {'basalt': (144, 168), 'chalk': (180, 180), 'emery': (200, 228), 'gabbro': (260, 260)}
OUT_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'resources', 'data')

# 2 bits per channel, so each 0-255 channel snaps to 0, 85, 170 or 255
PALETTE = [level * 85 for r in range(4) for g in range(4) for b in range(4) for level in (r, g, b)]


def convert(img, size):
    fitted = ImageOps.fit(img, size, Image.LANCZOS)
    palette_img = Image.new('P', (1, 1))
    palette_img.putpalette(PALETTE)
    quantized = fitted.quantize(palette=palette_img, dither=Image.Dither.FLOYDSTEINBERG)

    # map palette indexes back through their colors, so padding entries can't sneak in
    pal = quantized.getpalette()
    lut = [0xC0 | (pal[i * 3] // 85) << 4 | (pal[i * 3 + 1] // 85) << 2 | (pal[i * 3 + 2] // 85)
           for i in range(len(pal) // 3)]
    return bytes(lut[i] for i in quantized.tobytes())


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__)

    img = ImageOps.exif_transpose(Image.open(sys.argv[1])).convert('RGB')

    for platform, size in SIZES.items():
        data = convert(img, size)
        assert len(data) == size[0] * size[1] and all(b >= 0xC0 for b in data)

        path = os.path.join(OUT_DIR, 'bg_image~%s.bin' % platform)
        with open(path, 'wb') as f:
            f.write(data)
        print('%s: %dx%d -> %s' % (platform, size[0], size[1], os.path.relpath(path)))


if __name__ == '__main__':
    main()
