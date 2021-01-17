#include "finstrument.h"
#include "ringbuffer.h"
#include <CppUTest/CommandLineTestRunner.h>
#include <CppUTest/MemoryLeakDetectorMallocMacros.h>
#include <stdlib.h>
#include <string.h>

extern "C" {
    extern int isExistFile(const char *path);
    extern int changeTraceOption(TRACER_OPTION *tp);

}

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

TEST_GROUP(finstrument_test)
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


TEST(finstrument_test, isExistFile) {
    CHECK_EQUAL(1, isExistFile("/bin/sh"));
    CHECK_EQUAL(0, isExistFile("/bin"));
    CHECK_EQUAL(0, isExistFile(""));
    CHECK_EQUAL(0, isExistFile("./hoge"));
}

TEST(finstrument_test, changeTraceOption) {
    RINGBUFFER **ring = NULL;
    TRACER_OPTION tp;

    ring = initRingbuffer(1, nextPow2(8), sizeof(int));
    changeTraceOption(&tp);
}
