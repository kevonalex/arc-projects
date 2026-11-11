#pragma once
// SOLUTION - worksheet 04, Q7/Q10.
#include <cctype>
#include <initializer_list>
#include <string>
#include <string_view>

#include "lmr/todo.hpp"

namespace tetra {

enum class Layer { Mac, Llc, Mle, Cmce, Mm, Unknown };

inline std::string normalize_pdu_name(std::string_view name) {
  std::string out;
  out.reserve(name.size());
  for (char c : name) {
    if (c == ' ' || c == '_') {
      out.push_back('-');
    } else {
      out.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(c))));
    }
  }
  return out;
}

namespace detail {
inline bool starts_with_any(const std::string& s, std::initializer_list<const char*> prefixes) {
  for (const char* p : prefixes) {
    if (s.rfind(p, 0) == 0) return true;
  }
  return false;
}
}  // namespace detail

inline Layer layer_of_pdu(std::string_view name) {
  const std::string n = normalize_pdu_name(name);
  if (detail::starts_with_any(n, {"MAC-"})) return Layer::Mac;
  if (detail::starts_with_any(n, {"BL-", "AL-"})) return Layer::Llc;
  if (detail::starts_with_any(n, {"D-NWRK-BROADCAST", "D-NEW-CELL", "D-PREPARE-FAIL", "U-PREPARE",
                                  "U-RESTORE", "D-RESTORE"})) {
    return Layer::Mle;
  }
  if (detail::starts_with_any(n, {"D-LOCATION-UPDATE", "U-LOCATION-UPDATE", "U-ATTACH", "D-ATTACH",
                                  "D-AUTHENTICATION", "U-AUTHENTICATION", "U-MM-", "D-MM-"})) {
    return Layer::Mm;
  }
  if (detail::starts_with_any(n, {"D-SETUP", "U-SETUP", "D-CONNECT", "U-CONNECT", "D-CALL-PROCEEDING",
                                  "D-ALERT", "U-ALERT", "D-TX-", "U-TX-", "D-RELEASE", "U-DISCONNECT",
                                  "D-DISCONNECT", "D-INFO", "U-INFO", "D-SDS-", "U-SDS-", "D-STATUS",
                                  "U-STATUS"})) {
    return Layer::Cmce;
  }
  return Layer::Unknown;
}

inline const char* layer_name(Layer l) {
  switch (l) {
    case Layer::Mac: return "MAC";
    case Layer::Llc: return "LLC";
    case Layer::Mle: return "MLE";
    case Layer::Cmce: return "CMCE";
    case Layer::Mm: return "MM";
    case Layer::Unknown: return "?";
  }
  return "?";
}

}  // namespace tetra
