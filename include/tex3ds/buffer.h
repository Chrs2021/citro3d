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
/** @file buffer.h
 *  @brief Buffer routines
 */
#pragma once
#include "types.h"

/** @brief Initialize buffer object
 *  @param[in] buffer Tex3DS buffer object
 *  @param[in] size   Buffer size limit
 *  @returns Whether succeeded
 */
bool Tex3DS_BufferInit(Tex3DS_Buffer *buffer, size_t size);

/** @brief Destroy buffer object
 *  @param[in] buffer Tex3DS buffer object
 */
void Tex3DS_BufferDestroy(Tex3DS_Buffer *buffer);

/** @brief Read from buffer object
 *  @param[in]  buffer   Tex3DS buffer object
 *  @param[out] dest     Output buffer
 *  @param[in]  size     Amount to read
 *  @param[in]  callback Data callback
 *  @param[in]  userdata User data passed to callback
 *  @returns Whether succeeded
 */
bool Tex3DS_BufferRead(Tex3DS_Buffer *buffer, void *dest, size_t size,
                       Tex3DS_DataCallback callback, void *userdata);

/** @brief Read a byte from a buffer object
 *  @param[in]  buffer   Tex3DS buffer object
 *  @param[out] dest     Output buffer
 *  @param[in]  callback Data callback
 *  @param[in]  userdata User data passed to callback
 *  @returns Whether succeeded
 */
static inline bool
Tex3DS_BufferGet(Tex3DS_Buffer *buffer, uint8_t *dest,
                 Tex3DS_DataCallback callback, void *userdata)
{
  // fast-path; just read the byte if we have it
  if(buffer->pos < buffer->size)
  {
    *dest = buffer->data[buffer->pos++];
    ++buffer->total;
    return true;
  }

  // grab the byte with read-ahead
  return Tex3DS_BufferRead(buffer, dest, sizeof(*dest), callback, userdata);
}
