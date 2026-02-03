const uint8_t bpHigh[8] = {
  0x92, 0x93, 0x9A, 0x9B, 0xD2, 0xD3, 0xDA, 0xDB
};
const uint8_t bpMid[4] = {
  0x49, 0x4D, 0x69, 0x6D
};
const uint8_t bpLow[8] = {
  0x24, 0x26, 0x34, 0x36, 0xA4, 0xA6, 0xB4, 0xB6
};

void setup() {
  Serial.begin(500000);
  Serial.println();
  Serial.println("Lookup table inverter : ");
  Serial.print("Original = ");
  Serial.println(0b10101010, BIN);
  Serial.print("Inverted = ");
  Serial.println(0b10101010 ^ 0xFF, BIN);
  Serial.println();
  
  Serial.println("  const uint8_t bpHigh[8] = {");
  Serial.print("    ");
  for (uint8_t i = 0; i < 8; i++) {
    Serial.print("0x");
    Serial.print(bpHigh[i] ^ 0xFF, HEX);
    if (i < 7) {
      Serial.print(", ");
    }
    else {
      Serial.println();
    }
  }
  Serial.println("  };");

  Serial.println("  const uint8_t bpMid[4] = {");
  Serial.print("    ");
  for (uint8_t i = 0; i < 4; i++) {
    Serial.print("0x");
    Serial.print(bpMid[i] ^ 0xFF, HEX);
    if (i < 3) {
      Serial.print(", ");
    }
    else {
      Serial.println();
    }
  }
  Serial.println("  };");

  Serial.println("  const uint8_t bpLow[8] = {");
  Serial.print("    ");
  for (uint8_t i = 0; i < 8; i++) {
    Serial.print("0x");
    Serial.print(bpLow[i] ^ 0xFF, HEX);
    if (i < 7) {
      Serial.print(", ");
    }
    else {
      Serial.println();
    }
  }
  Serial.println("  };");

}

void loop() {
  // put your main code here, to run repeatedly:

}
