#define BUFFERSIZE 679

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
  pixelData = (uint8_t*) malloc(bufferSize + 8);
  uint8_t* pix27 = pixelData;
  for (uint8_t i = 0; i < 27; i++) {
    *(pix27++) = pixelData27[i];
  }

  for (uint16_t i = 27; i < bufferSize + 8; i++) {
    pixelData[i] = pixelData[i - 27];
  }
  uint32_t elaps, startTime;
  Serial.begin(500000);
  //while (Serial);
  Serial.println();
  Serial.println();
  delay(500);
  Serial.println("Encoding Speed Tester");
  Serial.println();

  uint8_t* dmaOutputBuffer4Step;
  dmaOutputBuffer4Step = (uint8_t*) malloc(bufferSize * 4 + 4);
  memset(dmaOutputBuffer4Step, 0x00, bufferSize * 4 + 4);
  uint8_t* dmaOutputBuffer3Step;
  dmaOutputBuffer3Step = (uint8_t*) malloc(bufferSize * 3 + 4);
  memset(dmaOutputBuffer3Step, 0x00, bufferSize * 3 + 4);
  uint8_t* dmaOutputBuffer3StepFast;
  dmaOutputBuffer3StepFast = (uint8_t*) malloc(bufferSize * 3 + 4);
  memset(dmaOutputBuffer3StepFast, 0x00, bufferSize * 3 + 4);

  Serial.println("* 4 Step encoding");
  delay(1000);
  startTime = micros();
  EncodeIntoDma4Step(dmaOutputBuffer4Step, pixelData, bufferSize);
  elaps = micros() - startTime;
  Serial.print("   us Elapsed : ");
  Serial.println(elaps, DEC);
  PrintOutputBuffer(dmaOutputBuffer4Step, 100); //bufferSize * 4 + 4);

  Serial.println();
  Serial.println("* 3 Step encoding");
  delay(1000);
  startTime = micros();
  EncodeIntoDma3Step(dmaOutputBuffer3Step, pixelData, bufferSize);
  elaps = micros() - startTime;
  Serial.print("   us Elapsed : ");
  Serial.println(elaps, DEC);
  PrintOutputBuffer(dmaOutputBuffer3Step, 100); // bufferSize * 3 + 4);
  Serial.println();

  Serial.println("* 3 Step encoding fast");
  delay(1000);
  startTime = micros();
  EncodeIntoDma3StepFast(dmaOutputBuffer3StepFast, pixelData, bufferSize);
  elaps = micros() - startTime;
  Serial.print("   us Elapsed : ");
  Serial.println(elaps, DEC);
  PrintOutputBuffer(dmaOutputBuffer3StepFast, 100); //bufferSize * 3 + 4);
  Serial.println();

  int cmp = memcmp(dmaOutputBuffer3Step, dmaOutputBuffer3StepFast, bufferSize * 3 + 4);
  if (!cmp) {
    Serial.println("Data Blocks compare !!");
  }
  delay(500);
  Serial.end();
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

void EncodeIntoDma4Step(uint8_t* dmaBuffer, const uint8_t* data, size_t sizeData)
{
  const uint16_t bitpatterns[16] =
  {
    0b1000100010001000, 0b1000100010001110, 0b1000100011101000, 0b1000100011101110,
    0b1000111010001000, 0b1000111010001110, 0b1000111011101000, 0b1000111011101110,
    0b1110100010001000, 0b1110100010001110, 0b1110100011101000, 0b1110100011101110,
    0b1110111010001000, 0b1110111010001110, 0b1110111011101000, 0b1110111011101110,
  };

  uint16_t* pDma = reinterpret_cast<uint16_t*>(dmaBuffer);
  const uint8_t* pEnd = data + sizeData;
  for (const uint8_t* pSrc = data; pSrc < pEnd; pSrc++)
  {
    *(pDma++) = bitpatterns[((*pSrc) >> 4) /*& 0x0f*/];
    *(pDma++) = bitpatterns[((*pSrc) & 0x0f)];
  }
}

