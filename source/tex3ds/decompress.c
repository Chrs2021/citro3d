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
/** @file decompress.c
 *  @brief Decompression routines
 */
#include "tex3ds.h"
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/*! I/O vector iterator */
typedef struct
{
  const Tex3DS_IOVec *iov; //!< I/O vector
  size_t             cnt;  //!< Number of buffers
  size_t             num;  //!< Current buffer number
  size_t             pos;  //!< Current buffer position
} iov_iter;

/*! Create I/O vector iterator
 *  @param[in] iov I/O vector
 *  @param[in] iovcnt Number of buffers
 *  @returns I/O vector iterator
 */
static inline
iov_iter iov_begin(const Tex3DS_IOVec *iov, size_t iovcnt)
{
  iov_iter it;
  it.iov = iov;
  it.cnt = iovcnt;
  it.num = 0;
  it.pos = 0;

  return it;
}

/*! Get I/O vector total size
 *  @param[in] iov I/O vector
 *  @param[in] iovcnt Number of buffers
 *  @returns Total buffer size
 */
static inline
size_t iov_size(const Tex3DS_IOVec *iov, size_t iovcnt)
{
  size_t size = 0;

  for(size_t i = 0; i < iovcnt; ++i)
  {
    assert(SIZE_MAX - size >= iov[i].size);
    size += iov[i].size;
  }

  return size;
}

/*! Get address for current iterator position
 *  @param[in] it I/O vector iterator
 *  @returns address for current iterator position
 */
static inline
uint8_t* iov_addr(const iov_iter *it)
{
  assert(it->num < it->cnt);
  assert(it->pos < it->iov[it->num].size);

  return (uint8_t*)it->iov[it->num].data + it->pos;
}

/*! Increment iterator by one position
 *  @param[in] it Iterator to increment
 */
static inline
void iov_increment(iov_iter *it)
{
  assert(it->num < it->cnt);
  assert(it->pos < it->iov[it->num].size);

  ++it->pos;

  if(it->pos == it->iov[it->num].size)
  {
    // advance to next buffer
    it->pos = 0;
    ++it->num;
  }
}

/*! Increment iterator by size
 *  @param[in] it   Iterator to increment
 *  @param[in] size Size to increment
 */
static inline
void iov_add(iov_iter *it, size_t size)
{
  while(true)
  {
    assert(it->num <= it->cnt);
    assert(it->iov[it->num].size > it->pos);

    if(it->iov[it->num].size - it->pos > size)
    {
      // position is within current buffer
      it->pos += size;
      return;
    }

    // advance to next buffer
    size -= it->iov[it->num].size - it->pos;
    ++it->num;
    it->pos = 0;
  }
}

/*! Decrement iterator by size
 *  @param[in] it   Iterator to decrement
 *  @param[in] size Size to decrement
 */
static inline
void iov_sub(iov_iter *it, size_t size)
{
  while(true)
  {
    if(it->pos >= size)
    {
      // position is within current buffer
      it->pos -= size;
      return;
    }

    // move to previous buffer
    size -= it->pos;
    assert(it->num > 0);
    --it->num;
    it->pos = it->iov[it->num].size;
  }
}

static bool
iov_read(Tex3DS_Buffer *buffer, iov_iter *out, size_t size,
         Tex3DS_DataCallback callback, void *userdata)
{
  while(size > 0)
  {
    assert(out->num < out->cnt);
    assert(out->pos < out->iov[out->num].size);

    size_t bytes = out->iov[out->num].size - out->pos;
    if(size < bytes)
      bytes = size;

    if(!Tex3DS_BufferRead(buffer, iov_addr(out), bytes, callback, userdata))
      return false;

    size -= bytes;
    iov_add(out, bytes);
  }

  return true;
}

