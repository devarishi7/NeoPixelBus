#define BUFFERSIZE 680
#define _WS2812_
//#define _DMX512_
//#define _USE32BIT_
//#define _WS2812X8_
//#define CONFIG_IDF_TARGET_ESP32S2




#ifdef _WS2812X8_

// by using a 3 step cadence, the dma data can't be updated with a single OR operation as
//    its value resides across a non-uint16_t aligned 3 element type, so it requires two separate OR
//    operations to update a single pixel bit, the last element can be skipped as its always 0
void EncodeIntoDma3StepX8(uint8_t* dmaBuffer, const uint8_t* data, size_t sizeData, uint8_t muxId) {

  uint8_t* pDma = dmaBuffer;
  const uint8_t* pValue = data;
  const uint8_t* pEnd = pValue + sizeData;
  const uint8_t muxBit = 0x1 << muxId;
#if defined(CONFIG_IDF_TARGET_ESP32S2)
  const uint8_t offsetMap[] = { 0, 1, 2, 3 }; // i2s sample is two 16bit values
#else
  const uint8_t offsetMap[] = { 2, 3, 0, 1 }; // i2s sample is two 16bit values
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
      if (value & 0x80) {  // checking is bit 7 is set
        pDma[offsetMap[offset]] |= muxBit;
      }
      // last cadence step already init to 0, skip it
      offset += 2;
      if (offset > 3) {
        offset %= 4;
        pDma += 4;
      }
      // Next bit.
      value <<= 1;
    }
  }
}


void EncodeIntoDma3StepX8Fast(uint8_t* dmaBuffer, const uint8_t* data, size_t sizeData, uint8_t muxId) {

  uint32_t *pDma32 = reinterpret_cast<uint32_t*>(dmaBuffer);
  const uint8_t* pValue = data;
  const uint8_t* pEnd = pValue + sizeData;

#if defined(CONFIG_IDF_TARGET_ESP32S2)
  uint32_t lookupNibble[] = {
    0x01000001, 0x00010000, 0x00000100,    0x01000001, 0x00010000, 0x00010100,
    0x01000001, 0x01010000, 0x00000100,    0x01000001, 0x01010000, 0x00010100,
    0x01000001, 0x00010001, 0x00000100,    0x01000001, 0x00010001, 0x00010100,
    0x01000001, 0x01010001, 0x00000100,    0x01000001, 0x01010001, 0x00010100,
    0x01000101, 0x00010000, 0x00000100,    0x01000101, 0x00010000, 0x00010100,
    0x01000101, 0x01010000, 0x00000100,    0x01000101, 0x01010000, 0x00010100,
    0x01000101, 0x00010001, 0x00000100,    0x01000101, 0x00010001, 0x00010100,
    0x01000101, 0x01010001, 0x00000100,    0x01000101, 0x01010001, 0x00010100
  };
#else
  uint32_t lookupNibble[] = {
    0x00010100, 0x00000001, 0x01000000,    0x00010100, 0x00000001, 0x01000001,
    0x00010100, 0x00000101, 0x01000000,    0x00010100, 0x00000101, 0x01000001,
    0x00010100, 0x00010001, 0x01000000,    0x00010100, 0x00010001, 0x01000001,
    0x00010100, 0x00010101, 0x01000000,    0x00010100, 0x00010101, 0x01000001,
    0x01010100, 0x00000001, 0x01000000,    0x01010100, 0x00000001, 0x01000001,
    0x01010100, 0x00000101, 0x01000000,    0x01010100, 0x00000101, 0x01000001,
    0x01010100, 0x00010001, 0x01000000,    0x01010100, 0x00010001, 0x01000001,
    0x01010100, 0x00010101, 0x01000000,    0x01010100, 0x00010101, 0x01000001
  };
#endif

  for (uint8_t i = 0; i < 48; i++) {  // shift the table to the proper bit
    lookupNibble[i] <<= muxId;
  }

  while (pValue < pEnd) {
    uint8_t value = *(pValue++);
    uint8_t nibble = (value >> 4) * 3;
    *(pDma32++) |= lookupNibble[nibble];
    *(pDma32++) |= lookupNibble[nibble + 1];
    *(pDma32++) |= lookupNibble[nibble + 2];

    nibble = (value & 0xF) * 3;
    *(pDma32++) |= lookupNibble[nibble];
    *(pDma32++) |= lookupNibble[nibble + 1];
    *(pDma32++) |= lookupNibble[nibble + 2];
  }
}
#endif


