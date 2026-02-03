#define BUFFERSIZE 680
//#define INVERTED_METHOD
#ifndef ARDUINO_ARCH_ESP8266
  #error **Sketch meant for ESP8266**
#endif

void setup() {

  uint16_t bufferSize = BUFFERSIZE;
  uint8_t pixelData27[27] = {0xfe, 0x68, 0x12, 0xdc, 0x86, 0x34, 0xdc, 0x86, 0x34, 0xfe, 0x68,
                             0x12, 0xdc, 0x86, 0x34, 0xdc, 0x86, 0x34, 0xfe, 0x68, 0x12, 0xdc,
                             0x86, 0x34, 0xdc, 0x86, 0x34
                            };

  uint8_t* pixelData;
  pixelData = (uint8_t*) malloc(bufferSize);
  uint8_t* pix27 = pixelData;
  for (uint8_t i = 0; i < 27; i++) {
    *(pix27++) = pixelData27[i];
  }

  for (uint16_t i = 27; i < bufferSize; i++) {
    pixelData[i] = pixelData[i - 27];
  }
  uint32_t elaps, startTime;

  Serial.begin(500000);
  Serial.println();
  Serial.println();
  delay(500);
  Serial.println("Encoding Speed Tester");
  Serial.println();
  Serial.print("F_CPU at : ");
  Serial.println(F_CPU, DEC);
  Serial.println();
#if defined (INVERTED_METHOD)
  Serial.println("Inverted method");
#else
  Serial.println("Normal method");
#endif
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
  NeoEsp8266Dma4StepEncode(dmaOutputBuffer4Step, pixelData, bufferSize);
  elaps = micros() - startTime;
  Serial.print("   us Elapsed : ");
  Serial.println(elaps, DEC);
  PrintOutputBuffer(dmaOutputBuffer4Step, 30); //bufferSize * 4 + 4);

  Serial.println();
  Serial.println("* 3 Step encoding");
  delay(1000);
  startTime = micros();
  NeoEsp8266Dma3StepEncode(dmaOutputBuffer3Step, pixelData, bufferSize);
  elaps = micros() - startTime;
  Serial.print("   us Elapsed : ");
  Serial.println(elaps, DEC);
  PrintOutputBuffer(dmaOutputBuffer3Step, 30); //bufferSize * 3 + 4);
  Serial.println();

  Serial.println("* 3 Step encoding fast");
  delay(1000);
  startTime = micros();
  NeoEsp8266Dma3StepEncodeFast(dmaOutputBuffer3StepFast, pixelData, bufferSize);
  elaps = micros() - startTime;
  Serial.print("   us Elapsed : ");
  Serial.println(elaps, DEC);
  PrintOutputBuffer(dmaOutputBuffer3StepFast, 30); //bufferSize * 3 + 4);
  Serial.println();

  int cmp = memcmp(dmaOutputBuffer3Step, dmaOutputBuffer3StepFast, bufferSize * 3 + 4);
  if (!cmp) {
    Serial.println("Data Blocks compare !!");
  }
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
  //ShowBitPatterns();
}

void loop() {}


void NeoEsp8266Dma4StepEncode(uint8_t* i2sBuffer, const uint8_t* data, size_t sizeData) {
#if defined (INVERTED_METHOD)
  const uint16_t bitpatterns[16] = {
    0b0111011101110111, 0b0111011101110001, 0b0111011100010111, 0b0111011100010001,
    0b0111000101110111, 0b0111000101110001, 0b0111000100010111, 0b0111000100010001,
    0b0001011101110111, 0b0001011101110001, 0b0001011100010111, 0b0001011100010001,
    0b0001000101110111, 0b0001000101110001, 0b0001000100010111, 0b0001000100010001,
  };
#else
  const uint16_t bitpatterns[16] = {
    0b1000100010001000, 0b1000100010001110, 0b1000100011101000, 0b1000100011101110,
    0b1000111010001000, 0b1000111010001110, 0b1000111011101000, 0b1000111011101110,
    0b1110100010001000, 0b1110100010001110, 0b1110100011101000, 0b1110100011101110,
    0b1110111010001000, 0b1110111010001110, 0b1110111011101000, 0b1110111011101110,
  };
#endif
  uint16_t* pDma = (uint16_t*)i2sBuffer;
  const uint8_t* pEnd = data + sizeData;
  for (const uint8_t* pData = data; pData < pEnd; pData++) {
    *(pDma++) = bitpatterns[(*pData) & 0x0f];
    *(pDma++) = bitpatterns[((*pData) >> 4) & 0x0f];
  }
}

