#ifndef ENGINE_SOURCE_JPEG_JMEMSRC_H
#define ENGINE_SOURCE_JPEG_JMEMSRC_H

#include <cstdio>
#include "jpeglib.h"

typedef struct mem_source_struct * mem_source_ptr;

struct mem_source_struct {
  /* 0000 */ void (*start_input) (j_compress_ptr cinfo, mem_source_ptr sinfo);
  /* 0004 */ JDIMENSION (*get_pixel_rows) (j_compress_ptr cinfo, mem_source_ptr sinfo);
  /* 0008 */ void (*finish_input) (j_compress_ptr cinfo, mem_source_ptr sinfo);
  /* 000C */ FILE * input_file;
  /* 0010 */ JSAMPARRAY buffer;
  /* 0014 */ JDIMENSION buffer_height;
  /* 0018 */ const JOCTET * pixels;
  /* 001C */ JDIMENSION position;
  /* 0020 */ JDIMENSION size;
  /* 0024 */ JDIMENSION width;
  /* 0028 */ JDIMENSION height;
  /* 002C */ int bytes_per_pixel;
  /* 0030 */ j_compress_ptr cinfo;
  /* 0034 */ int unused_0034;
  /* 0038 */ void (*read_pixel) (mem_source_ptr sinfo);
  /* 003C */ JOCTET pixel[4];
  /* 0040 */ JDIMENSION (*get_rows) (j_compress_ptr cinfo, mem_source_ptr sinfo);
};

mem_source_ptr jinit_read_mem(j_compress_ptr cinfo);

#endif