void setup() {

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
  uint32_t elaps;



#ifdef _WS2812X8_

  uint8_t* dmaOutputBuffer3StepX8;
  dmaOutputBuffer3StepX8 = (uint8_t*) malloc(bufferSize * 3 * 8 + 4);
  memset(dmaOutputBuffer3StepX8, 0x00, bufferSize * 3 * 8 + 4);

  uint8_t* dmaOutputBuffer3StepX8Fast;
  dmaOutputBuffer3StepX8Fast = (uint8_t*) malloc(bufferSize * 3 * 8 + 4);
  memset(dmaOutputBuffer3StepX8Fast, 0x00, bufferSize * 3 * 8 + 4);


#endif
  /*
    #ifdef _DMX512_
    uint8_t* dmaOutputBufferDmx512;
    dmaOutputBufferDmx512 = (uint8_t*) malloc((bufferSize * 11) / 8 + 16);
    memset(dmaOutputBufferDmx512, 0xff, (bufferSize * 11) / 8 + 16);


    uint8_t* dmaOutputBuffer11Step;
    dmaOutputBuffer11Step = (uint8_t*) malloc((bufferSize * 11) / 8 + 16);
    memset(dmaOutputBuffer11Step, 0xff, (bufferSize * 11) / 8 + 16);

    uint8_t* outputBufferDmaDmx;
    outputBufferDmaDmx = (uint8_t*) malloc((bufferSize * 11) / 8 + 16);
    memset(outputBufferDmaDmx, 0xff, (bufferSize * 11) / 8 + 16);
    #endif

    #ifdef _WS2812_
    uint8_t* dmaOutputBuffer4Step;
    dmaOutputBuffer4Step = (uint8_t*) malloc(bufferSize * 4 + 4);
    memset(dmaOutputBuffer4Step, 0x00, bufferSize * 4 + 4);
    uint8_t* dmaOutputBuffer3Step;
    dmaOutputBuffer3Step = (uint8_t*) malloc(bufferSize * 3 + 4);
    memset(dmaOutputBuffer3Step, 0x00, bufferSize * 3 + 4);
    uint8_t* dmaOutputBuffer3StepFast;
    dmaOutputBuffer3StepFast = (uint8_t*) malloc(bufferSize * 3 + 4);
    memset(dmaOutputBuffer3StepFast, 0x00, bufferSize * 3 + 4);
    uint8_t* dmaOutputBuffer3StepFast2;
    dmaOutputBuffer3StepFast2 = (uint8_t*) malloc(bufferSize * 3 + 4);
    memset(dmaOutputBuffer3StepFast2, 0x00, bufferSize * 3 + 4);
    #endif
  */

  Serial.begin(500000);
  Serial.println();
  Serial.println();
  delay(500);
  Serial.println("Encoding Speed Tester");
  Serial.println();

#ifdef _WS2812X8_

  Serial.println();
  Serial.println("* 3 Step X8 encoding");
  delay(1000);
  uint32_t startTime = micros();
  for (uint8_t i = 0; i < 8; i++) {
    EncodeIntoDma3StepX8(dmaOutputBuffer3StepX8, pixelData + i, bufferSize, i);
  }
  elaps = micros() - startTime;
  Serial.print("   us Elapsed : ");
  Serial.println(elaps, DEC);
  PrintOutputBuffer(dmaOutputBuffer3StepX8, 100); //bufferSize * 3 * 8 + 4);
  Serial.println();

  Serial.println("* 3 Step X8 encoding fast");
  delay(1000);
  startTime = micros();
  for (uint8_t i = 0; i < 8; i++) {
    EncodeIntoDma3StepX8Fast(dmaOutputBuffer3StepX8Fast, pixelData + i, bufferSize, i);
  }
  elaps = micros() - startTime;
  Serial.print("   us Elapsed : ");
  Serial.println(elaps, DEC);
  PrintOutputBuffer(dmaOutputBuffer3StepX8Fast, 100); //bufferSize * 3 * 8 + 4);
  Serial.println();

  int cmp = memcmp(dmaOutputBuffer3StepX8, dmaOutputBuffer3StepX8Fast, bufferSize * 3 * 8 + 4); 
  if (!cmp) {
    Serial.println("Data Blocks compare !!");
  }


#endif

#ifdef _WS2812_
  Serial.println("* 4 Step encoding");
  delay(1000);
  elaps = EncodeIntoDma4Step(dmaOutputBuffer4Step, pixelData, bufferSize);
  Serial.print("   us Elapsed : ");
  Serial.println(elaps, DEC);
  PrintOutputBuffer(dmaOutputBuffer4Step, bufferSize * 4 + 4);


  Serial.println();
  Serial.println("* 3 Step encoding");
  delay(1000);
  elaps = EncodeIntoDma3Step(dmaOutputBuffer3Step, pixelData, bufferSize);
  Serial.print("   us Elapsed : ");
  Serial.println(elaps, DEC);
  PrintOutputBuffer(dmaOutputBuffer3Step, bufferSize * 3 + 4);
  Serial.println();

  Serial.println("* 3 Step encoding fast");
  delay(1000);
  elaps = EncodeIntoDma3StepFast(dmaOutputBuffer3StepFast, pixelData, bufferSize);
  Serial.print("   us Elapsed : ");
  Serial.println(elaps, DEC);
  PrintOutputBuffer(dmaOutputBuffer3StepFast, bufferSize * 3 + 4);
  Serial.println();

  Serial.println("* 3 Step encoding fast 2");
  delay(1000);
  elaps = EncodeIntoDma3StepFast2(dmaOutputBuffer3StepFast2, pixelData, bufferSize);
  Serial.print("   us Elapsed : ");
  Serial.println(elaps, DEC);
  PrintOutputBuffer(dmaOutputBuffer3StepFast2, bufferSize * 3 + 4);
  Serial.println();
#endif

#ifdef _DMX512_
  Serial.println("* 11 Step encoding");
  delay(1000);
  elaps = EncodeIntoDma11Step(dmaOutputBuffer11Step, pixelData, bufferSize);
  Serial.print("   us Elapsed : ");
  Serial.println(elaps, DEC);
  PrintOutputBuffer(dmaOutputBuffer11Step, (bufferSize * 11) / 8 + 16);

  Serial.println("* Dmx512 encoding");
  delay(1000);
  elaps = EncoderDMX512(dmaOutputBufferDmx512, pixelData, bufferSize);
  Serial.print("   us Elapsed : ");
  Serial.println(elaps, DEC);
  PrintOutputBuffer(dmaOutputBufferDmx512, (bufferSize * 11) / 8 + 16);

  Serial.println("* Dmx encoding");
  delay(1000);
  elaps = EncodeDmaDmx(outputBufferDmaDmx, pixelData, bufferSize);
  Serial.print("   us Elapsed : ");
  Serial.println(elaps, DEC);
  PrintOutputBuffer(outputBufferDmaDmx, (bufferSize * 11) / 8 + 16);
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

void loop() {
}


uint32_t EncodeIntoDma4Step(uint8_t* dmaBuffer, const uint8_t* data, size_t sizeData)
{
  uint32_t startTime = micros();
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
  return micros() - startTime;
}

uint32_t EncodeIntoDma3Step(uint8_t* dmaBuffer, const uint8_t* data, size_t sizeData)
{
  uint32_t startTime = micros();
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
  return micros() - startTime;
}


uint32_t EncodeIntoDma3StepFast(uint8_t* dmaBuffer, const uint8_t* data, size_t sizeData)
{
  uint32_t startTime = micros();
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
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    memcpy(pDma++, output8 + 1 , 1);
    memcpy(pDma++, output8 + 2 , 1);
    memcpy(pDma++, output8 + 3 , 1);
    memcpy(pDma++, output8 + 5 , 1);
    memcpy(pDma++, output8 + 6 , 1);
    memcpy(pDma++, output8 + 7 , 1);
#else
    memcpy(pDma++, output8 + 1 , 1);
    memcpy(pDma++, output8 + 2 , 1);
    memcpy(pDma++, output8 + 6 , 1);
    memcpy(pDma++, output8 + 0 , 1);
    memcpy(pDma++, output8 + 4 , 1);
    memcpy(pDma++, output8 + 5 , 1);
#endif
  }
  if (pSrc == pEnd)  // the last pixelbuffer byte if it exists
  {
    output[0] = bitpatternsHigh[((*pSrc) >> 4) & 0x0f] | bitpatternsLow[((*pSrc) & 0x0f)];
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    memcpy(pDma++, output8 + 1 , 1);
    memcpy(pDma++, output8 + 2 , 1);
    memcpy(pDma++, output8 + 3 , 1);
#else
    memcpy(pDma++, output8 + 1 , 1);
    memcpy(pDma++, output8 + 2 , 1);
    pDma++;
    memcpy(pDma++, output8 + 0 , 1);
#endif
  }
  return micros() - startTime;
}

