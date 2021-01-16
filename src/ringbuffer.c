#include "ringbuffer.h"
#include <stdlib.h>
#include <string.h>

int nextPow2(unsigned int v)
{
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;
}

void initRingbuffer(RINGBUFFER ***ring, int max_ringNum, int max_ringbufferItemNum, size_t itemSize)
{
    int i;
    
    if (max_ringNum != 0) {
        // realloc ringbuffer
        *ring = (RINGBUFFER **)calloc(sizeof(RINGBUFFER *), max_ringNum);
        for (i = 0; i < max_ringNum; i++)
        {
            (*ring)[i] = (RINGBUFFER *)calloc(sizeof(RINGBUFFER), 1);
            (*ring)[i]->itemNumber = max_ringbufferItemNum;
            (*ring)[i]->itemSize = itemSize;
            (*ring)[i]->top =  (*ring)[i]->bottom = 0;
            (*ring)[i]->buffer = (void *)calloc(max_ringbufferItemNum , itemSize);
        }
    }
}

void clear_ringbuffer(RINGBUFFER *ring)
{
    ring->top = ring->bottom = ring->dataNum = 0;
}

void free_ringbuffer(RINGBUFFER *ring)
{
    free(ring->buffer);
    free(ring);
    ring = (RINGBUFFER *)NULL;
}

/**
 * @brief          push data to ringbuffer
 * @fn             push_ringbuffer
 * @param          (RINGBUFFER *ring) ring buffer
 * @param          (void *data) data
 * @param          (size_t size) write data length.
 * @return         error code.
 */
int push_ringbuffer(RINGBUFFER *ring, void *data)
{
    int pos = 0;

    if ((ring == (RINGBUFFER *)NULL) || (ring->buffer == (void *)NULL))
    {
        return -2;
    }
    pos = (ring->top % ring->itemNumber) * ring->itemSize;
    // TODO
    if ((pos == ring->bottom) && (ring->dataNum == ring->itemNumber)) {
        // update bottom
        ring->bottom = (ring->bottom + 1) % ring->itemNumber;
    }
    memcpy((void *)((char *)ring->buffer + pos), data, ring->itemSize);
    // update top
    ring->top = (ring->top + 1) % ring->itemNumber;
    ring->dataNum = ((ring->dataNum + 1) > ring->itemNumber) ? ring->dataNum: (ring->dataNum + 1);
    return 0;
}


int pop_ringbuffer(RINGBUFFER *ring, void *data)
{
    int pos;
    
    if (ring->dataNum == 0)
        return 0;               // empty
    if (data == (void *)NULL)
        return -1;              // illeagal argument
    pos = ((ring->bottom + (ring->dataNum - 1)) % ring->itemNumber) * ring->itemSize;
    memcpy(data, (void *)((char *)ring->buffer + pos), ring->itemSize);
    ring->dataNum--;
    ring->top = (ring->bottom + ring->dataNum) % ring->itemNumber;
    return 0;
}


// This function return a pointer to Nth item.
void *nthPinter_ringbuffer(RINGBUFFER *ring,int number)
{
    int pos;
    if (number >= ring->dataNum)
        return (void *)NULL;              // invalid arguemnt
    pos = ((ring->bottom + (ring->dataNum - number - 1)) % ring->itemNumber) * ring->itemSize;
    return (void *)((char *)ring->buffer + pos);
}