void NeoEsp8266Dma3StepEncode(uint8_t* i2sBuffer, const uint8_t* data, size_t sizeData) {
  const uint8_t SrcBitMask = 0x80;
  const size_t BitsInSample = sizeof(uint32_t) * 8;
#if defined (INVERTED_METHOD)
  const uint16_t OneBit3Step =  0b00000001;
  const uint16_t ZeroBit3Step = 0b00000011;
#else
  const uint16_t OneBit3Step = 0b00000110;
  const uint16_t ZeroBit3Step = 0b00000100;
#endif

  uint32_t* pDma = reinterpret_cast<uint32_t*>(i2sBuffer);
  uint32_t dmaValue = 0;
  uint8_t destBitsLeft = BitsInSample;

  const uint8_t* pSrc = data;
  const uint8_t* pEnd = pSrc + sizeData;

  while (pSrc < pEnd) {
    uint8_t value = *(pSrc++);

    for (uint8_t bitSrc = 0; bitSrc < 8; bitSrc++) {
      const uint16_t Bit = ((value & SrcBitMask) ? OneBit3Step : ZeroBit3Step);
      if (destBitsLeft > 3) {
        destBitsLeft -= 3;
        dmaValue |= Bit << destBitsLeft;
      }
      else if (destBitsLeft <= 3) {
        uint8_t bitSplit = (3 - destBitsLeft);
        dmaValue |= Bit >> bitSplit;
        *(pDma++) = dmaValue;
        dmaValue = 0;
        destBitsLeft = BitsInSample - bitSplit;
        if (bitSplit) {
          dmaValue |= Bit << destBitsLeft;
        }
      }
      value <<= 1;
    }
  }
  *pDma++ = dmaValue;
}

void NeoEsp8266Dma3StepEncodeFast(uint8_t* i2sBuffer, const uint8_t* data, size_t sizeData) {

#if defined (INVERTED_METHOD)
  const uint8_t bpHigh[8] = {
    0x6D, 0x6C, 0x65, 0x64, 0x2D, 0x2C, 0x25, 0x24
  };
  const uint8_t bpMid[4] = {
    0xB6, 0xB2, 0x96, 0x92
  };
  const uint8_t bpLow[8] = {
    0xDB, 0xD9, 0xCB, 0xC9, 0x5B, 0x59, 0x4B, 0x49
  };
#else
  const uint8_t bpHigh[8] = {
    0x92, 0x93, 0x9A, 0x9B, 0xD2, 0xD3, 0xDA, 0xDB
  };
  const uint8_t bpMid[4] = {
    0x49, 0x4D, 0x69, 0x6D
  };
  const uint8_t bpLow[8] = {
    0x24, 0x26, 0x34, 0x36, 0xA4, 0xA6, 0xB4, 0xB6
  };
#endif

  uint8_t* pDma = i2sBuffer;
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

    *(pDma++) = bpHigh[Source[9]];
    *(pDma++) = bpLow[Source[0]];
    *(pDma++) = bpMid[Source[4]];
    *(pDma++) = bpHigh[Source[8]];

    *(pDma++) = bpMid[Source[6]];
    *(pDma++) = bpHigh[Source[10]];
    *(pDma++) = bpLow[Source[1]];
    *(pDma++) = bpMid[Source[5]];

    *(pDma++) = bpLow[Source[3]];
    *(pDma++) = bpMid[Source[7]];
    *(pDma++) = bpHigh[Source[11]];
    *(pDma++) = bpLow[Source[2]];
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
    *(pDma + 1) = bpLow[Source[0]];
    *(pDma + 2) = bpMid[Source[4]];
    *(pDma + 3) = bpHigh[Source[8]];
    i--;    
  }
  if (i) {
    *(pDma) = bpHigh[Source[9]];
    *(pDma + 6) = bpLow[Source[1]];
    *(pDma + 7) = bpMid[Source[5]];
    i--;
  }
  if (i) {
    *(pDma + 4) = bpMid[Source[6]];
    *(pDma + 5) = bpHigh[Source[10]];
    *(pDma + 11) = bpLow[Source[2]];
  }
}

