#pragma once
// Test-vector loading (provided; do not edit).  Vector files are plain text, one number per line,
// written by octave/gen_vectors.m with %.17g (17 significant digits round-trips a double exactly).
//
// VECTOR_DIR is a compile definition set by CMake (<pack>/vectors).
#include <cstddef>
#include <fstream>
#include <string>
#include <vector>

#include "octave_builds.hpp"

#ifndef VECTOR_DIR
#define VECTOR_DIR "vectors"
#endif

namespace dsp {

inline std::string vector_path(const std::string& name) { return std::string(VECTOR_DIR) + "/" + name; }

// Returns false if the file cannot be opened or is empty.
inline bool read_vector(const std::string& name, std::vector<double>& out) {
  std::ifstream f(vector_path(name));
  if (!f) return false;
  out.clear();
  double v = 0.0;
  while (f >> v) out.push_back(v);
  return !out.empty();
}

inline bool read_int_vector(const std::string& name, std::vector<int>& out) {
  std::vector<double> d;
  if (!read_vector(name, d)) return false;
  out.assign(d.size(), 0);
  for (std::size_t i = 0; i < d.size(); ++i) out[i] = static_cast<int>(d[i]);  // exact: values are small integers
  return true;
}

inline std::vector<cplx> merge_cplx(const std::vector<double>& re, const std::vector<double>& im) {
  std::vector<cplx> z(re.size());
  for (std::size_t i = 0; i < re.size() && i < im.size(); ++i) z[i] = cplx(re[i], im[i]);
  return z;
}

}  // namespace dsp