void EncodeIntoDma3Step(uint8_t* dmaBuffer, const uint8_t* data, size_t sizeData)
{

  const uint16_t OneBit =  0b00000110;
  const uint16_t ZeroBit = 0b00000100;
  const uint8_t SrcBitMask = 0x80;
  const size_t BitsInSample = sizeof(uint16_t) * 8;

  uint16_t* pDma = reinterpret_cast<uint16_t*>(dmaBuffer);
  uint16_t dmaValue = 0;
  uint8_t destBitsLeft = BitsInSample;

  const uint8_t* pSrc = data;
  const uint8_t* pEnd = pSrc + sizeData;

  while (pSrc < pEnd)
  {
    uint8_t value = *(pSrc++);
    for (uint8_t bitSrc = 0; bitSrc < 8; bitSrc++)
    {
      const uint16_t Bit = ((value & SrcBitMask) ? OneBit : ZeroBit);
      if (destBitsLeft > 3)
      {
        destBitsLeft -= 3;
        dmaValue |= Bit << destBitsLeft;  // this is the most time consuming way to do this
        // it is much more efficient to to '|=' first and shift as needed
      }
      else if (destBitsLeft <= 3)
      {
        uint8_t bitSplit = (3 - destBitsLeft);
        dmaValue |= Bit >> bitSplit;
        *(pDma++) = dmaValue;
        dmaValue = 0;

        destBitsLeft = BitsInSample - bitSplit;
        if (bitSplit)
        {
          dmaValue |= Bit << destBitsLeft;
        }
      }
      value <<= 1;
    }
  }
  *pDma++ = dmaValue;
}


void EncodeIntoDma3StepFast3(uint8_t* dmaBuffer, const uint8_t* data, size_t sizeData)
{
  const uint32_t bitpatternsLow[16] =
  {
    0x000924, 0x000926, 0x000934, 0x000936, 0x0009A4, 0x0009A6, 0x0009B4, 0x0009B6,
    0x000D24, 0x000D26, 0x000D34, 0x000D36, 0x000DA4, 0x000DA6, 0x000DB4, 0x000DB6
  };
  const uint32_t bitpatternsHigh[16] =
  {
    0x924000, 0x926000, 0x934000, 0x936000, 0x9A4000, 0x9A6000, 0x9B4000, 0x9B6000,
    0xD24000, 0xD26000, 0xD34000, 0xD36000, 0xDA4000, 0xDA6000, 0xDB4000, 0xDB6000
  };
  uint32_t output[2];  // 2x 24-bit bitPattern
  uint8_t * output8 = reinterpret_cast<uint8_t *>(output);

  uint8_t* pDma = dmaBuffer;
  const uint8_t* pEnd = data + sizeData - 1;  // Encode 2 bytes at a time, make sure they are there
  const uint8_t* pSrc = data;
  while (pSrc < pEnd)
  {
    output[0] = bitpatternsHigh[((*pSrc) >> 4) /*& 0x0f*/] | bitpatternsLow[((*pSrc) & 0x0f)];
    //Serial.println(output[0], BIN);
    pSrc++;
    output[1] = bitpatternsHigh[((*pSrc) >> 4) /*& 0x0f*/] | bitpatternsLow[((*pSrc) & 0x0f)];
    pSrc++;   // note: the mask for the bitpatternsHigh index should be obsolete
    // To get the 2x 3-byte values in the right order copy them Byte by Byte
    memcpy(pDma++, output8 + 1 , 1);
    memcpy(pDma++, output8 + 2 , 1);
    memcpy(pDma++, output8 + 6 , 1);
    memcpy(pDma++, output8 + 0 , 1);
    memcpy(pDma++, output8 + 4 , 1);
    memcpy(pDma++, output8 + 5 , 1);
  }
  if (pSrc == pEnd)  // the last pixelbuffer byte if it exists
  {
    output[0] = bitpatternsHigh[((*pSrc) >> 4) & 0x0f] | bitpatternsLow[((*pSrc) & 0x0f)];
    memcpy(pDma++, output8 + 1 , 1);
    memcpy(pDma++, output8 + 2 , 1);
    pDma++;
    memcpy(pDma++, output8 + 0 , 1);
  }
}

