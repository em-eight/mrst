
#pragma once

#include <string>

#include "common/types.h"

namespace rsnd {

struct AdpcmParam {
  s16 coeffs[16];
  u16 gain;
  u16 predictorScale;
  s16 yn1;
  s16 yn2;

  void bswap();
};

struct AdpcmParamLoop {
  u16 predictorScale;
  s16 yn1;
  s16 yn2;

  void bswap();
};

struct AdpcParams {
  AdpcmParam params;
  AdpcmParamLoop paramsLoop;

  void bswap();
};

void decodePcm8Block(const u8* blockData, u32 sampleCount, s16* buffer, u8 stride);
void decodePcm16Block(const u8* blockData, u32 sampleCount, s16* buffer, u8 stride);
void decodeAdpcmBlock(const u8* blockData, u32 sampleCount, const s16 coeffs[16], s16 yn1, s16 yn2, s16* buffer, u8 stride);

constexpr u32 MAGIC_FOURCC(const char (&magic)[4]) {
    return (static_cast<u32>(magic[0]) << 24) |
           (static_cast<u32>(magic[1]) << 16) |
           (static_cast<u32>(magic[2]) << 8) |
           static_cast<u32>(magic[3]);
}

enum FileFormat {
  FMT_UNKNOWN,
  FMT_BRSAR,
  FMT_BRSTM,
  FMT_BRWAV,
  FMT_BRWAR,
  FMT_BRSEQ,
  FMT_BRBNK,
  FMT_BRWSD,
};

FileFormat detectFileFormat(const std::string& filename, void* fileData, size_t fileSize);
u32 detectFileSize(void* fileData);

std::string getFileFourcc(void* data);
inline bool isFalseEndian(u32 bom) { return bom != 0xFEFF; }
}