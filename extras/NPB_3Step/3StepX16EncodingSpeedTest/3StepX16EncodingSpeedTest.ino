#define BUFFERSIZE 680


// by using a 3 step cadence, the dma data can't be updated with a single OR operation as
//    its value resides across a non-uint16_t aligned 3 element type, so it requires two separate OR
//    operations to update a single pixel bit, the last element can be skipped as its always 0
void EncodeIntoDma3StepX16(uint8_t* dmaBuffer, const uint8_t* data, size_t sizeData, uint8_t muxId) {

  uint16_t* pDma = reinterpret_cast<uint16_t*>(dmaBuffer);
  const uint8_t* pValue = data;
  const uint8_t* pEnd = pValue + sizeData;
  const uint16_t muxBit = 0x1 << muxId;
#if defined(CONFIG_IDF_TARGET_ESP32S2)
  const uint8_t offsetMap[] = { 0, 1, 2, 3 }; // i2s sample is two 16bit values
#else
  const uint8_t offsetMap[] = { 1, 0, 3, 2 }; // i2s sample is two 16bit values
#endif
  uint8_t offset = 0;

  while (pValue < pEnd) {
    uint8_t value = *(pValue++);
    for (uint8_t bit = 0; bit < 8; bit++) {
      // first cadence step init to 1
      pDma[offsetMap[offset]] |= muxBit;
      offset++;
      if (offset > 3) {
        offset %= 4;
        pDma += 4;
      }
      // second cadence step set based on bit
      if (value & 0x80) {
        pDma[offsetMap[offset]] |= muxBit;
      }
      offset++;
      // last cadence step already 0, skip it
      offset++;
      if (offset > 3) {
        offset %= 4;
        pDma += 4;
      }
      // Next
      value <<= 1;
    }
  }
}

void EncodeIntoDma3StepX16Fast2(uint8_t* dmaBuffer, const uint8_t* data, size_t sizeData, uint8_t muxId) {

  uint32_t *pDma32 = reinterpret_cast<uint32_t*>(dmaBuffer);
  const uint8_t* pValue = data;
  const uint8_t* pEnd = pValue + sizeData;

#if defined(CONFIG_IDF_TARGET_ESP32S2)
  uint32_t lookupBitty[] = {
    0x00000001, 0x00010000, 0x00000000,    0x00000001, 0x00010000, 0x00000001,
    0x00010001, 0x00010000, 0x00000000,    0x00010001, 0x00010000, 0x00000001
  };
#else
  uint32_t lookupBitty[] = {
    0x00010000, 0x00000001, 0x00000000,    0x00010000, 0x00000001, 0x00010000,
    0x00010001, 0x00000001, 0x00000000,    0x00010001, 0x00000001, 0x00010000
  };
#endif

  for (uint8_t i = 0; i < 12; i++) {  // shift the table to the proper bit
    lookupBitty[i] <<= muxId;
  }

  while (pValue < pEnd) {
    uint8_t value = *(pValue++);
    uint8_t bitty4 = (value & 0x3) * 3;
    value >>= 2;
    uint8_t bitty3 = (value & 0x3) * 3;
    value >>= 2;
    uint8_t bitty2 = (value & 0x3) * 3;
    value >>= 2;
    uint8_t bitty1 = value * 3;

    *(pDma32++) |= lookupBitty[bitty1++];
    *(pDma32++) |= lookupBitty[bitty1++];
    *(pDma32++) |= lookupBitty[bitty1];

    *(pDma32++) |= lookupBitty[bitty2++];
    *(pDma32++) |= lookupBitty[bitty2++];
    *(pDma32++) |= lookupBitty[bitty2];

    *(pDma32++) |= lookupBitty[bitty3++];
    *(pDma32++) |= lookupBitty[bitty3++];
    *(pDma32++) |= lookupBitty[bitty3];

    *(pDma32++) |= lookupBitty[bitty4++];
    *(pDma32++) |= lookupBitty[bitty4++];
    *(pDma32++) |= lookupBitty[bitty4];
  }
}

