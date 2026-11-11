// byteorder.h  --  STUBS for Pack 02 (student version).
// NOTE: deliberately NOT named endian.h: glibc has <endian.h>, and an -I directory containing a file
// with that name would shadow it and break system headers.
// Reference answers: ../solutions/byteorder.h
#pragma once
#include <cstdint>

namespace byteorder {

// ---- host byte order (W 3.2) ----
// True if the least-significant byte of a uint16_t is stored at the lowest address.
// Detect it by inspecting memory (memcpy), NOT with a pointer cast.
inline bool host_is_little_endian() { return false; }  // TODO W 3.2

// ---- manual byte swap (W 3.5) ----
inline uint16_t bswap16(uint16_t v) { return v; }  // TODO: 0x1234 -> 0x3412
inline uint32_t bswap32(uint32_t v) { return v; }  // TODO: 0x11223344 -> 0x44332211

// ---- htons/ntohs/htonl/ntohl equivalents (W 3.4) ----
// Result must be such that *storing the returned integer in memory* yields big-endian bytes.
inline uint16_t my_htons(uint16_t v) { return v; }  // TODO (swap on little-endian hosts, nothing on big)
inline uint16_t my_ntohs(uint16_t v) { return v; }  // TODO
inline uint32_t my_htonl(uint32_t v) { return v; }  // TODO
inline uint32_t my_ntohl(uint32_t v) { return v; }  // TODO

// ---- host-independent load/store using shifts (W 3.6) ----
inline uint16_t load_be16([[maybe_unused]] const uint8_t* p) { return 0; }  // TODO p[0] is the high byte
inline uint32_t load_be32([[maybe_unused]] const uint8_t* p) { return 0; }  // TODO
inline uint16_t load_le16([[maybe_unused]] const uint8_t* p) { return 0; }  // TODO p[0] is the low byte
inline uint32_t load_le32([[maybe_unused]] const uint8_t* p) { return 0; }  // TODO
inline void store_be16([[maybe_unused]] uint8_t* p, [[maybe_unused]] uint16_t v) {}  // TODO
inline void store_be32([[maybe_unused]] uint8_t* p, [[maybe_unused]] uint32_t v) {}  // TODO
inline void store_le16([[maybe_unused]] uint8_t* p, [[maybe_unused]] uint16_t v) {}  // TODO
inline void store_le32([[maybe_unused]] uint8_t* p, [[maybe_unused]] uint32_t v) {}  // TODO

// ---- serial-number (sequence) arithmetic, RFC 1982 style (W 3.7) ----
// seq_diff16(a,b): signed distance "a minus b" on the 16-bit clock face, in [-32768, 32767].
inline int16_t seq_diff16(uint16_t a, [[maybe_unused]] uint16_t b) { return static_cast<int16_t>(a); }  // TODO
// true iff a is strictly newer than b (exactly half a turn apart => false both ways).
inline bool seq_newer16([[maybe_unused]] uint16_t a, [[maybe_unused]] uint16_t b) { return false; }  // TODO
inline int32_t seq_diff32(uint32_t a, [[maybe_unused]] uint32_t b) { return static_cast<int32_t>(a); }  // TODO
inline bool seq_newer32([[maybe_unused]] uint32_t a, [[maybe_unused]] uint32_t b) { return false; }  // TODO

// ---- extend a wrapping 16-bit sequence number to a monotonic 64-bit counter (W 3.8) ----
// update(s) returns the extended value of the packet with 16-bit sequence s.
//  * First call: returns s.
//  * Later: pick the extended value closest to the highest seen so far (reordered packets get a
//    value <= highest; the highest only moves forward).
class SeqExtender {
 public:
  int64_t update([[maybe_unused]] uint16_t s) { return -999; }  // TODO
  int64_t highest() const { return highest_; }

 private:
  [[maybe_unused]] bool started_ = false;  // you will need this
  int64_t highest_ = 0;
};

}  // namespace byteorder
