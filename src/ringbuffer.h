#ifndef RINGBUFFER_H_
#define RINGBUFFER_H_

#include <stddef.h>
#ifdef __cplusplus
extern "C"
{
#endif

typedef struct ringbuffer {
    int itemNumber;             /* number of item */
    int itemSize;   /* size of item */
    int top;        /* top(next storing) position */
    int bottom;     /* bottom position */
    int dataNum;    /* number of data */
    void *buffer;   /* store data body */
} RINGBUFFER;


void initRingbuffer(RINGBUFFER ***ring, int max_ringNum, int max_ringbufferItemNum, size_t itemSize) __attribute__((no_instrument_function));
void clear_ringbuffer(RINGBUFFER *ring) __attribute__((no_instrument_function));
void free_ringbuffer(RINGBUFFER *ring) __attribute__((no_instrument_function));
int push_ringbuffer(RINGBUFFER *ring, void *data) __attribute__((no_instrument_function));
int pop_ringbuffer(RINGBUFFER *ring, void *data) __attribute__((no_instrument_function));
void *nthPinter_ringbuffer(RINGBUFFER *ring,int number) __attribute__((no_instrument_function));
#ifdef __cplusplus
}
#endif

#endif  /* RINGBUFFER_H_ */
