// 02-parsing  --  stubs for the parsing / state-machine pattern pack.
//
// Practise: copy solutions/parsing_solutions.cpp to attempts/, blank the bodies,
// build with -DIMPL_DIR=attempts. Every function here must be total: any byte
// string in, a defined answer out, no out-of-bounds reads (ASan checks this).
#pragma once

#include <string>
#include <vector>

namespace lc {

// 8    String to Integer (atoi).  Context: parsing a decimal field from a SIP/SDP text header with saturation.
int my_atoi(const std::string& s);

// 468  Validate IP Address.       Context: validating a bearer/endpoint address before building a socket.
// Returns exactly "IPv4", "IPv6" or "Neither".
std::string valid_ip_address(const std::string& ip);

// 271  Encode and Decode Strings. Context: framing arbitrary text/binary records on a byte stream (length-prefix framing).
std::string encode_strings(const std::vector<std::string>& strs);
std::vector<std::string> decode_strings(const std::string& s);

// 443  String Compression.        Context: in-place run-length encoding of a repeating pad/idle pattern.
// Compress in place, return the new length. Runs of 1 are not followed by a count.
int compress(std::vector<char>& chars);

// 65   Valid Number.              Context: validating a numeric config / frequency field ("868.1e6") with a DFA.
bool is_number(const std::string& s);

}  // namespace lc