static void
iov_memmove(iov_iter *out, iov_iter *in, size_t size)
{
  while(size > 0)
  {
    assert(out->num < out->cnt);
    assert(out->pos < out->iov[out->num].size);
    assert(in->num < in->cnt);
    assert(in->pos < in->iov[in->num].size);

    size_t bytes;
    uint8_t *outbuf = iov_addr(out);
    uint8_t *inbuf  = iov_addr(in);

    if(out->iov[out->num].size - out->pos < in->iov[in->num].size - in->pos)
      bytes = out->iov[out->num].size - out->pos;
    else
      bytes = in->iov[in->num].size - in->pos;

    if(size < bytes)
      bytes = size;

    size -= bytes;

    while(bytes-- > 0)
      *outbuf++ = *inbuf++;

    iov_add(out, bytes);
    iov_add(in, bytes);
  }
}

static void
iov_memset(iov_iter *out, char val, size_t size)
{
  while(size > 0)
  {
    assert(out->num < out->cnt);
    assert(out->pos < out->iov[out->num].size);

    size_t bytes = out->iov[out->num].size - out->pos;
    if(size < bytes)
      bytes = size;

    memset(iov_addr(out), val, bytes);

    size -= bytes;
    iov_add(out, bytes);
  }
}

/** @brief Decompress LZSS/LZ10
 *  @param[in] buffer   Tex3DS buffer object
 *  @param[in] iov      Output vector
 *  @param[in] iovcnt   Number of buffers
 *  @param[in] size     Output size limit
 *  @param[in] callback Data callback
 *  @param[in] userdata User data passed to callback
 *  @returns Whether succeeded
 */
static bool
lzss_decode(Tex3DS_Buffer *buffer, const Tex3DS_IOVec *iov, size_t iovcnt,
            size_t size, Tex3DS_DataCallback callback, void *userdata)
{
  iov_iter     out = iov_begin(iov, iovcnt);
  uint8_t      flags = 0;
  uint8_t      mask  = 0;
  unsigned int len;
  unsigned int disp;

  while(size > 0)
  {
    if(mask == 0)
    {
      // read in the flags data
      // from bit 7 to bit 0:
      //     0: raw byte
      //     1: compressed block
      if(!Tex3DS_BufferGet(buffer, &flags, callback, userdata))
        return false;
      mask = 0x80;
    }

    if(flags & mask) // compressed block
    {
      uint8_t displen[2];
      if(!Tex3DS_BufferGet(buffer, &displen[0], callback, userdata)
      || !Tex3DS_BufferGet(buffer, &displen[1], callback, userdata))
        return false;

      // disp: displacement
      // len:  length
      len  = ((displen[0] & 0xF0) >> 4) + 3;
      disp = displen[0] & 0x0F;
      disp = disp << 8 | displen[1];

      if(len > size)
        len = size;

      size -= len;

      iov_iter in = out;
      iov_sub(&in, disp+1);
      iov_memmove(&out, &in, size);
    }
    else // uncompressed block
    {
      // copy a raw byte from the input to the output
      if(!Tex3DS_BufferGet(buffer, iov_addr(&out), callback, userdata))
        return false;

      --size;
      iov_increment(&out);
    }

    mask >>= 1;
  }

  return true;
}

/** @brief Decompress LZ11
 *  @param[in] buffer   Tex3DS buffer object
 *  @param[in] iov      Output vector
 *  @param[in] iovcnt   Number of buffers
 *  @param[in] size     Output size limit
 *  @param[in] callback Data callback
 *  @param[in] userdata User data passed to callback
 *  @returns Whether succeeded
 */
