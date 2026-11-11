#include "bits.hpp"

namespace lc {

// 191 -- Kernighan: n & (n-1) clears the lowest set bit. O(popcount) time, O(1) space.
int hamming_weight(std::uint32_t n) {
    int count = 0;
    while (n != 0) {
        n &= n - 1;
        ++count;
    }
    return count;
}

// 190 -- shift result left, feed it n's low bit, shift n right. 32 iterations.
std::uint32_t reverse_bits(std::uint32_t n) {
    std::uint32_t out = 0;
    for (int i = 0; i < 32; ++i) {
        out = (out << 1) | (n & 1u);
        n >>= 1;
    }
    return out;
}

// 338 -- bits(i) = bits(i / 2) + (i mod 2). O(n) time and space.
std::vector<int> count_bits(int n) {
    std::vector<int> bits(static_cast<std::size_t>(n) + 1, 0);
    for (int i = 1; i <= n; ++i) {
        bits[static_cast<std::size_t>(i)] =
            bits[static_cast<std::size_t>(i >> 1)] + (i & 1);
    }
    return bits;
}

// 136 -- XOR is commutative, associative, x^x=0, x^0=x: pairs vanish.
int single_number(const std::vector<int>& nums) {
    int acc = 0;
    for (int x : nums) acc ^= x;
    return acc;
}

// 137 -- two-bit-per-position counter mod 3, done for all 32 positions at once.
//   (twos, ones) counts 00 -> 01 -> 10 -> 00.
int single_number_ii(const std::vector<int>& nums) {
    std::uint32_t ones = 0, twos = 0;
    for (int v : nums) {
        const auto x = static_cast<std::uint32_t>(v);
        ones = (ones ^ x) & ~twos;
        twos = (twos ^ x) & ~ones;
    }
    return static_cast<int>(ones);
}

// 268 -- XOR of 0..n and all values: every present id cancels, the missing id remains.
int missing_number(const std::vector<int>& nums) {
    auto acc = static_cast<int>(nums.size());
    for (int i = 0; i < static_cast<int>(nums.size()); ++i) {
        acc ^= i ^ nums[static_cast<std::size_t>(i)];
    }
    return acc;
}

// 461 -- XOR marks differing positions; popcount counts them.
int hamming_distance(int x, int y) {
    return hamming_weight(static_cast<std::uint32_t>(x) ^ static_cast<std::uint32_t>(y));
}

// 477 -- per bit position: k ones and (n-k) zeros give k*(n-k) differing pairs.
int total_hamming_distance(const std::vector<int>& nums) {
    const long long n = static_cast<long long>(nums.size());
    long long total = 0;
    for (int bit = 0; bit < 32; ++bit) {
        long long ones = 0;
        for (int v : nums) ones += (static_cast<std::uint32_t>(v) >> bit) & 1u;
        total += ones * (n - ones);
    }
    return static_cast<int>(total);
}

// 371 -- a + b = (a ^ b) + ((a & b) << 1); repeat until the carry dies out.
// Unsigned arithmetic makes the carry shift and negative wrap well defined.
int get_sum(int a, int b) {
    auto x = static_cast<std::uint32_t>(a);
    auto y = static_cast<std::uint32_t>(b);
    while (y != 0) {
        const std::uint32_t carry = (x & y) << 1;
        x ^= y;
        y = carry;
    }
    return static_cast<int>(x);
}

// 201 -- the AND of a range is the common binary prefix of left and right, then zeros.
int range_bitwise_and(int left, int right) {
    int shift = 0;
    while (left != right) {
        left >>= 1;
        right >>= 1;
        ++shift;
    }
    return left << shift;
}

// 89 -- reflected binary code: g(i) = i ^ (i >> 1).
std::vector<int> gray_code(int n) {
    std::vector<int> out;
    const int total = 1 << n;
    out.reserve(static_cast<std::size_t>(total));
    for (int i = 0; i < total; ++i) out.push_back(i ^ (i >> 1));
    return out;
}

// 393 -- header byte tells how many 10xxxxxx continuation bytes must follow.
bool valid_utf8(const std::vector<int>& data) {
    int remaining = 0;
    for (int v : data) {
        const int b = v & 0xFF;  // only the least-significant 8 bits count
        if (remaining == 0) {
            if ((b >> 7) == 0b0)            remaining = 0;
            else if ((b >> 5) == 0b110)     remaining = 1;
            else if ((b >> 4) == 0b1110)    remaining = 2;
            else if ((b >> 3) == 0b11110)   remaining = 3;
            else return false;              // 10xxxxxx orphan or 11111xxx
        } else {
            if ((b >> 6) != 0b10) return false;
            --remaining;
        }
    }
    return remaining == 0;
}

}  // namespace lc