void NeoEsp8266Dma3StepEncodeFast1(uint8_t* i2sBuffer, const uint8_t* data, size_t sizeData) {

  uint32_t bitpatternsHigh0[16] = {
    0x92400000, 0x92600000, 0x93400000, 0x93600000, 0x9A400000, 0x9A600000, 0x9B400000, 0x9B600000,
    0xD2400000, 0xD2600000, 0xD3400000, 0xD3600000, 0xDA400000, 0xDA600000, 0xDB400000, 0xDB600000
  };

  uint32_t bitpatternsLow0[16] = {
    0x00092400, 0x00092600, 0x00093400, 0x00093600, 0x0009A400, 0x0009A600, 0x0009B400, 0x0009B600,
    0x000D2400, 0x000D2600, 0x000D3400, 0x000D3600, 0x000DA400, 0x000DA600, 0x000DB400, 0x000DB600
  };

  const uint8_t bitpatternsHigh1a[16] = {
    0x92, 0x92, 0x93, 0x93, 0x9A, 0x9A, 0x9B, 0x9B,
    0xD2, 0xD2, 0xD3, 0xD3, 0xDA, 0xDA, 0xDB, 0xDB
  };

  const uint32_t bitpatternsHigh1b[16] = {
    0x40000000, 0x60000000, 0x40000000, 0x60000000, 0x40000000, 0x60000000, 0x40000000, 0x60000000,
    0x40000000, 0x60000000, 0x40000000, 0x60000000, 0x40000000, 0x60000000, 0x40000000, 0x60000000
  };

  const uint32_t bitpatternsLow1[16] = {
    0x09240000, 0x09260000, 0x09340000, 0x09360000, 0x09A40000, 0x09A60000, 0x09B40000, 0x09B60000,
    0x0D240000, 0x0D260000, 0x0D340000, 0x0D360000, 0x0DA40000, 0x0DA60000, 0x0DB40000, 0x0DB60000
  };

  const uint16_t bitpatternsHigh2[16] = {
    0x9240, 0x9260, 0x9340, 0x9360, 0x9A40, 0x9A60, 0x9B40, 0x9B60,
    0xD240, 0xD260, 0xD340, 0xD360, 0xDA40, 0xDA60, 0xDB40, 0xDB60
  };

  const uint8_t bitpatternsLow2a[16] = {
    0x9, 0x9, 0x9, 0x9, 0x9, 0x9, 0x9, 0x9,
    0xD, 0xD, 0xD, 0xD, 0xD, 0xD, 0xD, 0xD
  };

  const uint32_t bitpatternsLow2b[16] = {
    0x24000000, 0x26000000, 0x34000000, 0x36000000, 0xA4000000, 0xA6000000, 0xB4000000, 0xB6000000,
    0x24000000, 0x26000000, 0x34000000, 0x36000000, 0xA4000000, 0xA6000000, 0xB4000000, 0xB6000000
  };

  const uint32_t bitpatternsHigh3[16] = {
    0x924000, 0x926000, 0x934000, 0x936000, 0x9A4000, 0x9A6000, 0x9B4000, 0x9B6000,
    0xD24000, 0xD26000, 0xD34000, 0xD36000, 0xDA4000, 0xDA6000, 0xDB4000, 0xDB6000
  };

  const uint16_t bitpatternsLow3[16] = {
    0x924, 0x926, 0x934, 0x936, 0x9A4, 0x9A6, 0x9B4, 0x9B6,
    0xD24, 0xD26, 0xD34, 0xD36, 0xDA4, 0xDA6, 0xDB4, 0xDB6
  };

  uint32_t* pDma32 = reinterpret_cast<uint32_t *>(i2sBuffer);
  const uint8_t* pEnd = data + sizeData - 3;  // Encode 4 bytes at a time, make sure they are there
  const uint8_t* pSrc = data;
  while (pSrc < pEnd) {
    *(pDma32) = bitpatternsHigh0[((*pSrc) >> 4)] | bitpatternsLow0[((*pSrc) & 0x0f)];
    pSrc++;
    *(pDma32++) |= bitpatternsHigh1a[((*pSrc) >> 4)];
    *(pDma32) = bitpatternsHigh1b[((*pSrc) >> 4)] | bitpatternsLow1[((*pSrc) & 0x0f)];
    pSrc++;
    *(pDma32++) |= bitpatternsHigh2[((*pSrc) >> 4)] | bitpatternsLow2a[((*pSrc) & 0x0f)];
    *(pDma32) = bitpatternsLow2b[((*pSrc) & 0x0f)];
    pSrc++;
    *(pDma32++) |= bitpatternsHigh3[((*pSrc) >> 4)] | bitpatternsLow3[((*pSrc) & 0x0f)];
    pSrc++;
  }
  pEnd += 3;
  if (pSrc < pEnd) {
    *(pDma32) = bitpatternsHigh0[((*pSrc) >> 4)] | bitpatternsLow0[((*pSrc) & 0x0f)];
    pSrc++;
  }
  if (pSrc < pEnd) {
    *(pDma32++) |= bitpatternsHigh1a[((*pSrc) >> 4)];
    *(pDma32) = bitpatternsHigh1b[((*pSrc) >> 4)] | bitpatternsLow1[((*pSrc) & 0x0f)];
    pSrc++;
  }
  if (pSrc < pEnd) {
    *(pDma32++) |= bitpatternsHigh2[((*pSrc) >> 4)] | bitpatternsLow2a[((*pSrc) & 0x0f)];
    *(pDma32) = bitpatternsLow2b[((*pSrc) & 0x0f)];
  }
}