void EncodeIntoDma3StepFast1(uint8_t* dmaBuffer, const uint8_t* data, size_t sizeData) {

  const uint8_t bpHigh[8] = {
    0x92, 0x93, 0x9A, 0x9B, 0xD2, 0xD3, 0xDA, 0xDB
  };
  const uint8_t bpMid[4] = {
    0x49, 0x4D, 0x69, 0x6D
  };
  const uint8_t bpLow[8] = {
    0x24, 0x26, 0x34, 0x36, 0xA4, 0xA6, 0xB4, 0xB6
  };

  uint8_t* pDma = dmaBuffer;
  const uint8_t* pEnd = data + sizeData - 3;  // Encode 4 bytes at a time, make sure they are there
  const uint8_t* pSrc = data;
  uint8_t Src[4]; // used to store 4 data Bytes at a time and for up to 3 separate bytes to complete the frame
  uint32_t* pSrc32 = reinterpret_cast<uint32_t*>(Src);
  uint8_t Source[12]; // to store those 4 Bytes into 3 parts, 3 Low bits, 2 middle bits, 3 Low Bits
  uint32_t* pSource32 = reinterpret_cast<uint32_t*>(Source);

  while (pSrc < pEnd) {
    *(pSrc32) = (*(reinterpret_cast<const uint32_t*>(pSrc)));
    pSrc += 4;

    *(pSource32) = *(pSrc32) & 0x07070707;
    *(pSrc32) = (*(pSrc32) & 0xF8F8F8F8) >> 3;
    *(pSource32 + 1) = *(pSrc32) & 0x03030303;
    *(pSource32 + 2) = (*(pSrc32) & 0xFCFCFCFC) >> 2;

    *(pDma++) = bpMid[Source[4]];
    *(pDma++) = bpHigh[Source[8]];
    *(pDma++) = bpHigh[Source[9]];
    *(pDma++) = bpLow[Source[0]];

    *(pDma++) = bpLow[Source[1]];
    *(pDma++) = bpMid[Source[5]];
    *(pDma++) = bpMid[Source[6]];
    *(pDma++) = bpHigh[Source[10]];

    *(pDma++) = bpHigh[Source[11]];
    *(pDma++) = bpLow[Source[2]];
    *(pDma++) = bpLow[Source[3]];
    *(pDma++) = bpMid[Source[7]];
  }
  //  complete last few source bytes

  pEnd += 3;  // up the endpoint again
  *(pSrc32) = 0; // clear the 32-bit
  uint8_t i = 0;
  while (pSrc < pEnd) {
    Src[i++] = *(pSrc++);  // put them in a byte at a time, not reading any bytes we don't have
  }
  *(pSource32) = *(pSrc32) & 0x07070707;
  *(pSrc32) = (*(pSrc32) & 0xF8F8F8F8) >> 3;
  *(pSource32 + 1) = *(pSrc32) & 0x03030303;
  *(pSource32 + 2) = (*(pSrc32) & 0xFCFCFCFC) >> 2;
  if (i) {
    *(pDma + 3) = bpLow[Source[0]];
    *(pDma) = bpMid[Source[4]];
    *(pDma + 1) = bpHigh[Source[8]];
    i--;
  }
  if (i) {
    *(pDma + 4) = bpLow[Source[1]];
    *(pDma + 5) = bpMid[Source[5]];
    *(pDma + 2) = bpHigh[Source[9]];
    i--;
  }
  if (i) {
    *(pDma + 9) = bpLow[Source[2]];
    *(pDma + 6) = bpMid[Source[6]];
    *(pDma + 7) = bpHigh[Source[10]];
  }
}

void EncodeIntoDma3StepFast(uint8_t* dmaBuffer, const uint8_t* data, size_t sizeData) {

  const uint8_t bpHigh[8] = {
    0x92, 0x93, 0x9A, 0x9B, 0xD2, 0xD3, 0xDA, 0xDB
  };
  const uint8_t bpMid[4] = {
    0x49, 0x4D, 0x69, 0x6D
  };
  const uint8_t bpLow[8] = {
    0x24, 0x26, 0x34, 0x36, 0xA4, 0xA6, 0xB4, 0xB6
  };

  uint8_t* pDma = dmaBuffer;
  const uint8_t* pEnd = data + sizeData - 1;  // Encode 4 bytes at a time, make sure they are there
  const uint8_t* pSrc = data;
  uint8_t Src[2]; // used to store 2 data Bytes at a time and for up to 1 separate byte to complete the frame
  uint16_t* pSrc16 = reinterpret_cast<uint16_t*>(Src);
  uint8_t Source[6]; // to store those 2 Bytes into 3 parts, 3 Low bits, 2 middle bits, 3 Low Bits
  uint16_t* pSource16 = reinterpret_cast<uint16_t*>(Source);

  while (pSrc < pEnd) {
    *(pSrc16) = (*(reinterpret_cast<const uint16_t*>(pSrc)));
    pSrc += 2;

    *(pSource16) = *(pSrc16) & 0x0707;
    *(pSrc16) = (*(pSrc16) & 0xF8F8) >> 3;
    *(pSource16 + 1) = *(pSrc16) & 0x0303;
    *(pSource16 + 2) = (*(pSrc16) & 0xFCFC) >> 2;

    *(pDma++) = bpMid[Source[2]];
    *(pDma++) = bpHigh[Source[4]];
    *(pDma++) = bpHigh[Source[5]];
    *(pDma++) = bpLow[Source[0]];
    *(pDma++) = bpLow[Source[1]];
    *(pDma++) = bpMid[Source[3]];
  }
  //  complete last few source bytes

  pEnd ++;  // up the endpoint again
  *(pSrc16) = 0; // clear the 32-bit
  if (pSrc < pEnd) {
    Src[0] = *(pSrc);  // put them in a byte at a time, not reading any bytes we don't have
    *(pSource16) = *(pSrc16) & 0x0707;// The 8-bit or the 16 bit process is equally quick
    *(pSrc16) = (*(pSrc16) & 0xF8F8) >> 3;
    *(pSource16 + 1) = *(pSrc16) & 0x0303;
    *(pSource16 + 2) = (*(pSrc16) & 0xFCFC) >> 2;

    *(pDma + 3) = bpLow[Source[0]];
    *(pDma) = bpMid[Source[2]];
    *(pDma + 1) = bpHigh[Source[4]];
  }
}

