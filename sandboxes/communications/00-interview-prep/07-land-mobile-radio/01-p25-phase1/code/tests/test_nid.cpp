#include <gtest/gtest.h>

#include <stdexcept>

#include "p25p1/nid.hpp"

using namespace p25p1;

TEST(Nid, PackUsesNacHighDuidLow) {
  EXPECT_EQ(pack_nid_data(0x293, Duid::Ldu1), 0x2935);
  EXPECT_EQ(pack_nid_data(0x000, Duid::Hdu), 0x0000);
  EXPECT_EQ(pack_nid_data(0xFFF, Duid::TduLc), 0xFFFF);
  EXPECT_EQ(pack_nid_data(0x001, Duid::Ldu2), 0x001A);
}

TEST(Nid, NacTooWideThrows) {
  EXPECT_THROW(pack_nid_data(0x1000, Duid::Hdu), std::out_of_range);
}

TEST(Nid, UnpackRoundTrips) {
  for (std::uint16_t nac : {0x000, 0x293, 0xF7E, 0xFFF}) {
    const auto n = unpack_nid_data(pack_nid_data(nac, Duid::Tsbk));
    EXPECT_EQ(n.nac, nac);
    EXPECT_EQ(n.duid, 0x7);
  }
}

TEST(Nid, DuidTable) {
  EXPECT_EQ(duid_from_nibble(0x5), Duid::Ldu1);
  EXPECT_EQ(duid_from_nibble(0xA), Duid::Ldu2);
  EXPECT_EQ(duid_from_nibble(0x7), Duid::Tsbk);
  EXPECT_EQ(duid_from_nibble(0x0), Duid::Hdu);
  EXPECT_EQ(duid_from_nibble(0x3), Duid::Tdu);
  EXPECT_EQ(duid_from_nibble(0xC), Duid::Pdu);
  EXPECT_EQ(duid_from_nibble(0xF), Duid::TduLc);
}

TEST(Nid, UnassignedDuidsAreRejected) {
  for (std::uint8_t v : {0x1, 0x2, 0x4, 0x6, 0x8, 0x9, 0xB, 0xD, 0xE}) {
    EXPECT_FALSE(duid_from_nibble(v).has_value()) << "nibble " << int(v);
  }
}
