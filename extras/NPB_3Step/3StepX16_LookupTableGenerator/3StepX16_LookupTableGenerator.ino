#define TABLE_SIZE 12  // 3x 16-bit is 1 bit
                       // 3x 32-bit to represent 2 bits
                       // 2 bits creates 4 options 
#define CONFIG_IDF_TARGET_ESP32S2

uint32_t pDma32[TABLE_SIZE]; //  12 x 32 bit 

void setup() {
  Serial.begin(500000);
  Serial.println();
  Serial.println("Lookup table generator.");
  memset(pDma32, 0x00, TABLE_SIZE * 4);
  CreateLookup3StepX16();
  Serial.println("uint32_t lookupBitty[] = {");
  for (uint8_t i = 0; i < 4; i++) {
    Serial.print(Hex32(pDma32[i * 3]));
    Serial.print(", ");
    Serial.print(Hex32(pDma32[i * 3 + 1]));
    Serial.print(", ");
    Serial.print(Hex32(pDma32[i * 3 + 2]));
    if (i < 3) {
      Serial.print(",    ");
      if (i % 2) {
        Serial.println();
      }
    }
  }
  Serial.println("};");
  Serial.println();
}

void loop() {
}

String Hex32(uint32_t hex) {
  String Hex = String(hex, HEX);
  String H32 = "0x";
  for (uint8_t i = 0; i < 8 - Hex.length(); i++) {
    H32 += '0';
  }
  H32 += Hex;
  return H32;
}

void CreateLookup3StepX16() {
  uint16_t* pDma = reinterpret_cast<uint16_t*>(pDma32);
  const uint16_t muxBit = 0x1;
#if defined(CONFIG_IDF_TARGET_ESP32S2)
  const uint8_t offsetMap[] = { 0, 1, 2, 3 }; // i2s sample is two 16bit values
#else
  const uint8_t offsetMap[] = { 1, 0, 3, 2 }; // i2s sample is two 16bit values
#endif

  uint8_t offset = 0;
  for (uint8_t bitties = 0; bitties < 4; bitties++) {
    uint8_t value = bitties;
    for (uint8_t bit = 0; bit < 2; bit++) {
      pDma[offsetMap[offset]] |= muxBit;
      offset++;
      if (offset > 3) {
        offset %= 4;
        pDma += 4;
      }
      if (value & 0x2) {  // checking if bit 1 is set (instead of bit 7)
        pDma[offsetMap[offset]] |= muxBit;
      }
      offset += 2;
      if (offset > 3) {
        offset %= 4;
        pDma += 4;
      }
      value <<= 1;
    }
  }
}