uint32_t EncodeIntoDma3StepFast2(uint8_t* dmaBuffer, const uint8_t* data, size_t sizeData)
{
  uint32_t startTime = micros();
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
  return micros() - startTime;
}

uint32_t EncodeDmaDmx(uint8_t* dmaBuffer, const uint8_t* data, size_t sizeData) {
  uint32_t startTime = micros();
  uint16_t* pDma = reinterpret_cast<uint16_t*>(dmaBuffer);
  uint16_t dmaValue[2] = {0, 0};  // 16-bit dual buffer, used for reading
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
  const uint8_t msW = 0, lsW = 1;
#else
  const uint8_t lsW = 0, msW = 1;
#endif
  uint32_t* dmaV32 = reinterpret_cast<uint32_t*>(dmaValue);  // pointer for the 32-bit version
  const uint8_t* pSrc = data;
  const uint8_t* pEnd = pSrc + sizeData;
  const uint8_t BitsInSample = 16; //sizeof(dmaValue) * 8;
  uint8_t destBitsLeft = BitsInSample;

  *(pDma++) = 0x0000; // start the break.
  *(pDma++) = 0x0000; // with the headebyte added it doesn't fit into 4 bytes anymore
  *(pDma++) = 0x3803; // 0b0011 1000 0000 0011
  *pDma = 0x0; // clear the first word

#ifndef _USE32BIT_
  const uint16_t LookUpMsN[16] = {0x003, 0x023, 0x013, 0x033, 0x00b, 0x02b, 0x01b, 0x03b,
                                  0x007, 0x027, 0x017, 0x037, 0x00f, 0x02f, 0x01f, 0x03f
                                 };

  const uint16_t LookUpLsN[16] = {0x000, 0x200, 0x100, 0x300, 0x080, 0x280, 0x180, 0x380,
                                  0x040, 0x240, 0x140, 0x340, 0x0c0, 0x2c0, 0x1c0, 0x3c0
                                 };
#else
  const uint32_t LookUpMsN[16] = {0x0030000, 0x0230000, 0x0130000, 0x0330000, 0x00b0000, 0x02b0000, 0x01b0000, 0x03b0000,
                                  0x0070000, 0x0270000, 0x0170000, 0x0370000, 0x00f0000, 0x02f0000, 0x01f0000, 0x03f0000
                                 };

  const uint32_t LookUpLsN[16] = {0x0000000, 0x2000000, 0x1000000, 0x3000000, 0x0800000, 0x2800000, 0x1800000, 0x3800000,
                                  0x0400000, 0x2400000, 0x1400000, 0x3400000, 0x0c00000, 0x2c00000, 0x1c00000, 0x3c00000
                                 };
#endif

  while (pSrc < pEnd)
  {
#ifndef _USE32BIT_
    dmaValue[msW] = LookUpLsN[(*pSrc) & 0x0f] | LookUpMsN[(*pSrc) >> 4];
#else
    *dmaV32 = LookUpLsN[(*pSrc) & 0x0f] | LookUpMsN[(*pSrc) >> 4];  // basically read it as a 32-bit (asignment clears lsWord)
#endif
    pSrc++;
    if (destBitsLeft < 11) {
      *dmaV32 = *dmaV32 >> (11 - destBitsLeft);  // shift it as a 32-bit, so the rightshifted bits end up in the lsWord
      *(pDma++) |= dmaValue[msW]; // the msWord  & and up the 16-bit Dma buffer pointer
      *pDma = dmaValue[lsW]; // lsWord aka whatever got shifted out of the variable
#ifndef _USE32BIT_
      dmaValue[lsW] = 0;  // clear the lsWord after use, not needed when using 32-bit lookup
#endif
      destBitsLeft += 5;  // 11 bits subtracted but rolled over backwards = 5 bits added.
    }
    else {
      *dmaV32 = *dmaV32 << (destBitsLeft - 11); // shift to be the next bit
      *pDma |= dmaValue[msW]; // 'or' the most significant word
      destBitsLeft -= 11;   // substract the 11 bits added
    }
  }
  if (destBitsLeft) {
    *pDma |= 0xffff >> (16 - destBitsLeft);
  }
  return micros() - startTime;
}

