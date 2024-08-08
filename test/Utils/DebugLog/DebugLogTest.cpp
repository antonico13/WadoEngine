#include <gtest/gtest.h>

#include "System.h"
#include "FiberSystem.h"
#include "DebugLog.h"

#include <cstdlib>
#include <string>
#include <iostream>
#include <vector>

#include <cstdarg>

static const size_t BUFFER_SIZE = 1024;

static uint64_t writeToBuffer(char *buffer, const char* format, ...) {
    std::va_list args;
    va_start(args, format);
    uint64_t writtenCount = Wado::DebugLog::DebugMessageFormatter(buffer, format, args);
    va_end(args);
    return writtenCount;
};

TEST(DebugMessageFormatterTest, WritesNothingForPureString) {
    char *testBuffer = static_cast<char *>(malloc(BUFFER_SIZE));
    ASSERT_EQ(sizeof("Hello World!"), writeToBuffer(testBuffer, "Hello World!")) << "Expected nothing to be written to buffer with no format strings";
    free(testBuffer);
};

TEST(DebugMessageFormatterTest, PureStringIsKeptIntact) {
    char *testBuffer = static_cast<char *>(malloc(BUFFER_SIZE));
    writeToBuffer(testBuffer, "Hello World!");
    ASSERT_EQ(strcmp("Hello World!", testBuffer), 0) << "Expected \"Hello World!\", but got: " << testBuffer;
    free(testBuffer);
};

TEST(DebugMessageFormatterTest, FormatterIsReplacedCorrectly) {
    char *testBuffer = static_cast<char *>(malloc(BUFFER_SIZE));
    writeToBuffer(testBuffer, "Hello World!%%");
    ASSERT_EQ(strcmp("Hello World!%", testBuffer), 0) << "Expected \"Hello World!%\", but got: " << testBuffer;
    free(testBuffer);
};

TEST(DebugMessageFormatterTest, SpecialCharactersAreKeptIntact) {
    char *testBuffer = static_cast<char *>(malloc(BUFFER_SIZE));
    writeToBuffer(testBuffer, "Hello World!\n");
    ASSERT_EQ(strcmp("Hello World!\n", testBuffer), 0) << "Expected \"Hello World!\n\", but got: " << testBuffer;
    writeToBuffer(testBuffer, "Hello World!\n\t");
    ASSERT_EQ(strcmp("Hello World!\n\t", testBuffer), 0) << "Expected \"Hello World!\n\t\", but got: " << testBuffer;
    writeToBuffer(testBuffer, "Hello World!\n\t");
    ASSERT_EQ(strcmp("Hello World!\n\t", testBuffer), 0) << "Expected \"Hello World!\n\t\", but got: " << testBuffer;
    writeToBuffer(testBuffer, "Hello World!\'\"");
    ASSERT_EQ(strcmp("Hello World!\'\"", testBuffer), 0) << "Expected \"Hello World!\'\"\", but got: " << testBuffer;
    free(testBuffer);
};

TEST(DebugMessageFormatterTest, WritesCorrectAmountForUnsignedIntegers) {
    char *testBuffer = static_cast<char *>(malloc(BUFFER_SIZE));
    ASSERT_EQ(10, writeToBuffer(testBuffer, "I want %d", 50)) << "Expected the correct amount of characters to be written.";
    ASSERT_EQ(13, writeToBuffer(testBuffer, "I want %d", 12765)) << "Expected the correct amount of characters to be written.";
    free(testBuffer);
};


TEST(DebugMessageFormatterTest, ReplacesUnsignedIntegers) {
    char *testBuffer = static_cast<char *>(malloc(BUFFER_SIZE));
    writeToBuffer(testBuffer, "I want %d", 50);
    ASSERT_EQ(strcmp("I want 50", testBuffer), 0) << "Expected \"I want 50\", but got: " << testBuffer;
    writeToBuffer(testBuffer, "I want %d", 1236817);
    ASSERT_EQ(strcmp("I want 1236817", testBuffer), 0) << "Expected \"I want 1236817\", but got: " << testBuffer;
    free(testBuffer);
};

TEST(DebugMessageFormatterTest, ReplacesSignedIntegers) {
    char *testBuffer = static_cast<char *>(malloc(BUFFER_SIZE));
    writeToBuffer(testBuffer, "I want %d", -50);
    ASSERT_EQ(strcmp("I want -50", testBuffer), 0) << "Expected \"I want -50\", but got: " << testBuffer;
    writeToBuffer(testBuffer, "I want %d", -1236817);
    ASSERT_EQ(strcmp("I want -1236817", testBuffer), 0) << "Expected \"I want -1236817\", but got: " << testBuffer;
    free(testBuffer);
};

TEST(DebugMessageFormatterTest, CanReplaceMixedSignedAndUnsignedIntegers) {
    char *testBuffer = static_cast<char *>(malloc(BUFFER_SIZE));
    writeToBuffer(testBuffer, "I want %d, %d", -50, 10);
    ASSERT_EQ(strcmp("I want -50, 10", testBuffer), 0) << "Expected \"I want -50, 10\", but got: " << testBuffer;
    free(testBuffer);
};

TEST(DebugMessageFormatterTest, WritesCorrectAmountForHexadecimal) {
    char *testBuffer = static_cast<char *>(malloc(BUFFER_SIZE));
    ASSERT_EQ(19, writeToBuffer(testBuffer, "%x", 56)) << "Expected 2 characters to be written for formatter, 16 for the number, and 1 for the terminator.";
    free(testBuffer);
};

TEST(DebugMessageFormatterTest, CanReplaceHexadecimal) {
    char *testBuffer = static_cast<char *>(malloc(BUFFER_SIZE));
    writeToBuffer(testBuffer, "I want %x", 256);
    ASSERT_EQ(strcmp("I want 0x0000000000000100", testBuffer), 0) << "Expected \"I want 0x0000000000000100\", but got: " << testBuffer;
    writeToBuffer(testBuffer, "I want %x", 4294967296);
    ASSERT_EQ(strcmp("I want 0x0000000100000000", testBuffer), 0) << "Expected \"I want 0x0000000010000000\", but got: " << testBuffer;
   free(testBuffer);
};

TEST(DebugMessageFormatterTest, WritesCorrectAmountForBinary) {
    char *testBuffer = static_cast<char *>(malloc(BUFFER_SIZE));
    ASSERT_EQ(67, writeToBuffer(testBuffer, "%b", 56)) << "Expected 2 characters to be written for formatter, 64 for the number, and 1 for the terminator.";
    free(testBuffer);
};

TEST(DebugMessageFormatterTest, CanReplaceBinary) {
    char *testBuffer = static_cast<char *>(malloc(BUFFER_SIZE));
    writeToBuffer(testBuffer, "I want %b", 256);
    ASSERT_EQ(strcmp("I want 0b0000000000000000000000000000000000000000000000000000000100000000", testBuffer), 0) << "Expected \"I want 0b0000000000000000000000000000000000000000000000000000000100000000\", but got: " << testBuffer;
    writeToBuffer(testBuffer, "I want %b", 4294967296);
    ASSERT_EQ(strcmp("I want 0b0000000000000000000000000000000100000000000000000000000000000000", testBuffer), 0) << "Expected \"I want 0b0000000000000000000000000000000100000000000000000000000000000000\", but got: " << testBuffer;
   free(testBuffer);
};

// TODO: Need to add compressed printing, too long for some of these 