static bool
lz11_decode(Tex3DS_Buffer *buffer, const Tex3DS_IOVec *iov, size_t iovcnt,
            size_t size, Tex3DS_DataCallback callback, void *userdata)
{
  iov_iter out = iov_begin(iov, iovcnt);
  int      i;
  uint8_t  flags;

  while(size > 0)
  {
    // read in the flags data
    // from bit 7 to bit 0, following blocks:
    //     0: raw byte
    //     1: compressed block
    if(!Tex3DS_BufferGet(buffer, &flags, callback, userdata))
      return false;

    for(i = 0; i < 8 && size > 0; i++, flags <<= 1)
    {
      if(flags & 0x80) // compressed block
      {
        uint8_t displen[4];
        if(!Tex3DS_BufferGet(buffer, &displen[0], callback, userdata))
          return false;

        size_t len;     // length
        size_t disp;    // displacement
        size_t pos = 0; // displen position

        switch(displen[pos] >> 4)
        {
          case 0: // extended block
            if(!Tex3DS_BufferGet(buffer, &displen[1], callback, userdata)
            || !Tex3DS_BufferGet(buffer, &displen[2], callback, userdata))
              return false;

            len   = displen[pos++] << 4;
            len  |= displen[pos] >> 4;
            len  += 0x11;
            break;

          case 1: // extra extended block
            if(!Tex3DS_BufferGet(buffer, &displen[1], callback, userdata)
            || !Tex3DS_BufferGet(buffer, &displen[2], callback, userdata)
            || !Tex3DS_BufferGet(buffer, &displen[3], callback, userdata))
              return false;

            len   = (displen[pos++] & 0x0F) << 12;
            len  |= (displen[pos++]) << 4;
            len  |= displen[pos] >> 4;
            len  += 0x111;
            break;

          default: // normal block
            if(!Tex3DS_BufferGet(buffer, &displen[1], callback, userdata))
              return false;
            len   = (displen[pos] >> 4) + 1;
            break;
        }

        disp  = (displen[pos++] & 0x0F) << 8;
        disp |= displen[pos];

        if(len > size)
          len = size;

        size -= len;

        iov_iter in = out;
        iov_sub(&in, disp+1);
        iov_memmove(&out, &in, size);
      }
      else // uncompressed block
      {
        // copy a raw byte from the input to the output
        if(!Tex3DS_BufferGet(buffer, iov_addr(&out), callback, userdata))
          return false;

        --size;
        iov_increment(&out);
      }
    }
  }

  return true;
}

/** @brief Decompress Huffman
 *  @param[in] buffer   Tex3DS buffer object
 *  @param[in] iov      Output vector
 *  @param[in] iovcnt   Number of buffers
 *  @param[in] size     Output size limit
 *  @param[in] callback Data callback
 *  @param[in] userdata User data passed to callback
 *  @returns Whether succeeded
 */
static bool
huff_decode(Tex3DS_Buffer *buffer, const Tex3DS_IOVec *iov, size_t iovcnt,
            size_t size, Tex3DS_DataCallback callback, void *userdata)
{
  uint8_t *tree = (uint8_t*)malloc(512);
  if(!tree)
    return false;

  // get tree size
  if(!Tex3DS_BufferRead(buffer, &tree[0], 1, callback, userdata))
  {
    free(tree);
    return false;
  }

  // read tree
  if(!Tex3DS_BufferRead(buffer, &tree[1], (((size_t)tree[0])+1)*2-1, callback, userdata))
  {
    free(tree);
    return false;
  }

  iov_iter     out = iov_begin(iov, iovcnt);
  const size_t bits = 8;
  uint32_t     word = 0;               // 32-bits of input bitstream
  uint32_t     mask = 0;               // which bit we are reading
  uint32_t     dataMask = (1<<bits)-1; // mask to apply to data
  size_t       node;                   // node in the huffman tree
  size_t       child;                  // child of a node
  uint32_t     offset;                 // offset from node to child

  // point to the root of the huffman tree
  node = 1;

  while(size > 0)
  {
    if(mask == 0) // we exhausted 32 bits
    {
      // reset the mask
      mask = 0x80000000;

      // read the next 32 bits
      uint8_t wordbuf[4];
      if(!Tex3DS_BufferGet(buffer, &wordbuf[0], callback, userdata)
      || !Tex3DS_BufferGet(buffer, &wordbuf[1], callback, userdata)
      || !Tex3DS_BufferGet(buffer, &wordbuf[2], callback, userdata)
      || !Tex3DS_BufferGet(buffer, &wordbuf[3], callback, userdata))
      {
        free(tree);
        return false;
      }

      word = (wordbuf[0] <<  0)
           | (wordbuf[1] <<  8)
           | (wordbuf[2] << 16)
           | (wordbuf[3] << 24);
    }

    // read the current node's offset value
    offset = tree[node] & 0x1F;

    child = (node & ~1) + offset*2 + 2;

    if(word & mask) // we read a 1
    {
      // point to the "right" child
      ++child;

      if(tree[node] & 0x40) // "right" child is a data node
      {
        // copy the child node into the output buffer and apply mask
        *iov_addr(&out) = tree[child] & dataMask;
        iov_increment(&out);
        --size;

        // start over at the root node
        node = 1;
      }
      else // traverse to the "right" child
        node = child;
    }
    else // we read a 0
    {
      // pointed to the "left" child

      if(tree[node] & 0x80) // "left" child is a data node
      {
        // copy the child node into the output buffer and apply mask
        *iov_addr(&out) = tree[child] & dataMask;
        iov_increment(&out);
        --size;

        // start over at the root node
        node = 1;
      }
      else // traverse to the "left" child
        node = child;
    }

    // shift to read next bit (read bit 31 to bit 0)
    mask >>= 1;
  }

  free(tree);
  return true;
}

