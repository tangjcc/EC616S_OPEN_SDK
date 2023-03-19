/****************************************************************************
 *
 * Copy right:   2017-, Copyrigths of EigenComm Ltd.
 * File name:    ring_buffer.c
 * Description:  ring buffer source file
 * History:      Rev1.0   2018-08-06
 *
 ****************************************************************************/

#include "ring_buffer.h"

/**
  \fn          int initRingBuffer( ring_buffer_t* ring_buffer, uint8_t* buffer, uint32_t size )
  \brief       ring buffer init function
  \returns     int
*/
int initRingBuffer( ring_buffer_t* ring_buffer, uint8_t* buffer, uint32_t size )
{
    if (ring_buffer)
    {
        ring_buffer->buffer = (uint8_t*)buffer;
        ring_buffer->size   = size;
        ring_buffer->head   = 0;
        ring_buffer->tail   = 0;
        return 0;
    }
    else
        return -1;
}

/**
  \fn          int deinitRingBuffer( ring_buffer_t* ring_buffer )
  \brief       ring buffer de-init function
  \returns     int
*/
int deinitRingBuffer( ring_buffer_t* ring_buffer )
{
    ring_buffer->buffer = (void *)0;
    ring_buffer->size   = 0;
    ring_buffer->head   = 0;
    ring_buffer->tail   = 0;

    return 0;
}

/**
  \fn          uint32_t getFreeRingBuffer( ring_buffer_t* ring_buffer )
  \brief       get ring buffer free size function
  \returns     value of free ring buffer size
*/
uint32_t getFreeRingBuffer( ring_buffer_t* ring_buffer )
{
    uint32_t tail_to_end = ring_buffer->size-1 - ring_buffer->tail;
    return ((tail_to_end + ring_buffer->head) % ring_buffer->size);
}

/**
  \fn          uint32_t getRingBufferUsedSpace( ring_buffer_t* ring_buffer )
  \brief       get ring buffer used size function
  \returns     value of used ring buffer size
*/
uint32_t getRingBufferUsedSpace( ring_buffer_t* ring_buffer )
{
    uint32_t head_to_end = ring_buffer->size - ring_buffer->head;
    return ((head_to_end + ring_buffer->tail) % ring_buffer->size);
}

/**
  \fn          int getRingBufferData( ring_buffer_t* ring_buffer, uint8_t** data, uint32_t* contiguous_bytes )
  \brief       get ring buffer data function
  \returns     int
*/
int getRingBufferData( ring_buffer_t* ring_buffer, uint8_t** data, uint32_t* contiguous_bytes )
{
    uint32_t head_to_end = ring_buffer->size - ring_buffer->head;

    *data = &ring_buffer->buffer[ring_buffer->head];
    *contiguous_bytes = MIN(head_to_end, (head_to_end + ring_buffer->tail) % ring_buffer->size);

    return 0;
}

static int consumeRingBuffer( ring_buffer_t* ring_buffer, uint32_t bytes_consumed )
{
    ring_buffer->head = (ring_buffer->head + bytes_consumed) % ring_buffer->size;
    return 0;
}

/**
  \fn          uint32_t pushRingBuffer( ring_buffer_t* ring_buffer, const uint8_t* data, uint32_t data_length )
  \brief       write data to ring buffer function
  \returns     amount of pushed data
*/
uint32_t pushRingBuffer( ring_buffer_t* ring_buffer, const uint8_t* data, uint32_t data_length )
{
    uint32_t tail_to_end = ring_buffer->size - 1 - ring_buffer->tail;

    // Calculate the maximum amount we can copy
    uint32_t amount_to_copy = MIN(data_length, (tail_to_end + ring_buffer->head) % ring_buffer->size);

    // Fix the bug when tail is at the end of buffer
    tail_to_end++;

    // Copy as much as we can until we fall off the end of the buffer
    memcpy(&ring_buffer->buffer[ring_buffer->tail], data, MIN(amount_to_copy, tail_to_end));

    // Check if we have more to copy to the front of the buffer
    if ( tail_to_end < amount_to_copy )
    {
        memcpy( &ring_buffer->buffer[ 0 ], data + tail_to_end, amount_to_copy - tail_to_end );
    }

    // Update the tail
    ring_buffer->tail = (ring_buffer->tail + amount_to_copy) % ring_buffer->size;

    return amount_to_copy;
}

/**
  \fn          int popRingBuffer( ring_buffer_t* ring_buffer, uint8_t* data, uint32_t data_length, uint32_t* number_of_bytes_read )
  \brief       read data from ring buffer function
  \returns     amount of poped data
*/
int popRingBuffer( ring_buffer_t* ring_buffer, uint8_t* data, uint32_t data_length, uint32_t* number_of_bytes_read )
{
    uint32_t i, used_space, max_bytes_to_read, head;

    head = ring_buffer->head;

    used_space = getRingBufferUsedSpace(ring_buffer);

    max_bytes_to_read = MIN(data_length, used_space);

    if ( max_bytes_to_read != 0 )
    {
        for ( i = 0; i != max_bytes_to_read; i++, ( head = ( head + 1 ) % ring_buffer->size ) )
        {
            data[ i ] = ring_buffer->buffer[ head ];
        }

        consumeRingBuffer( ring_buffer, max_bytes_to_read );
    }

    *number_of_bytes_read = max_bytes_to_read;

    return 0;
}

/**
  \fn          uint8_t isRingBufferFull(ring_buffer_t *ring_buffer)
  \brief       check if ring buffer is full function
  \returns     uint8_t
*/
uint8_t isRingBufferFull(ring_buffer_t *ring_buffer)
{
    if (getRingBufferUsedSpace(ring_buffer) >= ring_buffer->size - 1)
        return 1;
    else
        return 0;
}

/**
  \fn          uint32_t getRingBufferTotalSize(ring_buffer_t *ring_buffer)
  \brief       get ring buffer total size function
  \returns     value of ring buffer total size
*/
uint32_t getRingBufferTotalSize(ring_buffer_t *ring_buffer)
{
    return ring_buffer->size - 1;
}