uint32_t EncoderDMX512(uint8_t* dmaBuffer, const uint8_t* data, size_t sizeData ) {
  uint32_t startTime = micros();
  const uint8_t* pSrc = data;
  const uint8_t* pEnd = pSrc + sizeData;
  uint16_t* pDma = reinterpret_cast<uint16_t*>(dmaBuffer);

  int8_t outputBit = 16;
  uint16_t output = 0;

  *(pDma++) = 0x0000; // start the break.
  *(pDma++) = 0x0000; // with the headebyte added it doesn't fit into 4 bytes anymore
  *(pDma++) = 0x3803; // 0b0011 1000 0000 0011

  const uint8_t Reverse8BitsLookup[16] = {0x0, 0x8, 0x4, 0xc, 0x2, 0xa, 0x6, 0xe, 0x1, 0x9, 0x5, 0xd, 0x3, 0xb, 0x7, 0xf};

  // DATA stream, one start, two stop
  while (pSrc < pEnd)
  {
    uint8_t invertedByte = (Reverse8BitsLookup[*pSrc & 0xf] << 4) | Reverse8BitsLookup[*pSrc >> 4];
    pSrc++;

    if (outputBit > 10)
    {
      outputBit -= 1;
      output |= 0 << outputBit;

      outputBit -= 8;
      output |= invertedByte << outputBit;

      outputBit -= 2;
      output |= 3 << outputBit;
    }
    else
    {
      // split across an output uint16_t
      // handle start bit
      if (outputBit < 1)
      {
        *(pDma++) = output;
        output = 0;
        outputBit += 16;
      }
      outputBit -= 1;
      output |= 0 << outputBit;

      // handle data bits
      if (outputBit < 8)
      {
        output |= invertedByte >> (8 - outputBit);

        *(pDma++) = output;
        output = 0;
        outputBit += 16;
      }
      outputBit -= 8;
      output |= invertedByte << outputBit;

      // handle stop bits
      if (outputBit < 2)
      {
        output |= 3 >> (2 - outputBit);

        *(pDma++) = output;
        output = 0;
        outputBit += 16;
      }
      outputBit -= 2;
      output |= 3 << outputBit;
    }
  }
  if (outputBit > 0)
  {
    // padd last output uint32_t with Mtbp
    output |= 0xffff >> (16 - outputBit);
    *(pDma++) = output;
  }
  // fill the rest of the output with Mtbp
  return micros() - startTime;
}


