#include "ringbuffer.h"
#include <CppUTest/CommandLineTestRunner.h>
#include <CppUTest/MemoryLeakDetectorMallocMacros.h>
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

TEST_GROUP(Ringbuffer_test)
{
   void setup()
   {
      // Init stuff
   }

   void teardown()
   {
      // Uninit stuff
   }
};


TEST(Ringbuffer_test, pow2) {
    for (int i = 0; i < 100; i++) {
        CHECK(i <= nextPow2(i));
    }
}

TEST(Ringbuffer_test, init1) {
    RINGBUFFER **ring = NULL;

    ring = initRingbuffer(0, 0, 0);
    POINTERS_EQUAL(ring, NULL);
}

TEST(Ringbuffer_test, init2) {
    RINGBUFFER **ring = NULL;

    ring = initRingbuffer(10, nextPow2(2), sizeof(int));
    CHECK(ring != NULL);
}

TEST(Ringbuffer_test, clear) {
    RINGBUFFER **ring = NULL;
    int tr = 10;

    ring = initRingbuffer(1, nextPow2(8), sizeof(int));
    push_ringbuffer(ring[0], &tr);
    clear_ringbuffer(ring[0]);
    CHECK_EQUAL(0, ring[0]->dataNum);
    CHECK_EQUAL(0, ring[0]->top);
    CHECK_EQUAL(0, ring[0]->bottom);
}

TEST(Ringbuffer_test, free) {
    RINGBUFFER **ring = NULL;

    ring = initRingbuffer(1, nextPow2(8), sizeof(int));
    free_ringbuffer(&ring[0]);
    CHECK_EQUAL((RINGBUFFER *)NULL, ring[0]);
}

TEST(Ringbuffer_test, push1) {
    RINGBUFFER **ring = NULL;
    int tr;

    ring = initRingbuffer(10, nextPow2(8), sizeof(int));
    tr = 10;
    for (int i = 0; i < nextPow2(8); i++) {
        tr = i + 10;
        push_ringbuffer(ring[0], &tr);
        CHECK_EQUAL(i + 10, ((int *)(ring[0]->buffer))[i]);
        CHECK_EQUAL(ring[0]->top, (i + 1)%8);
        CHECK_EQUAL(ring[0]->bottom, 0);
        CHECK_EQUAL(*((int *)nthPinter_ringbuffer(ring[0], 0)), tr);
    }
    CHECK_EQUAL(10, ((int *)(ring[0]->buffer))[0]);
    CHECK_EQUAL(11, ((int *)(ring[0]->buffer))[1]);
    CHECK_EQUAL(12, ((int *)(ring[0]->buffer))[2]);
    CHECK_EQUAL(13, ((int *)(ring[0]->buffer))[3]);
    CHECK_EQUAL(14, ((int *)(ring[0]->buffer))[4]);
    CHECK_EQUAL(15, ((int *)(ring[0]->buffer))[5]);
    CHECK_EQUAL(16, ((int *)(ring[0]->buffer))[6]);
    CHECK_EQUAL(17, ((int *)(ring[0]->buffer))[7]);
    
    CHECK_EQUAL(*((int *)nthPinter_ringbuffer(ring[0], 0)), 17);
    CHECK_EQUAL(*((int *)nthPinter_ringbuffer(ring[0], 1)), 16);
    CHECK_EQUAL(*((int *)nthPinter_ringbuffer(ring[0], 2)), 15);
    CHECK_EQUAL(*((int *)nthPinter_ringbuffer(ring[0], 3)), 14);
    CHECK_EQUAL(*((int *)nthPinter_ringbuffer(ring[0], 4)), 13);
    CHECK_EQUAL(*((int *)nthPinter_ringbuffer(ring[0], 5)), 12);
    CHECK_EQUAL(*((int *)nthPinter_ringbuffer(ring[0], 6)), 11);
    CHECK_EQUAL(*((int *)nthPinter_ringbuffer(ring[0], 7)), 10);
    // round up
    tr = 1000;
    push_ringbuffer(ring[0], &tr);

    CHECK_EQUAL(ring[0]->top, 1);
    CHECK_EQUAL(ring[0]->bottom, 1);
    CHECK_EQUAL(ring[0]->dataNum, nextPow2(8));

    CHECK_EQUAL(1000, ((int *)(ring[0]->buffer))[0]);
    CHECK_EQUAL(11, ((int *)(ring[0]->buffer))[1]);
    CHECK_EQUAL(12, ((int *)(ring[0]->buffer))[2]);
    CHECK_EQUAL(13, ((int *)(ring[0]->buffer))[3]);
    CHECK_EQUAL(14, ((int *)(ring[0]->buffer))[4]);
    CHECK_EQUAL(15, ((int *)(ring[0]->buffer))[5]);
    CHECK_EQUAL(16, ((int *)(ring[0]->buffer))[6]);
    CHECK_EQUAL(17, ((int *)(ring[0]->buffer))[7]);
}

