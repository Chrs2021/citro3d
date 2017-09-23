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
/** @file buffer.c
 *  @brief Buffer routines
 */
#include "tex3ds.h"
#include <stdlib.h>
#include <string.h>

bool
Tex3DS_BufferInit(Tex3DS_Buffer *buffer, size_t size)
{
  buffer->data = (uint8_t*)malloc(size);
  if(!buffer->data)
    return false;

  buffer->limit = size;
  buffer->size  = 0;
  buffer->pos   = 0;

  return true;
}

void
Tex3DS_BufferDestroy(Tex3DS_Buffer *buffer)
{
  free(buffer->data);
}

bool
Tex3DS_BufferRead(Tex3DS_Buffer *buffer, void *dest, size_t size,
                  Tex3DS_DataCallback callback, void *userdata)
{
  while(size > 0)
  {
    // entire request is in our buffer
    if(size <= buffer->size - buffer->pos)
    {
      memcpy(dest, buffer->data + buffer->pos, size);
      buffer->pos += size;
      return true;
    }

    // copy partial data
    if(buffer->pos != buffer->size)
    {
      memcpy(dest, buffer->data + buffer->pos, buffer->size - buffer->pos);
      dest  = (uint8_t*)dest + (buffer->size - buffer->pos);
      size -= buffer->size - buffer->pos;
    }

    // fetch some more data
    buffer->size = buffer->pos = 0;
    ssize_t rc = callback(userdata, buffer->data, buffer->limit);
    if(rc <= 0)
      return false;

    buffer->size = rc;
  }

  return true;
}
