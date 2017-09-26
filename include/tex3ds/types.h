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
/** @file types.h
 *  @brief Tex3DS types
 */
#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

/** @brief Buffer */
typedef struct Tex3DS_Buffer
{
  uint8_t *data; ///< Pointer to buffer
  size_t  limit; ///< Max buffer size
  size_t  size;  ///< Current buffer size
  size_t  pos;   ///< Buffer position
  size_t  total; ///< Total bytes consumed
} Tex3DS_Buffer;

/** @brief I/O vector */
typedef struct Tex3DS_IOVec
{
  void   *data; ///< I/O buffer
  size_t size;  ///< Buffer size
} Tex3DS_IOVec;

/** @brief Sub-Texture
 *  @note If top > bottom, the sub-texture is rotated 1/4 revolution counter-clockwise
 */
typedef struct Tex3DS_SubTexture
{
  uint16_t width;   ///< Sub-texture width (pixels)
  uint16_t height;  ///< Sub-texture height (pixels)
  float    left;    ///< Left u-coordinate
  float    top;     ///< Top v-coordinate
  float    right;   ///< Right u-coordinate
  float    bottom;  ///< Bottom v-coordinate
} Tex3DS_SubTexture;

/** @brief Texture */
typedef struct Tex3DS_Texture_s *Tex3DS_Texture;

/** @brief Data callback */
typedef ssize_t (*Tex3DS_DataCallback)(void *userdata, void *buffer, size_t size);