void EncodeIntoDma3StepX16Fast1(uint8_t* dmaBuffer, const uint8_t* data, size_t sizeData, uint8_t muxId) {

  uint32_t *pDma32 = reinterpret_cast<uint32_t*>(dmaBuffer);
  const uint8_t* pValue = data;
  const uint8_t* pEnd = pValue + sizeData;

#if defined(CONFIG_IDF_TARGET_ESP32S2)
  uint32_t lookupBitty[] = {
    0x00000001, 0x00010000, 0x00000000, 0,   0x00000001, 0x00010000, 0x00000001, 0,
    0x00010001, 0x00010000, 0x00000000, 0,   0x00010001, 0x00010000, 0x00000001
  };
#else
  uint32_t lookupBitty[] = {
    0x00010000, 0x00000001, 0x00000000, 0,   0x00010000, 0x00000001, 0x00010000, 0,
    0x00010001, 0x00000001, 0x00000000, 0,   0x00010001, 0x00000001, 0x00010000
  };
#endif

  for (uint8_t i = 0; i < 15; i++) {  // shift the table to the proper bit
    lookupBitty[i] <<= muxId;
  }

  while (pValue < pEnd) {
    uint8_t value = *(pValue++);
    uint8_t bitty4 = (value & 0x3) << 2;
    uint8_t bitty3 = value & 0xC;
    value >>= 2;
    uint8_t bitty2 = value & 0xC;
    value >>= 2;
    uint8_t bitty1 = value & 0xC;

    *(pDma32++) |= lookupBitty[bitty1++];
    *(pDma32++) |= lookupBitty[bitty1++];
    *(pDma32++) |= lookupBitty[bitty1];

    *(pDma32++) |= lookupBitty[bitty2++];
    *(pDma32++) |= lookupBitty[bitty2++];
    *(pDma32++) |= lookupBitty[bitty2];

    *(pDma32++) |= lookupBitty[bitty3++];
    *(pDma32++) |= lookupBitty[bitty3++];
    *(pDma32++) |= lookupBitty[bitty3];

    *(pDma32++) |= lookupBitty[bitty4++];
    *(pDma32++) |= lookupBitty[bitty4++];
    *(pDma32++) |= lookupBitty[bitty4];
  }
}

void EncodeIntoDma3StepX16Fast(uint8_t* dmaBuffer, const uint8_t* data, size_t sizeData, uint8_t muxId) {

  uint32_t *pDma32 = reinterpret_cast<uint32_t*>(dmaBuffer);
  const uint8_t* pValue = data;
  const uint8_t* pEnd = pValue + sizeData;
  uint8_t bitty[4];

#if defined(CONFIG_IDF_TARGET_ESP32S2)
  uint32_t lookupBitty[] = {
    0x00000001, 0x00010000, 0x00000000, 0x0,   0x00000001, 0x00010000, 0x00000001, 0x0,
    0x00010001, 0x00010000, 0x00000000, 0x0,   0x00010001, 0x00010000, 0x00000001
  };

#else
  uint32_t lookupBitty[] = {
    0x00010000, 0x00000001, 0x00000000, 0x0,   0x00010000, 0x00000001, 0x00010000, 0x0,
    0x00010001, 0x00000001, 0x00000000, 0x0,   0x00010001, 0x00000001, 0x00010000
  };
#endif

  for (uint8_t i = 0; i < 15; i++) {  // shift the table to the proper bit
    lookupBitty[i] <<= muxId;
  }

  while (pValue < pEnd) {
    uint8_t value = *(pValue++);
    bitty[3] = (value & 0x3) << 2;
    bitty[2] = value & 0xC;
    value >>= 2;
    bitty[1] = value & 0xC;
    value >>= 2;
    bitty[0] = value & 0xC;

    *(pDma32++) |= lookupBitty[bitty[0]++];
    *(pDma32++) |= lookupBitty[bitty[0]++];
    *(pDma32++) |= lookupBitty[bitty[0]];

    *(pDma32++) |= lookupBitty[bitty[1]++];
    *(pDma32++) |= lookupBitty[bitty[1]++];
    *(pDma32++) |= lookupBitty[bitty[1]];

    *(pDma32++) |= lookupBitty[bitty[2]++];
    *(pDma32++) |= lookupBitty[bitty[2]++];
    *(pDma32++) |= lookupBitty[bitty[2]];

    *(pDma32++) |= lookupBitty[bitty[3]++];
    *(pDma32++) |= lookupBitty[bitty[3]++];
    *(pDma32++) |= lookupBitty[bitty[3]];
  }
}


