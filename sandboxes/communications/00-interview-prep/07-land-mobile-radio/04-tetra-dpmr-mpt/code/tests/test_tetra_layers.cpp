#include <gtest/gtest.h>

#include "tetra/layers.hpp"

using namespace tetra;

TEST(TetraLayers, Normalise) {
  EXPECT_EQ(normalize_pdu_name("d-tx granted"), "D-TX-GRANTED");
  EXPECT_EQ(normalize_pdu_name("U_SETUP"), "U-SETUP");
  EXPECT_EQ(normalize_pdu_name(""), "");
}

TEST(TetraLayers, MacPdus) {
  EXPECT_EQ(layer_of_pdu("MAC-RESOURCE"), Layer::Mac);
  EXPECT_EQ(layer_of_pdu("MAC-ACCESS"), Layer::Mac);
  EXPECT_EQ(layer_of_pdu("mac-data"), Layer::Mac);
}

TEST(TetraLayers, LlcPdus) {
  EXPECT_EQ(layer_of_pdu("BL-ADATA"), Layer::Llc);
  EXPECT_EQ(layer_of_pdu("BL-ACK"), Layer::Llc);
  EXPECT_EQ(layer_of_pdu("AL-SETUP"), Layer::Llc);
}

TEST(TetraLayers, MlePdus) {
  EXPECT_EQ(layer_of_pdu("D-NWRK-BROADCAST"), Layer::Mle);
  EXPECT_EQ(layer_of_pdu("D-NEW-CELL"), Layer::Mle);
  EXPECT_EQ(layer_of_pdu("U-PREPARE"), Layer::Mle);
}

TEST(TetraLayers, MmPdus) {
  EXPECT_EQ(layer_of_pdu("U-LOCATION UPDATE DEMAND"), Layer::Mm);
  EXPECT_EQ(layer_of_pdu("D-LOCATION UPDATE ACCEPT"), Layer::Mm);
  EXPECT_EQ(layer_of_pdu("U-ATTACH/DETACH GROUP IDENTITY"), Layer::Mm);
  EXPECT_EQ(layer_of_pdu("D-AUTHENTICATION DEMAND"), Layer::Mm);
}

TEST(TetraLayers, CmcePdus) {
  EXPECT_EQ(layer_of_pdu("D-SETUP"), Layer::Cmce);
  EXPECT_EQ(layer_of_pdu("U-SETUP"), Layer::Cmce);
  EXPECT_EQ(layer_of_pdu("D-CONNECT"), Layer::Cmce);
  EXPECT_EQ(layer_of_pdu("U-TX DEMAND"), Layer::Cmce);   // floor request
  EXPECT_EQ(layer_of_pdu("D-TX GRANTED"), Layer::Cmce);  // floor grant
  EXPECT_EQ(layer_of_pdu("D-TX CEASED"), Layer::Cmce);
  EXPECT_EQ(layer_of_pdu("D-SDS-DATA"), Layer::Cmce);
  EXPECT_EQ(layer_of_pdu("U-DISCONNECT"), Layer::Cmce);
}

TEST(TetraLayers, UnknownIsUnknown) {
  EXPECT_EQ(layer_of_pdu("NOT-A-PDU"), Layer::Unknown);
  EXPECT_EQ(layer_of_pdu(""), Layer::Unknown);
  EXPECT_EQ(layer_of_pdu("D"), Layer::Unknown);
}

TEST(TetraLayers, Names) {
  EXPECT_STREQ(layer_name(Layer::Cmce), "CMCE");
  EXPECT_STREQ(layer_name(Layer::Mm), "MM");
}
