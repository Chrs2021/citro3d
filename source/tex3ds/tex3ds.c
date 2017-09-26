/*------------------------------------------------------------------------------
 * Copyright (c) 2017
 *     Michael Theall (mtheall)
 *
 * This file is part of citro3d.
 *
 * This software is provided 'as-is', without any express or implied warranty.
 * In no event will the authors be held liable for any damages arising from the
 * use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 *   1. The origin of this software must not be misrepresented; you must not
 *      claim that you wrote the original software. If you use this software in
 *      a product, an acknowledgment in the product documentation would be
 *      appreciated but is not required.
 *   2. Altered source versions must be plainly marked as such, and must not be
 *      misrepresented as being the original software.
 *   3. This notice may not be removed or altered from any source distribution.
 *----------------------------------------------------------------------------*/
/** @file tex3ds.c
 *  @brief Tex3DS routines
 */
#include "tex3ds.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/** @brief Tex3DS texture
 */
struct Tex3DS_Texture_s
{
  uint16_t          numSubTextures; ///< Number of sub-textures
  uint16_t          width;          ///< Texture width
  uint16_t          height;         ///< Texture height
  uint8_t           format;         ///< Texture format
  uint8_t           mipmapLevels;   ///< Number of mipmaps
  Tex3DS_SubTexture subTextures[];  ///< Sub-textures
};

/** @brief Decode a uint8_t value
 *  @param[in]  buffer   Tex3DS buffer object
 *  @param[out] out      Output buffer
 *  @param[in]  callback Data callback
 *  @param[in]  userdata User data passed to callback
 *  @returns Whether succeeded
 */
static inline bool
decode_u8(Tex3DS_Buffer *buffer, uint8_t *out,
          Tex3DS_DataCallback callback, void *userdata)
{
  return Tex3DS_BufferGet(buffer, out, callback, userdata);
}

/** @brief Decode a uint16_t value
 *  @param[in]  buffer   Tex3DS buffer object
 *  @param[out] out      Output buffer
 *  @param[in]  callback Data callback
 *  @param[in]  userdata User data passed to callback
 *  @returns Whether succeeded
 */
static inline bool
decode_u16(Tex3DS_Buffer *buffer, uint16_t *out,
           Tex3DS_DataCallback callback, void *userdata)
{
  uint8_t in[2];
  if(!Tex3DS_BufferGet(buffer, &in[0], callback, userdata)
  || !Tex3DS_BufferGet(buffer, &in[1], callback, userdata))
    return false;

  *out = in[0] | (in[1] << 8);
  return true;
}

/** @brief Decode a uint32_t value
 *  @param[in]  buffer   Tex3DS buffer object
 *  @param[out] out      Output buffer
 *  @param[in]  callback Data callback
 *  @param[in]  userdata User data passed to callback
 *  @returns Whether succeeded
 */
static inline bool
decode_u32(Tex3DS_Buffer *buffer, uint32_t *out,
           Tex3DS_DataCallback callback, void *userdata)
{
  uint8_t in[4];
  if(!Tex3DS_BufferGet(buffer, &in[0], callback, userdata)
  || !Tex3DS_BufferGet(buffer, &in[1], callback, userdata)
  || !Tex3DS_BufferGet(buffer, &in[2], callback, userdata)
  || !Tex3DS_BufferGet(buffer, &in[3], callback, userdata))
    return false;

  *out = in[0] | (in[1] << 8) | (in[2] << 16) | (in[3] << 24);
  return true;
}

/** @brief Decode a float value
 *  @param[in]  buffer   Tex3DS buffer object
 *  @param[out] out      Output buffer
 *  @param[in]  callback Data callback
 *  @param[in]  userdata User data passed to callback
 *  @returns Whether succeeded
 */
static inline bool
decode_float(Tex3DS_Buffer *buffer, float *out,
             Tex3DS_DataCallback callback, void *userdata)
{
  uint16_t value;

  if(!decode_u16(buffer, &value, callback, userdata))
    return false;

  *out = value / 1024.0f;
  return true;
}

/** @brief Decode a Tex3DS_SubTexture
 *  @param[in]  buffer   Tex3DS buffer object
 *  @param[out] subtex   Output buffer
 *  @param[in]  callback Data callback
 *  @param[in]  userdata User data passed to callback
 *  @returns Whether succeeded
 */
