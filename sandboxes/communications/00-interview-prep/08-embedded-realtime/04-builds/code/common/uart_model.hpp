// uart_model.hpp  --  an emulated UART (GIVEN, not a stub). Time advances only when tick() is called.
// TX: write_dr() moves a byte into the shift register (TXE goes low); the next tick() puts it on the wire (TXE high).
// RX: inject_rx() models a byte arriving; reading DR clears RXNE. An unread byte that is overwritten sets overruns.
#pragma once
#include <cstdint>
#include <vector>

namespace uartsim {

struct UartModel {
  bool txe = true, rxne = false;
  bool txeie = false, rxneie = false;  // interrupt enables
  uint8_t shift = 0, rx_dr = 0;
  int overruns = 0;
  std::vector<uint8_t> wire;  // everything transmitted, in order

  void write_dr(uint8_t b) {
    shift = b;
    txe = false;
  }
  uint8_t read_dr() {
    rxne = false;
    return rx_dr;
  }
  void tick() {
    if (!txe) {
      wire.push_back(shift);
      txe = true;
    }
  }
  void inject_rx(uint8_t b) {
    if (rxne) ++overruns;
    rx_dr = b;
    rxne = true;
  }
  bool irq_pending() const { return (txe && txeie) || (rxne && rxneie); }
};

}  // namespace uartsim
