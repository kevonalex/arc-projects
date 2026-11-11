#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

#include <es_toolchain/frame.hpp>

using namespace es::toolchain;

TEST(FrameEncode, HandComputedFrame) {
    const std::uint8_t payload[] = {0x01, 0x02, 0x04};  // xor = 0x07
    std::vector<std::uint8_t> out(6);                   // EXACT size: ASan guards the edge
    ASSERT_EQ(frame_encode(payload, 3, out.data(), out.size()), 6u);
    const std::vector<std::uint8_t> expect = {0x7E, 0x03, 0x01, 0x02, 0x04, 0x07};
    EXPECT_EQ(out, expect);
}

TEST(FrameEncode, EmptyPayloadIsThreeBytes) {
    std::vector<std::uint8_t> out(3);
    ASSERT_EQ(frame_encode(nullptr, 0, out.data(), out.size()), 3u);
    EXPECT_EQ(out[0], 0x7E);
    EXPECT_EQ(out[1], 0x00);
    EXPECT_EQ(out[2], 0x00);
}

TEST(FrameEncode, TooSmallOutputReturnsZeroAndWritesNothing) {
    const std::uint8_t payload[] = {1, 2, 4};
    std::vector<std::uint8_t> out(5, 0xCC);  // needs 6
    EXPECT_EQ(frame_encode(payload, 3, out.data(), out.size()), 0u);
    for (auto b : out) EXPECT_EQ(b, 0xCC);
}

TEST(FrameEncode, PayloadOver255Rejected) {
    std::vector<std::uint8_t> payload(256, 0x11);
    std::vector<std::uint8_t> out(1000);
    EXPECT_EQ(frame_encode(payload.data(), payload.size(), out.data(), out.size()), 0u);
}

TEST(FrameEncode, Max255PayloadFitsExactly) {
    std::vector<std::uint8_t> payload(255, 0x11);  // xor of odd count of 0x11 = 0x11
    std::vector<std::uint8_t> out(258);
    ASSERT_EQ(frame_encode(payload.data(), payload.size(), out.data(), out.size()), 258u);
    EXPECT_EQ(out[1], 0xFF);
    EXPECT_EQ(out[257], 0x11);
}

TEST(FrameDecode, RoundTrip) {
    const std::vector<std::uint8_t> payload = {9, 8, 7, 6, 5};
    std::vector<std::uint8_t> frame(payload.size() + kFrameOverhead);
    ASSERT_EQ(frame_encode(payload.data(), payload.size(), frame.data(), frame.size()), frame.size());
    std::vector<std::uint8_t> back(payload.size());
    ASSERT_EQ(frame_decode(frame.data(), frame.size(), back.data(), back.size()), payload.size());
    EXPECT_EQ(back, payload);
}

TEST(FrameDecode, RejectsBadStartByte) {
    const std::uint8_t f[] = {0x7F, 0x01, 0x05, 0x05};
    std::uint8_t out[1];
    EXPECT_EQ(frame_decode(f, 4, out, 1), kDecodeError);
}

TEST(FrameDecode, RejectsBadChecksum) {
    const std::uint8_t f[] = {0x7E, 0x01, 0x05, 0x06};
    std::uint8_t out[1];
    EXPECT_EQ(frame_decode(f, 4, out, 1), kDecodeError);
}

TEST(FrameDecode, RejectsLengthMismatchAndTruncation) {
    const std::uint8_t f[] = {0x7E, 0x02, 0x05, 0x05};  // says 2 payload bytes, only 1 present
    std::uint8_t out[4];
    EXPECT_EQ(frame_decode(f, 4, out, 4), kDecodeError);
    EXPECT_EQ(frame_decode(f, 2, out, 4), kDecodeError);  // shorter than overhead+len
    EXPECT_EQ(frame_decode(f, 0, out, 4), kDecodeError);
}

TEST(FrameDecode, RejectsOutputTooSmall) {
    const std::uint8_t f[] = {0x7E, 0x02, 0x01, 0x02, 0x03};
    std::uint8_t out[1];
    EXPECT_EQ(frame_decode(f, 5, out, 1), kDecodeError);
}