void NeoEsp8266Dma3StepEncodeFast2(uint8_t* i2sBuffer, const uint8_t* data, size_t sizeData) {

  const uint8_t bpHigh[8] = {
    0x92, 0x93, 0x9A, 0x9B, 0xD2, 0xD3, 0xDA, 0xDB
  };
  const uint8_t bpMid[4] = {
    0x49, 0x4D, 0x69, 0x6D
  };
  const uint8_t bpLow[8] = {
    0x24, 0x26, 0x34, 0x36, 0xA4, 0xA6, 0xB4, 0xB6
  };

  uint8_t* pDma = i2sBuffer;
  const uint8_t* pEnd = data + sizeData - 3;  // Encode 4 bytes at a time, make sure they are there
  const uint8_t* pSrc = data;
  uint8_t Src[4];
  uint32_t* pSrc32 = reinterpret_cast<uint32_t*>(Src);
  uint8_t Source[4][3];
  uint32_t* pSource32 = reinterpret_cast<uint32_t*>(Source);

  while (pSrc < pEnd) {
    *(pSrc32) = (*(reinterpret_cast<const uint32_t*>(pSrc)));
    pSrc += 4;
    for (uint8_t i = 0; i < 4; i++) {
      Source[i][0] = Src[i] & 0x7;
      Src[i] = Src[i] >> 3;
      Source[i][1] = Src[i] & 0x3;
      Source[i][2] = Src[i] >> 2;
    }

    *(pDma++) = bpHigh[Source[1][2]];
    *(pDma++) = bpLow[Source[0][0]];
    *(pDma++) = bpMid[Source[0][1]];
    *(pDma++) = bpHigh[Source[0][2]];

    *(pDma++) = bpMid[Source[2][1]];
    *(pDma++) = bpHigh[Source[2][2]];
    *(pDma++) = bpLow[Source[1][0]];
    *(pDma++) = bpMid[Source[1][1]];

    *(pDma++) = bpLow[Source[3][0]];
    *(pDma++) = bpMid[Source[3][1]];
    *(pDma++) = bpHigh[Source[3][2]];
    *(pDma++) = bpLow[Source[2][0]];
  }
}

