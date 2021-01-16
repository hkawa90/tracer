#include "../src/ringbuffer.h"
#include <gtest/gtest.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
extern "C" {
    extern int isExistFile(const char *path);
}

TEST(finstrument_test, isExistFile) {
    EXPECT_EQ(1, isExistFile("/bin/sh"));
    EXPECT_EQ(0, isExistFile("/bin"));
    EXPECT_EQ(0, isExistFile(""));
    EXPECT_EQ(0, isExistFile("./hoge"));
}