/** @brief Decompress Run-length encoding
 *  @param[in] buffer   Tex3DS buffer object
 *  @param[in] iov      Output vector
 *  @param[in] iovcnt   Number of buffers
 *  @param[in] size     Output size limit
 *  @param[in] callback Data callback
 *  @param[in] userdata User data passed to callback
 *  @returns Whether succeeded
 */
static bool
rle_decode(Tex3DS_Buffer *buffer, const Tex3DS_IOVec *iov, size_t iovcnt,
           size_t size, Tex3DS_DataCallback callback, void *userdata)
{
  iov_iter out = iov_begin(iov, iovcnt);
  uint8_t  byte;
  size_t   len;

  while(size > 0)
  {
    // read in the data header
    if(!Tex3DS_BufferGet(buffer, &byte, callback, userdata))
      return false;

    if(byte & 0x80) // compressed block
    {
      // read the length of the run
      len = (byte & 0x7F) + 3;

      if(len > size)
        len = size;

      size -= len;

      // read in the byte used for the run
      if(!Tex3DS_BufferGet(buffer, &byte, callback, userdata))
        return false;

      // for len, copy byte into output
      iov_memset(&out, byte, len);
    }
    else // uncompressed block
    {
      // read the length of uncompressed bytes
      len = (byte & 0x7F) + 1;

      if(len > size)
        len = size;

      size -= len;

      // for len, copy from input to output
      if(!iov_read(buffer, &out, len, callback, userdata))
        return false;
    }
  }

  return true;
}

bool
Tex3DS_Decompress(Tex3DS_Buffer *buffer, void *output, size_t outsize,
                  Tex3DS_DataCallback callback, void *userdata)
{
  Tex3DS_IOVec iov;

  iov.data = output;
  iov.size = outsize;

  return Tex3DS_DecompressV(buffer, &iov, 1, callback, userdata);
}

bool
Tex3DS_DecompressV(Tex3DS_Buffer *buffer, const Tex3DS_IOVec *iov,
                   size_t iovcnt, Tex3DS_DataCallback callback,
                   void *userdata)
{
  if(iovcnt == 0)
    return false;

  uint8_t header[8];
  if(!Tex3DS_BufferRead(buffer, header, 4, callback, userdata))
    return false;

  uint8_t type = header[0];
  size_t  size = (header[1] <<  0)
               | (header[2] <<  8)
               | (header[3] << 16);

  if(type & 0x80)
  {
    type &= ~0x80;

    if(!Tex3DS_BufferRead(buffer, &header[4], 4, callback, userdata))
      return false;

    size |= header[4] << 24;
  }

  size_t iovsize = iov_size(iov, iovcnt);
  if(iovsize < size)
    size = iovsize;

  switch(type)
  {
    case 0x00:
    {
      iov_iter out = iov_begin(iov, iovcnt);
      return iov_read(buffer, &out, size, callback, userdata);
    }

    case 0x10:
      return lzss_decode(buffer, iov, iovcnt, size, callback, userdata);

    case 0x11:
      return lz11_decode(buffer, iov, iovcnt, size, callback, userdata);

    case 0x28:
      return huff_decode(buffer, iov, iovcnt, size, callback, userdata);

    case 0x30:
      return rle_decode(buffer, iov, iovcnt, size, callback, userdata);
  }

  return false;
}
