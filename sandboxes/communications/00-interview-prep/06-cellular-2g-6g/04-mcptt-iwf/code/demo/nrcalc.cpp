// nrcalc -- NR numerology calculator CLI (Pack 06/04 build #2).  Built against the reference solution.
//   ./nrcalc            -> table for mu = 0..4 at 20 MHz (where the PRB table has an entry)
//   ./nrcalc 1 100      -> one row: mu = 1, 100 MHz channel
#include <cstdio>
#include <cstdlib>

#include "mcptt.hpp"

int main(int argc, char** argv) {
  if (argc == 3) {
    std::puts(iwf::format_row(iwf::numerology_row(std::atoi(argv[1]), std::atof(argv[2]))).c_str());
    return 0;
  }
  for (int mu = 0; mu <= 4; ++mu) std::puts(iwf::format_row(iwf::numerology_row(mu, 20.0)).c_str());
  return 0;
}
