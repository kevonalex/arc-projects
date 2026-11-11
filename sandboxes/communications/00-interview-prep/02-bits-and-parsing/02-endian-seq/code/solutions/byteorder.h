// byteorder.h  --  REFERENCE SOLUTION for Pack 02.
#pragma once
#include <cstdint>
#include <cstring>

namespace byteorder {

inline bool host_is_little_endian() {
  const uint16_t one = 1;
  uint8_t first_byte = 0;
  std::memcpy(&first_byte, &one, 1);  // memcpy: no aliasing violation
  return first_byte == 1;
}

inline uint16_t bswap16(uint16_t v) {
  return static_cast<uint16_t>((v << 8) | (v >> 8));
}
inline uint32_t bswap32(uint32_t v) {
  return ((v & 0x000000FFu) << 24) | ((v & 0x0000FF00u) << 8) | ((v >> 8) & 0x0000FF00u) |
         (v >> 24);
}

inline uint16_t my_htons(uint16_t v) { return host_is_little_endian() ? bswap16(v) : v; }
inline uint16_t my_ntohs(uint16_t v) { return my_htons(v); }  // the swap is its own inverse
inline uint32_t my_htonl(uint32_t v) { return host_is_little_endian() ? bswap32(v) : v; }
inline uint32_t my_ntohl(uint32_t v) { return my_htonl(v); }

inline uint16_t load_be16(const uint8_t* p) {
  return static_cast<uint16_t>((static_cast<uint16_t>(p[0]) << 8) | p[1]);
}
inline uint32_t load_be32(const uint8_t* p) {
  return (static_cast<uint32_t>(p[0]) << 24) | (static_cast<uint32_t>(p[1]) << 16) |
         (static_cast<uint32_t>(p[2]) << 8) | static_cast<uint32_t>(p[3]);
}
inline uint16_t load_le16(const uint8_t* p) {
  return static_cast<uint16_t>((static_cast<uint16_t>(p[1]) << 8) | p[0]);
}
inline uint32_t load_le32(const uint8_t* p) {
  return (static_cast<uint32_t>(p[3]) << 24) | (static_cast<uint32_t>(p[2]) << 16) |
         (static_cast<uint32_t>(p[1]) << 8) | static_cast<uint32_t>(p[0]);
}
inline void store_be16(uint8_t* p, uint16_t v) {
  p[0] = static_cast<uint8_t>(v >> 8);
  p[1] = static_cast<uint8_t>(v & 0xFFu);
}
inline void store_be32(uint8_t* p, uint32_t v) {
  p[0] = static_cast<uint8_t>(v >> 24);
  p[1] = static_cast<uint8_t>((v >> 16) & 0xFFu);
  p[2] = static_cast<uint8_t>((v >> 8) & 0xFFu);
  p[3] = static_cast<uint8_t>(v & 0xFFu);
}
inline void store_le16(uint8_t* p, uint16_t v) {
  p[0] = static_cast<uint8_t>(v & 0xFFu);
  p[1] = static_cast<uint8_t>(v >> 8);
}
inline void store_le32(uint8_t* p, uint32_t v) {
  p[0] = static_cast<uint8_t>(v & 0xFFu);
  p[1] = static_cast<uint8_t>((v >> 8) & 0xFFu);
  p[2] = static_cast<uint8_t>((v >> 16) & 0xFFu);
  p[3] = static_cast<uint8_t>(v >> 24);
}

// a - b is computed modulo 2^16 (the clock-face distance clockwise from b to a);
// reading it as int16_t turns "more than half a turn" into a negative number.
// uint16_t operands promote to int, so a-b may be negative before the cast: the explicit
// conversion to uint16_t first makes the modular step explicit.
inline int16_t seq_diff16(uint16_t a, uint16_t b) {
  return static_cast<int16_t>(static_cast<uint16_t>(a - b));
}
inline bool seq_newer16(uint16_t a, uint16_t b) { return seq_diff16(a, b) > 0; }
inline int32_t seq_diff32(uint32_t a, uint32_t b) { return static_cast<int32_t>(a - b); }
inline bool seq_newer32(uint32_t a, uint32_t b) { return seq_diff32(a, b) > 0; }

class SeqExtender {
 public:
  int64_t update(uint16_t s) {
    if (!started_) {
      started_ = true;
      highest_ = s;
      return highest_;
    }
    const uint16_t highest_low = static_cast<uint16_t>(highest_ & 0xFFFF);
    const int64_t d = seq_diff16(s, highest_low);  // signed clock-face distance from highest
    const int64_t ext = highest_ + d;
    if (d > 0) highest_ = ext;  // only move the high-water mark forward
    return ext;
  }
  int64_t highest() const { return highest_; }

 private:
  bool started_ = false;
  int64_t highest_ = 0;
};

}  // namespace byteorder