void EncodeIntoDma3StepFast2(uint8_t* dmaBuffer, const uint8_t* data, size_t sizeData)
{
  const uint16_t bitpatterns[6][16] = {
    {
      0x9240, 0x9260, 0x9340, 0x9360, 0x9A40, 0x9A60, 0x9B40, 0x9B60,
      0xD240, 0xD260, 0xD340, 0xD360, 0xDA40, 0xDA60, 0xDB40, 0xDB60
    },
    {
      0x0009, 0x0009, 0x0009, 0x0009, 0x0009, 0x0009, 0x0009, 0x0009,
      0x000D, 0x000D, 0x000D, 0x000D, 0x000D, 0x000D, 0x000D, 0x000D
    },
    {
      0x0092, 0x0092, 0x0093, 0x0093, 0x009A, 0x009A, 0x009B, 0x009B,
      0x00D2, 0x00D2, 0x00D3, 0x00D3, 0x00DA, 0x00DA, 0x00DB, 0x00DB
    },
    {
      0x2400, 0x2600, 0x3400, 0x3600, 0xA400, 0xA600, 0xB400, 0xB600,
      0x2400, 0x2600, 0x3400, 0x3600, 0xA400, 0xA600, 0xB400, 0xB600
    },
    {
      0x4000, 0x6000, 0x4000, 0x6000, 0x4000, 0x6000, 0x4000, 0x6000,
      0x4000, 0x6000, 0x4000, 0x6000, 0x4000, 0x6000, 0x4000, 0x6000
    },
    {
      0x0924, 0x0926, 0x0934, 0x0936, 0x09A4, 0x09A6, 0x09B4, 0x09B6,
      0x0D24, 0x0D26, 0x0D34, 0x0D36, 0x0DA4, 0x0DA6, 0x0DB4, 0x0DB6
    }
  };

  uint16_t* pDma = reinterpret_cast<uint16_t*>(dmaBuffer);
  const uint8_t* pEnd = data + sizeData - 1;  // Encode 2 bytes at a time, make sure they are there
  const uint8_t* pSrc = data;

  while (pSrc < pEnd)
  {
    uint8_t msNibble = ((*pSrc) >> 4) & 0x0f, lsNibble = (*pSrc) & 0x0f;
    *(pDma++) = bitpatterns[0][msNibble] | bitpatterns[1][lsNibble];
    pSrc++;
    msNibble = ((*pSrc) >> 4) & 0x0f;
    *(pDma++) = bitpatterns[2][msNibble] | bitpatterns[3][lsNibble];
    lsNibble = (*pSrc) & 0x0f;
    *(pDma++) = bitpatterns[4][msNibble] | bitpatterns[5][lsNibble];
    pSrc++;
  }
  if (pSrc == pEnd)  // the last pixelbuffer byte if it exists
  {
    uint8_t msNibble = ((*pSrc) >> 4) & 0x0f, lsNibble = (*pSrc) & 0x0f;
    *(pDma++) = bitpatterns[0][msNibble] | bitpatterns[1][lsNibble];
    *(pDma++) = bitpatterns[3][lsNibble];
  }
}
