#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <limits>
#include <vector>

#include <es_containers/views.hpp>

using namespace es::containers;

namespace {
ByteView view_of(const std::vector<std::uint8_t>& v) { return ByteView(v.data(), v.size()); }
}  // namespace

TEST(FramePayload, ReturnsZeroCopyViewIntoFrame) {
    const std::vector<std::uint8_t> frame = {0x7E, 0x03, 0x01, 0x02, 0x04, 0x07};
    auto p = frame_payload(view_of(frame));
    ASSERT_TRUE(p.has_value());
    EXPECT_EQ(p->size(), 3u);
    EXPECT_EQ(p->data(), frame.data() + 2) << "must be a view into the frame, not a copy";
    EXPECT_EQ((*p)[2], 0x04);
}

TEST(FramePayload, EmptyPayloadIsValid) {
    const std::vector<std::uint8_t> frame = {0x7E, 0x00, 0x00};
    auto p = frame_payload(view_of(frame));
    ASSERT_TRUE(p.has_value());
    EXPECT_TRUE(p->empty());
}

TEST(FramePayload, RejectsMalformedFrames) {
    EXPECT_FALSE(frame_payload(view_of({})).has_value());                                  // empty
    EXPECT_FALSE(frame_payload(view_of({0x7E, 0x00})).has_value());                        // too short
    EXPECT_FALSE(frame_payload(view_of({0x7F, 0x01, 0x05, 0x05})).has_value());            // bad marker
    EXPECT_FALSE(frame_payload(view_of({0x7E, 0x01, 0x05, 0x06})).has_value());            // bad checksum
    EXPECT_FALSE(frame_payload(view_of({0x7E, 0x02, 0x05, 0x05})).has_value());            // length > data
    EXPECT_FALSE(frame_payload(view_of({0x7E, 0x01, 0x05, 0x05, 0x00})).has_value());      // trailing byte
}

TEST(SubSpan, InRange) {
    const std::vector<std::uint8_t> v = {10, 11, 12, 13};
    auto a = sub_span(view_of(v), 0, 3);
    ASSERT_TRUE(a.has_value());
    EXPECT_EQ(a->size(), 3u);
    auto b = sub_span(view_of(v), 2, 2);
    ASSERT_TRUE(b.has_value());
    EXPECT_EQ(b->data(), v.data() + 2);
    auto c = sub_span(view_of(v), 4, 0);  // empty view at the end is legal
    ASSERT_TRUE(c.has_value());
    EXPECT_TRUE(c->empty());
}

TEST(SubSpan, OutOfRange) {
    const std::vector<std::uint8_t> v = {10, 11, 12, 13};
    EXPECT_FALSE(sub_span(view_of(v), 2, 3).has_value());
    EXPECT_FALSE(sub_span(view_of(v), 5, 0).has_value());
    EXPECT_FALSE(sub_span(view_of(v), 0, 5).has_value());
}

TEST(SubSpan, NoSizeTOverflow) {
    const std::vector<std::uint8_t> v = {1, 2, 3, 4};
    const auto big = std::numeric_limits<std::size_t>::max();
    EXPECT_FALSE(sub_span(view_of(v), 2, big).has_value());      // 2 + big wraps to 1 -> naive check passes!
    EXPECT_FALSE(sub_span(view_of(v), big, 2).has_value());
    EXPECT_FALSE(sub_span(view_of(v), 1, big).has_value());      // 1 + big wraps to 0
}

TEST(BeBytes, Basic) {
    EXPECT_EQ(be_bytes(0x01020304u), (std::array<std::uint8_t, 4>{1, 2, 3, 4}));
    EXPECT_EQ(be_bytes(0xFF000000u), (std::array<std::uint8_t, 4>{0xFF, 0, 0, 0}));
    EXPECT_EQ(be_bytes(0u), (std::array<std::uint8_t, 4>{0, 0, 0, 0}));
}

TEST(EraseValue, RemovesAllMatches) {
    std::vector<std::uint8_t> v = {1, 2, 1, 3, 1};
    EXPECT_EQ(erase_value(v, 1), 3u);
    EXPECT_EQ(v, (std::vector<std::uint8_t>{2, 3}));  // order of the rest preserved, size shrunk
}

TEST(EraseValue, NoMatchAndEmpty) {
    std::vector<std::uint8_t> v = {5, 6};
    EXPECT_EQ(erase_value(v, 9), 0u);
    EXPECT_EQ(v.size(), 2u);
    std::vector<std::uint8_t> e;
    EXPECT_EQ(erase_value(e, 0), 0u);
}

TEST(DedupeSorted, Basic) {
    std::vector<int> v = {1, 1, 2, 2, 2, 3};
    dedupe_sorted(v);
    EXPECT_EQ(v, (std::vector<int>{1, 2, 3}));
    std::vector<int> e;
    dedupe_sorted(e);
    EXPECT_TRUE(e.empty());
    std::vector<int> one = {7};
    dedupe_sorted(one);
    EXPECT_EQ(one, (std::vector<int>{7}));
}
