/*
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <png.h>

void luma2png(char *filename, const uint8_t *const luma, int width, int height) {
  int x,y;

  FILE *fp = fopen(filename, "wb");
  if(!fp) abort();

  png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!png) abort();

  png_infop info = png_create_info_struct(png);
  if (!info) abort();

  if (setjmp(png_jmpbuf(png))) abort();

  png_init_io(png, fp);

  png_set_IHDR(
    png,
    info,
    width, height,
    8,
    PNG_COLOR_TYPE_GRAY,
    PNG_INTERLACE_NONE,
    PNG_COMPRESSION_TYPE_DEFAULT,
    PNG_FILTER_TYPE_DEFAULT
  );
  png_write_info(png, info);

  png_bytep *row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * height);
  for(y = 0; y < height; y++) {
    row_pointers[y] = (png_byte*)malloc(png_get_rowbytes(png,info));
    for (x = 0; x < width; x++) {
      row_pointers[y][x] = luma[y * width + x];
    }
  }

  png_write_image(png, row_pointers);
  png_write_end(png, NULL);

  for(y = 0; y < height; y++) {
    free(row_pointers[y]);
  }
  free(row_pointers);

  fclose(fp);
}
