// ber_sweep -- Monte-Carlo BER curve of the 4FSK modem vs the closed-form theory.  Built against the reference solution.
//   ./ber_sweep [bits_per_point=200000]
// Prints Es/N0, Eb/N0, measured BER, theory BER and the number of errors seen (>=100 errors = trustworthy to about +-20 %).
#include <cstdio>
#include <cstdlib>

#include "modem.hpp"

int main(int argc, char** argv) {
  const size_t nbits = argc > 1 ? static_cast<size_t>(std::atol(argv[1])) : 200000;
  std::printf("%8s %8s %12s %12s %8s\n", "Es/N0", "Eb/N0", "BER(meas)", "BER(theory)", "errors");
  for (double esn0 = 0.0; esn0 <= 14.0; esn0 += 2.0) {
    gw::Rng rng(1000 + static_cast<uint64_t>(esn0 * 10));
    std::vector<uint8_t> bits(nbits);
    for (auto& b : bits) b = static_cast<uint8_t>(rng.next() >> 63);
    auto syms = gw::bits_to_symbols(bits);
    auto iq = gw::modulate(syms);
    gw::add_awgn(iq, esn0, rng);
    auto rx = gw::symbols_to_bits(gw::demodulate(iq, 0, syms.size()));
    const size_t e = gw::count_bit_errors(bits, rx);
    std::printf("%8.1f %8.2f %12.3e %12.3e %8zu%s\n", esn0, gw::esn0_to_ebn0_db(esn0), static_cast<double>(e) / static_cast<double>(nbits),
                gw::ber_4fsk_noncoherent(esn0), e, e < 100 ? "   (too few errors to trust)" : "");
  }
  return 0;
}
