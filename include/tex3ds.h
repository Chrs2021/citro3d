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
/** @file tex3ds.h
 *  @brief tex3ds support
 */
#pragma once
#include "tex3ds/buffer.h"
#ifdef CITRO3D_BUILD
#include "c3d/texture.h"
#else
#include <citro3d.h>
#endif

#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C"
{
#endif

/** @brief Import Tex3DS texture
 *  @param[in]  input   Input data
 *  @param[out] tex     Citro3D texture
 *  @param[out] texcube Citro3D texcube
 *  @param[in]  vram    Whether to store textures in VRAM
 *  @returns Tex3DS texture
 */
Tex3DS_Texture Tex3DS_TextureImport(const void *input, C3D_Tex *tex, C3D_TexCube *texcube, bool vram);

/** @brief Import Tex3DS texture
 *
 *  @description
 *  For example, use this if you want to import from a large file without
 *  pulling the entire file into memory.
 *
 *  @param[out] tex      Citro3D texture
 *  @param[out] texcube  Citro3D texcube
 *  @param[in]  vram     Whether to store textures in VRAM
 *  @param[in]  callback Data callback
 *  @param[in]  userdata User data passed to callback
 *  @returns Tex3DS texture
 */
Tex3DS_Texture Tex3DS_TextureImportCallback(C3D_Tex *tex, C3D_TexCube *texcube, bool vram,
                                            Tex3DS_DataCallback callback, void *userdata);

/** @brief Import Tex3DS texture
 *
 *  Starts reading at the current file descriptor's offset. The file
 *  descriptor's position is left at the end of the decoded data. On error, the
 *  file descriptor's position is indeterminate.
 *
 *  @param[in]  fd       Open file descriptor
 *  @param[out] tex      Citro3D texture
 *  @param[out] texcube  Citro3D texcube
 *  @param[in]  vram     Whether to store textures in VRAM
 *  @returns Tex3DS texture
 */
Tex3DS_Texture Tex3DS_TextureImportFD(int fd, C3D_Tex *tex, C3D_TexCube *texcube, bool vram);

/** @brief Import Tex3DS texture
 *
 *  Starts reading at the current file stream's offset. The file stream's
 *  position is left at the end of the decoded data. On error, the file
 *  stream's position is indeterminate.
 *
 *  @param[in]  fp       Open file stream
 *  @param[out] tex      Citro3D texture
 *  @param[out] texcube  Citro3D texcube
 *  @param[in]  vram     Whether to store textures in VRAM
 *  @returns Tex3DS texture
 */
Tex3DS_Texture Tex3DS_TextureImportStdio(FILE *fp, C3D_Tex *tex, C3D_TexCube *texcube, bool vram);

/** @brief Get sub-texture
 *  @param[in] texture Tex3DS texture
 *  @param[in] index   Sub-texture index
 *  @returns Sub-texture info
 */
const Tex3DS_SubTexture* Tex3DS_GetSubTexture(const Tex3DS_Texture texture, size_t index);

/** @brief Check if sub-texture is rotated
 *  @param[in] subtex Sub-texture to check
 *  @returns whether sub-texture is rotated
 */
static inline bool
Tex3DS_SubTextureRotated(const Tex3DS_SubTexture *subtex)
{
  return subtex->top < subtex->bottom;
}

/** @brief Get bottom-left texcoords
 *  @param[in]  subtex Sub-texture
 *  @param[out] u      u-coordinate
 *  @param[out] v      v-coordinate
 */
static inline void
Tex3DS_SubTextureBottomLeft(const Tex3DS_SubTexture *subtex, float *u, float *v)
{
  if(!Tex3DS_SubTextureRotated(subtex))
  {
    *u = subtex->left;
    *v = subtex->bottom;
  }
  else
  {
    *u = subtex->bottom;
    *v = subtex->left;
  }
}

/** @brief Get bottom-right texcoords
 *  @param[in]  subtex Sub-texture
 *  @param[out] u      u-coordinate
 *  @param[out] v      v-coordinate
 */
static inline void
Tex3DS_SubTextureBottomRight(const Tex3DS_SubTexture *subtex, float *u, float *v)
{
  if(!Tex3DS_SubTextureRotated(subtex))
  {
    *u = subtex->right;
    *v = subtex->bottom;
  }
  else
  {
    *u = subtex->bottom;
    *v = subtex->right;
  }
}

/** @brief Get top-left texcoords
 *  @param[in]  subtex Sub-texture
 *  @param[out] u      u-coordinate
 *  @param[out] v      v-coordinate
 */
static inline void
Tex3DS_SubTextureTopLeft(const Tex3DS_SubTexture *subtex, float *u, float *v)
{
  if(!Tex3DS_SubTextureRotated(subtex))
  {
    *u = subtex->left;
    *v = subtex->top;
  }
  else
  {
    *u = subtex->top;
    *v = subtex->left;
  }
}

/** @brief Get top-right texcoords
 *  @param[in]  subtex Sub-texture
 *  @param[out] u      u-coordinate
 *  @param[out] v      v-coordinate
 */
static inline void
Tex3DS_SubTextureTopRight(const Tex3DS_SubTexture *subtex, float *u, float *v)
{
  if(!Tex3DS_SubTextureRotated(subtex))
  {
    *u = subtex->right;
    *v = subtex->top;
  }
  else
  {
    *u = subtex->top;
    *v = subtex->right;
  }
}

/** @brief Free Tex3DS texture
 *  @param[in] texture Tex3DS texture to free
 */
void Tex3DS_TextureFree(Tex3DS_Texture texture);

/** @brief Decompress Tex3DS data
 *  @param[in] buffer   Tex3DS buffer object
 *  @param[in] output   Output buffer
 *  @param[in] outsize  Output size limit
 *  @param[in] callback Data callback
 *  @param[in] userdata User data passed to callback
 */
bool Tex3DS_Decompress(Tex3DS_Buffer *buffer, void *output, size_t outsize,
                       Tex3DS_DataCallback callback, void *userdata);

/** @brief Decompress Tex3DS data
 *  @param[in] buffer   Tex3DS buffer object
 *  @param[in] iov      Output vector
 *  @param[in] iovcnt   Number of buffers
 *  @param[in] callback Data callback
 *  @param[in] userdata User data passed to callback
 */
bool Tex3DS_DecompressV(Tex3DS_Buffer *buffer, const Tex3DS_IOVec *iov,
                        size_t iovcnt, Tex3DS_DataCallback callback,
                        void *userdata);

#ifdef __cplusplus
}
#endif