static bool
decode_subtexture(Tex3DS_Buffer *buffer, Tex3DS_SubTexture *subtex,
                  Tex3DS_DataCallback callback, void *userdata)
{
  return decode_u16  (buffer, &subtex->width,  callback, userdata)
      && decode_u16  (buffer, &subtex->height, callback, userdata)
      && decode_float(buffer, &subtex->left,   callback, userdata)
      && decode_float(buffer, &subtex->top,    callback, userdata)
      && decode_float(buffer, &subtex->right,  callback, userdata)
      && decode_float(buffer, &subtex->bottom, callback, userdata);
}

/** @brief Calculate texture size
 *  @param[in] format GPU color format
 *  @param[in] num    Number of pixels
 *  @returns Size of texture
 */
static size_t
calc_tex_size(GPU_TEXCOLOR format, size_t num)
{
  switch(format)
  {
    // 32bpp
    case GPU_RGBA8:
      return num * 4;

    // 24bpp
    case GPU_RGB8:
      return num * 3;

    // 16bpp
    case GPU_RGBA5551:
    case GPU_RGB565:
    case GPU_RGBA4:
    case GPU_LA8:
    case GPU_HILO8:
      return num * 2;

    // 8bpp
    case GPU_L8:
    case GPU_A8:
    case GPU_LA4:
    case GPU_ETC1A4:
      return num;

    // 4bpp
    case GPU_L4:
    case GPU_A4:
    case GPU_ETC1:
      return num / 2;
  }

  return 0;
}

/** @brief Import Tex3DS texture
 *  @param[in]  buffer   Tex3DS buffer object
 *  @param[out] tex      Citro3D texture
 *  @param[out] texcube  Citro3D texcube
 *  @param[in]  vram     Whether to store textures in VRAM
 *  @param[in]  callback Data callback
 *  @param[in]  userdata User data passed to callback
 *  @returns Tex3DS texture
 */
static Tex3DS_Texture
texture_import(Tex3DS_Buffer *buffer, C3D_Tex *tex, C3D_TexCube *texcube, bool vram,
               Tex3DS_DataCallback callback, void *userdata)
{
  // get number of subtextures
  uint16_t numSubTextures;
  if(!decode_u16(buffer, &numSubTextures, callback, userdata))
    return NULL;

  // allocate space for header + subtextures
  Tex3DS_Texture texture;
  size_t size = sizeof(*texture);
  size += numSubTextures * sizeof(Tex3DS_SubTexture);

  texture = (Tex3DS_Texture)malloc(size);
  if(!texture)
    return NULL;

  texture->numSubTextures = numSubTextures;

  // get texture parameters
  uint8_t texture_params;
  if(!decode_u8(buffer, &texture_params, callback, userdata))
  {
    free(texture);
    return NULL;
  }

  // decode width and height
  texture->width  = 1 << (((texture_params>>0) & 0x7) + 3);
  texture->height = 1 << (((texture_params>>3) & 0x7) + 3);

  // get texture format and mipmap levels
  if(!decode_u8(buffer, &texture->format,       callback, userdata)
  || !decode_u8(buffer, &texture->mipmapLevels, callback, userdata))
  {
    free(texture);
    return NULL;
  }

  // get subtextures
  for(size_t i = 0; i < numSubTextures; ++i)
  {
    if(!decode_subtexture(buffer, &texture->subTextures[i], callback, userdata))
    {
      free(texture);
      return NULL;
    }
  }

  // calculate texture size
  size_t texsize = calc_tex_size(texture->format, texture->width * texture->height);
  if(!texsize)
  {
    free(texture);
    return NULL;
  }

  // create C3D texture
  C3D_TexInitParams params;
  params.width    = texture->width;
  params.height   = texture->height;
  params.maxLevel = texture->mipmapLevels;
  params.format   = texture->format;
  params.type     = (texture_params >> 6) & 1;
  params.onVram   = vram;
  if(!C3D_TexInitWithParams(tex, texcube, params))
  {
    free(texture);
    return NULL;
  }

  // get texture size, including mipmaps
  size_t base_texsize = texsize = C3D_TexCalcTotalSize(texsize, texture->mipmapLevels);

  // if this is a cubemap/skybox, there are 6 textures
  if(params.type == GPU_TEX_CUBE_MAP)
    texsize *= 6;

  if(vram)
  {
    // allocate staging buffer
    void *texdata = linearAlloc(texsize);
    if(!texdata)
    {
      C3D_TexDelete(tex);
      free(texture);
      return NULL;
    }

    // decompress into linear memory for vram upload
    if(!Tex3DS_Decompress(buffer, texdata, texsize, callback, userdata))
    {
      linearFree(texdata);
      C3D_TexDelete(tex);
      free(texture);
      return NULL;
    }

    // flush linear memory to prepare dma to vram
    GSPGPU_FlushDataCache(texdata, texsize);

    size_t texcount = 1;
    if(params.type == GPU_TEX_CUBE_MAP)
      texcount = 6;

    // upload texture(s) to vram
    for(size_t i = 0; i < texcount; ++i)
      C3D_TexLoadImage(tex, (const char*)texdata + i*base_texsize, i, -1);

    linearFree(texdata);
  }
  else if(params.type == GPU_TEX_CUBE_MAP)
  {
    Tex3DS_IOVec iov[6];

    // setup iov
    for(size_t i = 0; i < 6; ++i)
    {
      u32 size;
      iov[i].data = C3D_TexCubeGetImagePtr(tex, i, -1, &size);
      iov[i].size = size;
    }

    // decompress into texture memory
    if(!Tex3DS_DecompressV(buffer, iov, 6, callback, userdata))
    {
      C3D_TexDelete(tex);
      free(texture);
      return NULL;
    }
  }
  else
  {
    u32  size;
    void *data = C3D_Tex2DGetImagePtr(tex, -1, &size);

    // decompress into texture memory
    if(!Tex3DS_Decompress(buffer, data, size, callback, userdata))
    {
      C3D_TexDelete(tex);
      free(texture);
      return NULL;
    }
  }

  return texture;
}