uint32_t EncodeIntoDma11Step(uint8_t* dmaBuffer, const uint8_t* data, size_t sizeData) {
  uint32_t startTime = micros();
  uint16_t* pDma = reinterpret_cast<uint16_t*>(dmaBuffer);
  uint16_t dmaValue = 0;
  const uint8_t* pSrc = data;
  const uint8_t* pEnd = pSrc + sizeData;
  const size_t BitsInSample = sizeof(dmaValue) * 8;

  uint8_t destBitsLeft = BitsInSample;


  *(pDma++) = 0x0000; // start the break.
  *(pDma++) = 0x0000; // with the headebyte added it doesn't fit into 4 bytes anymore
  *(pDma++) = 0x3803; // 0b0011 1000 0000 0011
  // end the break.  Breaklength = 32 + 2 * 4 = 136us
  // BreakMab  3 * 4 = 12us  header byte (ch 0) 11 * 4 = 44us
  // total length 192 us In effect the break will be even longer due
  // to the 4 byte alignment, which isn't a requirement for this

  while (pSrc < pEnd)
  {
    uint8_t source = *(pSrc++);
    if (!destBitsLeft)
    {
      destBitsLeft = BitsInSample;
      *(pDma++) = dmaValue;
    }
    dmaValue = dmaValue << 1;  // start Bit
    destBitsLeft--;

    for (uint8_t i = 0; i < 8; i++) // data bits
    {
      if (!destBitsLeft)
      {
        destBitsLeft = BitsInSample;
        *(pDma++) = dmaValue;
      }
      dmaValue = dmaValue << 1;
      dmaValue |= (source & 1);
      source = source >> 1;
      destBitsLeft--;
    }

    for (uint8_t i = 0; i < 2; i++) // stop bits
    {
      if (!destBitsLeft)
      {
        destBitsLeft = BitsInSample;
        *(pDma++) = dmaValue;
      }
      dmaValue = dmaValue << 1;
      dmaValue |= 1;
      destBitsLeft--;
    }
  }
  if (destBitsLeft) {
    dmaValue = dmaValue << destBitsLeft;  // shift the significant bits fully left
    dmaValue |= (0xffff >> (BitsInSample - destBitsLeft)); // fill it up with HIGh bits
    *pDma = dmaValue;  // and store the remaining bits
  }
  return micros() - startTime;
}
