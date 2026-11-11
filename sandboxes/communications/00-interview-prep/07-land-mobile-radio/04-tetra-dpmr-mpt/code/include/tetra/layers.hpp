#pragma once
// STUB - worksheet 04, Q7/Q10 : which TETRA protocol layer owns a PDU?
//
// TETRA air-interface stack (ETSI EN 300 392-2 and -2 for MAC/LLC/MLE; -2 clause numbers VERIFY;
// CMCE: EN 300 392-2 call control + SDS + SS; MM: EN 300 392-2 mobility management):
//   Layer 1  PHY   : bursts, modulation, channel coding
//   Layer 2  MAC   : slot access, fragmentation, logical channels      (PDUs "MAC-...")
//            LLC   : basic link (BL-...) / advanced link (AL-...), ARQ  (PDUs "BL-...", "AL-...")
//   Layer 3  MLE   : mobile link entity: cell selection, attach to network, roaming
//            CMCE  : circuit-mode control entity: call setup, floor (TX demand/granted), SDS
//            MM    : mobility management: location update, group attach/detach, authentication
//
// Names below are given the way the standard writes them (with spaces) in places; the classifier
// must ignore case and treat ' ' and '_' like '-'.  VERIFY every name against EN 300 392-2.
#include <string>
#include <string_view>

#include "lmr/todo.hpp"

namespace tetra {

enum class Layer { Mac, Llc, Mle, Cmce, Mm, Unknown };

// TODO: upper-case; replace ' ' and '_' by '-'.   "d-tx granted" -> "D-TX-GRANTED"
inline std::string normalize_pdu_name(std::string_view /*name*/) {
  lmr::todo("normalize_pdu_name");
}

// TODO: classify by (normalised) prefix:
//   Mac : "MAC-"
//   Llc : "BL-", "AL-"
//   Mle : "D-NWRK-BROADCAST", "D-NEW-CELL", "D-PREPARE-FAIL", "U-PREPARE", "U-RESTORE", "D-RESTORE"
//   Mm  : "D-LOCATION-UPDATE", "U-LOCATION-UPDATE", "U-ATTACH", "D-ATTACH", "D-AUTHENTICATION",
//         "U-AUTHENTICATION", "U-MM-", "D-MM-"
//   Cmce: "D-SETUP", "U-SETUP", "D-CONNECT", "U-CONNECT", "D-CALL-PROCEEDING", "D-ALERT", "U-ALERT",
//         "D-TX-", "U-TX-", "D-RELEASE", "U-DISCONNECT", "D-DISCONNECT", "D-INFO", "U-INFO",
//         "D-SDS-", "U-SDS-", "D-STATUS", "U-STATUS"
//   anything else -> Unknown
inline Layer layer_of_pdu(std::string_view /*name*/) { lmr::todo("layer_of_pdu"); }

// GIVEN
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