Tex3DS_Texture
Tex3DS_TextureImport(const void *input, C3D_Tex *tex, C3D_TexCube *texcube, bool vram)
{
  Tex3DS_Buffer buffer;
  buffer.data = (void*)input;
  buffer.size = SIZE_MAX;
  buffer.pos  = 0;

  return texture_import(&buffer, tex, texcube, vram, NULL, NULL);
}

Tex3DS_Texture
Tex3DS_TextureImportCallback(C3D_Tex *tex, C3D_TexCube *texcube, bool vram,
                             Tex3DS_DataCallback callback, void *userdata)
{
  Tex3DS_Buffer buffer;
  if(!Tex3DS_BufferInit(&buffer, 1024))
    return NULL;

  Tex3DS_Texture texture = texture_import(&buffer, tex, texcube, vram,
                                          callback, userdata);

  Tex3DS_BufferDestroy(&buffer);
  return texture;
}

static ssize_t
callback_fd(void *userdata, void *buffer, size_t size)
{
  int fd = *(int*)userdata;

  return read(fd, buffer, size);
}

Tex3DS_Texture
Tex3DS_TextureImportFD(int fd, C3D_Tex *tex, C3D_TexCube *texcube, bool vram)
{
  Tex3DS_Buffer buffer;
  if(!Tex3DS_BufferInit(&buffer, 1024))
    return NULL;

  off_t offset = lseek(fd, 0, SEEK_CUR);
  if(offset == (off_t)-1)
  {
    Tex3DS_BufferDestroy(&buffer);
    return NULL;
  }

  Tex3DS_Texture texture = texture_import(&buffer, tex, texcube, vram,
                                          callback_fd, &fd);

  offset += buffer.total;
  if(texture && lseek(fd, offset, SEEK_SET) != 0)
  {
    Tex3DS_TextureFree(texture);
    texture = NULL;
  }

  Tex3DS_BufferDestroy(&buffer);
  return texture;
}

static ssize_t
callback_stdio(void *userdata, void *buffer, size_t size)
{
  FILE *fp = (FILE*)userdata;

  return fread(buffer, 1, size, fp);
}

Tex3DS_Texture
Tex3DS_TextureImportStdio(FILE *fp, C3D_Tex *tex, C3D_TexCube *texcube, bool vram)
{
  Tex3DS_Buffer buffer;
  if(!Tex3DS_BufferInit(&buffer, 1024))
    return NULL;

  off_t offset = ftello(fp);
  if(offset == (off_t)-1)
  {
    Tex3DS_BufferDestroy(&buffer);
    return NULL;
  }

  Tex3DS_Texture texture = texture_import(&buffer, tex, texcube, vram,
                                          callback_stdio, fp);

  offset += buffer.total;
  if(texture && fseeko(fp, offset, SEEK_SET) != 0)
  {
    Tex3DS_TextureFree(texture);
    texture = NULL;
  }

  Tex3DS_BufferDestroy(&buffer);
  return texture;
}

const Tex3DS_SubTexture*
Tex3DS_GetSubTexture(const Tex3DS_Texture texture, size_t index)
{
  if(index < texture->numSubTextures)
    return &texture->subTextures[index];
  return NULL;
}

void
Tex3DS_TextureFree(Tex3DS_Texture texture)
{
  free(texture);
}
