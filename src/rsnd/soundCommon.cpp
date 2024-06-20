
#include <bit>
#include <algorithm>
#include <iostream>
#include <cstring>

#include "rsnd/soundCommon.hpp"
#include "common/util.h"

namespace rsnd {
void SoundWaveChannelInfo::bswap() {
  dataOffset = std::byteswap(dataOffset);
  adpcmOffset = std::byteswap(adpcmOffset);
  frontLeftVolume = std::byteswap(frontLeftVolume);
  frontRightVolume = std::byteswap(frontRightVolume);
  backLeftVolume = std::byteswap(backLeftVolume);
  backRightVolume = std::byteswap(backRightVolume);
}

void AdpcmParam::bswap() {
  for (int i = 0; i < 16; i++) {
    coeffs[i] = std::byteswap(coeffs[i]);
  }
  gain = std::byteswap(gain);
  predictorScale = std::byteswap(predictorScale);
  yn1 = std::byteswap(yn1);
  yn2 = std::byteswap(yn2);
}

void AdpcmParamLoop::bswap() {
  predictorScale = std::byteswap(predictorScale);
  yn1 = std::byteswap(yn1);
  yn2 = std::byteswap(yn2);
}

void AdpcParams::bswap() {
  params.bswap();
  paramsLoop.bswap();
}

void WaveInfo::bswap() {
  sampleRate = std::byteswap(sampleRate);
  loopStart = std::byteswap(loopStart);
  loopEnd = std::byteswap(loopEnd);
  channelInfoTableOffset = std::byteswap(channelInfoTableOffset);
  dataLoc = std::byteswap(dataLoc);
  _18 = std::byteswap(_18);
}

void decodePcm8Block(const u8* blockData, u32 sampleCount, s16* buffer, u8 stride) {
  for (u32 sampleIndex = 0; sampleIndex < sampleCount; sampleIndex++) {
    buffer[sampleIndex * stride] = (reinterpret_cast<const s8*>(blockData))[sampleIndex];
  }
}

void decodePcm16Block(const u8* blockData, u32 sampleCount, s16* buffer, u8 stride) {
  if (stride == 1) {
    memcpy(buffer, blockData, sampleCount * sizeof(s16));
  } else {
    for (u32 sampleIndex = 0; sampleIndex < sampleCount; sampleIndex++) {
      buffer[sampleIndex * stride] = (reinterpret_cast<const s16*>(blockData))[sampleIndex];
    }
  }
}

void decodeAdpcmBlock(const u8* blockData, u32 sampleCount, const s16 coeffs[16], s16 yn1, s16 yn2, s16* buffer, u8 stride) {
    u8 cps;
    s16 cyn1 = yn1;
    s16 cyn2 = yn2;
    u32 dataIndex = 0;

    for (u32 sampleIndex = 0; sampleIndex < sampleCount; sampleIndex++) {
      if (sampleIndex % 14 == 0) {
        cps = blockData[dataIndex++];
      }
      int outSample;
      if ((sampleIndex & 1) == 0) {
        outSample = blockData[dataIndex] >> 4;
      } else {
        outSample = blockData[dataIndex++] & 0x0f;
      }
      if (outSample >= 8) {
        outSample -= 16;
      }
      const s16 scale = 1 << (cps & 0x0f);
      int cIndex = 2 * (cps >> 4);

      outSample =
            (0x400 +
              ((scale * outSample) << 11) +
              coeffs[std::clamp(cIndex, 0, 15)] * cyn1 +
              coeffs[std::clamp(cIndex + 1, 0, 15)] * cyn2) >>
            11;

      cyn2 = cyn1;
      cyn1 = std::clamp(outSample, -32768, 32767);

      buffer[sampleIndex * stride] = cyn1;
    }
}

void decodeBlock(const u8* blockData, u32 sampleCount, s16* blockBuffer, u8 stride, u8 format, const AdpcParams* adpcParams) {
  switch (format)
  {
  case WaveInfo::FORMAT_PCM8:
    decodePcm8Block(blockData, sampleCount, blockBuffer, stride);
    break;
  
  case WaveInfo::FORMAT_PCM16:
    decodePcm16Block(blockData, sampleCount, blockBuffer, stride);
    break;
  
  case WaveInfo::FORMAT_ADPCM:
    decodeAdpcmBlock(blockData, sampleCount, adpcParams->params.coeffs, adpcParams->params.yn1, adpcParams->params.yn2, blockBuffer, stride);
    break;
  
  default:
    std::cerr << "Warning: unknown track format " << format << '\n';
  }
}

static constexpr u32 BRSAR_MAGIC = MAGIC_FOURCC({'R', 'S', 'A', 'R'});
static constexpr u32 BRSTM_MAGIC = MAGIC_FOURCC({'R', 'S', 'T', 'M'});
static constexpr u32 BRWAV_MAGIC = MAGIC_FOURCC({'R', 'W', 'A', 'V'});
static constexpr u32 BRWAR_MAGIC = MAGIC_FOURCC({'R', 'W', 'A', 'R'});
static constexpr u32 BRSEQ_MAGIC = MAGIC_FOURCC({'R', 'S', 'E', 'Q'});
static constexpr u32 BRBNK_MAGIC = MAGIC_FOURCC({'R', 'B', 'N', 'K'});
static constexpr u32 BRWSD_MAGIC = MAGIC_FOURCC({'R', 'W', 'S', 'D'});

FileFormat detectFileFormat(const std::string& filename, void* fileData, size_t fileSize) {
  u32 magic = *(u32*)fileData;
  if (isFalseEndian(magic)) magic = std::byteswap(magic);
  if (magic == BRSAR_MAGIC) {
    return FMT_BRSAR;
  } else if (magic == BRSTM_MAGIC) {
    return FMT_BRSTM;
  } else if (magic == BRWAV_MAGIC) {
    return FMT_BRWAV;
  } else if (magic == BRWAR_MAGIC) {
    return FMT_BRWAR;
  } else if (magic == BRWSD_MAGIC) {
    return FMT_BRWSD;
  } else if (magic == BRBNK_MAGIC) {
    return FMT_BRBNK;
  } else if (magic == BRSEQ_MAGIC) {
    return FMT_BRSEQ;
  } else {
    return FMT_UNKNOWN;
  }
}

u32 detectFileSize(void* fileData) {
  auto* bfh = static_cast<BinaryFileHeader*>(fileData);
  bool falseEndian = bfh->byteOrder != 0xFEFF;
  return falseEndian ? std::byteswap(bfh->fileSize) : bfh->fileSize;
}


std::string getFileFourcc(void* data) {
  char magicStr[5];
  u32 magic = *(u32*)data;
  *(u32*)magicStr = magic;
  magicStr[4] = '\0';
  return std::string(magicStr);
}
}