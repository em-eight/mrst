
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

struct WaveInfo {
  static const u8 FORMAT_PCM8 = 0;
  static const u8 FORMAT_PCM16 = 1;
  static const u8 FORMAT_ADPCM = 2;

  static const u8 LOC_OFFSET = 0;
  static const u8 LOC_ADDR = 1;

  u8 format;
  bool loop;
  u8 channelCount;
  u8 sampleRate24;
  u16 sampleRate;
  u8 dataLocType;
  u8 _7;
  u32 loopStart;
  u32 loopEnd;
  u32 channelInfoTableOffset;
  u32 dataLoc;
  u32 _18;

  void bswap();
};

inline u32 dspAddressToSamples(u32 dspAddr) {
  // magic https://github.com/magcius/vgmtrans/blob/8e34ddc2fb43948dc1e1a8759c739a0c1c7b62d7/src/main/formats/RSARInstrSet.cpp#L224
  return (dspAddr / 16) * 14 + (dspAddr % 16) - 2;
}

void decodePcm8Block(const u8* blockData, u32 sampleCount, s16* buffer, u8 stride);
void decodePcm16Block(const u8* blockData, u32 sampleCount, s16* buffer, u8 stride);
void decodeAdpcmBlock(const u8* blockData, u32 sampleCount, const s16 coeffs[16], s16 yn1, s16 yn2, s16* buffer, u8 stride);
void decodeBlock(const u8* blockData, u32 sampleCount, s16* blockBuffer, u8 stride, u8 format, const AdpcParams* adpcParams);

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