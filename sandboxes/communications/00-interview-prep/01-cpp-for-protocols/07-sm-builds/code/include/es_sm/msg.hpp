#pragma once
// PROVIDED (identical in include/ and solutions/) - messages as a std::variant, plus the `overloaded`
// helper used with std::visit. Unlike the plain `Event` enum, each message can carry its own payload.
#include <cstdint>
#include <variant>

namespace es::sm {

struct PttPress {};
struct PttRelease {};
struct FloorGranted { std::uint32_t talker_id; };   // the id the infrastructure granted (ours)
struct FloorDenied { std::uint8_t reason; };        // synthetic reason code
struct RemoteTalkStart { std::uint32_t talker_id; };
struct RemoteTalkEnd {};
struct HangTimeout {};

using Msg = std::variant<PttPress, PttRelease, FloorGranted, FloorDenied,
                         RemoteTalkStart, RemoteTalkEnd, HangTimeout>;

// "overload set from lambdas": inherits every lambda's operator() so std::visit can pick one by type.
template <class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;  // deduction guide (C++17; implicit in C++20)

}  // namespace es::sm