void setup() {

#if defined(CONFIG_IDF_TARGET_ESP32S2)
  pinMode(15, OUTPUT);
  digitalWrite(15, HIGH);
  delay(5000);
  digitalWrite(15, LOW);
#endif

  uint16_t bufferSize = BUFFERSIZE;
  uint8_t pixelData27[27] = {0xfe, 0x68, 0x12, 0xdc, 0x86, 0x34, 0xdc, 0x86, 0x34, 0xfe, 0x68,
                             0x12, 0xdc, 0x86, 0x34, 0xdc, 0x86, 0x34, 0xfe, 0x68, 0x12, 0xdc,
                             0x86, 0x34, 0xdc, 0x86, 0x34
                            };

  uint8_t* pixelData;
  pixelData = (uint8_t*) malloc(bufferSize + 16);
  uint8_t* pix27 = pixelData;
  for (uint8_t i = 0; i < 27; i++) {
    *(pix27++) = pixelData27[i];
  }

  for (uint16_t i = 27; i < bufferSize + 8; i++) {
    pixelData[i] = pixelData[i - 27];
  }
  uint32_t elaps, startTime;

  uint8_t* dmaOutputBuffer3StepX16;
  dmaOutputBuffer3StepX16 = (uint8_t*) malloc(bufferSize * 3 * 16 + 4);
  memset(dmaOutputBuffer3StepX16, 0x00, bufferSize * 3 * 16 + 4);

  uint8_t* dmaOutputBuffer3StepX16Fast;
  dmaOutputBuffer3StepX16Fast = (uint8_t*) malloc(bufferSize * 3 * 16 + 4);
  memset(dmaOutputBuffer3StepX16Fast, 0x00, bufferSize * 3 * 16 + 4);

  Serial.begin(500000);
  Serial.println();
  Serial.println();
  delay(500);
  Serial.println("Encoding Speed Tester");
  Serial.println();

  Serial.println();
  Serial.println("* 3 Step X16 encoding");
  delay(1000);
  startTime = micros();
  for (uint8_t i = 0; i < 16; i++) {
    EncodeIntoDma3StepX16(dmaOutputBuffer3StepX16, pixelData + i, bufferSize, i);
  }
  elaps = micros() - startTime;
  Serial.print("   us Elapsed : ");
  Serial.println(elaps, DEC);
  PrintOutputBuffer(dmaOutputBuffer3StepX16, 100); //bufferSize * 3 * 16 + 4);
  Serial.println();

  Serial.println("* 3 Step X16 encoding fast");
  delay(1000);
  startTime = micros();
  for (uint8_t i = 0; i < 16; i++) {
    EncodeIntoDma3StepX16Fast(dmaOutputBuffer3StepX16Fast, pixelData + i, bufferSize, i);
  }
  elaps = micros() - startTime;
  Serial.print("   us Elapsed : ");
  Serial.println(elaps, DEC);
  PrintOutputBuffer(dmaOutputBuffer3StepX16Fast, 100); //bufferSize * 3 * 16 + 4);
  Serial.println();

  int cmp = memcmp(dmaOutputBuffer3StepX16, dmaOutputBuffer3StepX16Fast, bufferSize * 3 * 16 + 4);
  if (!cmp) {
    Serial.println("Data Blocks compare !!");
  }

#if defined(CONFIG_IDF_TARGET_ESP32S2)
  digitalWrite(15, HIGH);
#endif
}

void PrintOutputBuffer(uint8_t * outputBuf, uint32_t bufferSize) {
  for (uint32_t i = 0; i < bufferSize; i++) {
    Serial.print("0x");
    String s =  String(*(outputBuf++), HEX);
    while (s.length() < 2) {
      s = "0" + s;
    }
    Serial.print(s);
    if (i < bufferSize - 1) {
      Serial.print(", ");
    }
    if (!((i + 1) % 24)) {
      Serial.println();
    }
  }
  Serial.println();
  Serial.println();
}

void loop() {}