/*
void ShowBitPatterns() {
  const uint8_t pSource[8] = {0x1A, 0x2A, 0x3A, 0x4A, 0x1B, 0x2B, 0x3B, 0x4B};
  const uint8_t* pSrc = (pSource);
  uint8_t Src[4];
  uint32_t* pSrc32 = reinterpret_cast<uint32_t*>(Src);
  uint32_t valu;

  *(pSrc32) = (*(reinterpret_cast<const uint32_t*>(pSrc)));
  pSrc += 4;
  valu = Src[0];
  Serial.print("valu : 0x");
  Serial.println(valu, HEX);

  *(pSrc32) = (*(reinterpret_cast<const uint32_t*>(pSrc)));
  valu = Src[2];
  Serial.print("valu : 0x");
  Serial.println(valu, HEX);

}

void ShowBitPatterns() {

  const uint8_t bitpatternsHigh8[83] = {
    0x00, 0x00, 0x00, 0x40, 0x92,   0x00, 0x00, 0x00, 0x60, 0x92,
    0x00, 0x00, 0x00, 0x40, 0x93,   0x00, 0x00, 0x00, 0x60, 0x93,
    0x00, 0x00, 0x00, 0x40, 0x9A,   0x00, 0x00, 0x00, 0x60, 0x9A,
    0x00, 0x00, 0x00, 0x40, 0x9B,   0x00, 0x00, 0x00, 0x60, 0x9B,

    0x00, 0x00, 0x00, 0x40, 0xD2,   0x00, 0x00, 0x00, 0x60, 0xD2,
    0x00, 0x00, 0x00, 0x40, 0xD3,   0x00, 0x00, 0x00, 0x60, 0xD3,
    0x00, 0x00, 0x00, 0x40, 0xDA,   0x00, 0x00, 0x00, 0x60, 0xDA,
    0x00, 0x00, 0x00, 0x40, 0xDB,   0x00, 0x00, 0x00, 0x60, 0xDB,
    0x00, 0x00, 0x00
  };

  const uint8_t bitpatternsLow8[83] = {
    0x00, 0x00, 0x00, 0x24, 0x09,   0x00, 0x00, 0x00, 0x26, 0x09,
    0x00, 0x00, 0x00, 0x34, 0x09,   0x00, 0x00, 0x00, 0x36, 0x09,
    0x00, 0x00, 0x00, 0xA4, 0x09,   0x00, 0x00, 0x00, 0xA6, 0x09,
    0x00, 0x00, 0x00, 0xB4, 0x09,   0x00, 0x00, 0x00, 0xB6, 0x09,

    0x00, 0x00, 0x00, 0x24, 0x0D,   0x00, 0x00, 0x00, 0x26, 0x0D,
    0x00, 0x00, 0x00, 0x34, 0x0D,   0x00, 0x00, 0x00, 0x36, 0x0D,
    0x00, 0x00, 0x00, 0xA4, 0x0D,   0x00, 0x00, 0x00, 0xA6, 0x0D,
    0x00, 0x00, 0x00, 0xB4, 0x0D,   0x00, 0x00, 0x00, 0xB6, 0x0D,
    0x00, 0x00, 0x00
  };


  const uint32_t bitpatternsHigh[18] = {
    0, 0x924000, 0x926000, 0x934000, 0x936000, 0x9A4000, 0x9A6000, 0x9B4000, 0x9B6000,
    0xD24000, 0xD26000, 0xD34000, 0xD36000, 0xDA4000, 0xDA6000, 0xDB4000, 0xDB6000, 0
  };

  const uint32_t bitpatternsLow[18] = {
    0, 0x000924, 0x000926, 0x000934, 0x000936, 0x0009A4, 0x0009A6, 0x0009B4, 0x0009B6,
    0x000D24, 0x000D26, 0x000D34, 0x000D36, 0x000DA4, 0x000DA6, 0x000DB40, 0x000DB6, 0
  };
  uint16_t temp[2]; // a 32 bit buffer
  uint32_t* temp32 = reinterpret_cast<uint32_t*>(temp);
  uint8_t* temp8msB = reinterpret_cast<uint8_t*>(temp) + 3;

  //const uint32_t* bpHigh0 = reinterpret_cast<const uint32_t*>(reinterpret_cast<const uint8_t*>(bitpatternsHigh) + 3);
  const uint32_t* bpHigh0 = reinterpret_cast<const uint32_t*>(bitpatternsHigh8 + 1);
  //const uint32_t* bpLow0 = reinterpret_cast<const uint32_t*>(reinterpret_cast<const uint8_t*>(bitpatternsLow) + 3);
  const uint32_t* bpLow0 = reinterpret_cast<const uint32_t*>(bitpatternsLow8 + 2);
  //const uint8_t* bpHigh1a = reinterpret_cast<const uint8_t*>(bitpatternsHigh) + 6;
  const uint32_t* bpHigh1a = reinterpret_cast<const uint32_t*>(bitpatternsHigh8 + 1);
  const uint8_t* bpHigh1b = bpHigh1a - 1;
  const uint32_t* bpLow1 = reinterpret_cast<const uint32_t*>(reinterpret_cast<const uint8_t*>(bitpatternsLow) + 2);
  const uint32_t* bpHigh2 = reinterpret_cast<const uint32_t*>(reinterpret_cast<const uint8_t*>(bitpatternsHigh) + 5);


  //uint32_t valu = *(bpHigh0 + 7); // for understanding the syntax
  //uint32_t valu = *(bpLow0 + 15);
  uint32_t valu = *(bpHigh1a);
  //uint32_t valu = *(bpHigh1a + (15 << 2));  // since the High is actually >> 4 we will
                                              // end up doing ( (byte >> 2) & 0x3)
  //uint32_t valu = *(bpHigh1b + (3 << 2)); // the resulting byte needs to be the msb of another
                                            // uint32_t temp  that we memcpy to the msB
                                            // and then 'or' the other 3 values
  //uint32_t valu = *(bpLow1 + 11);
  //uint32_t valu = *(bpHigh2 + 15);

  Serial.print("valu : 0x");
  Serial.println(valu, HEX);

  }*/
