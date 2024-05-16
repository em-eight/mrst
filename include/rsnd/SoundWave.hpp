
#pragma once

#include <cstddef>
#include <filesystem>

#include "common/util.h"
#include "rsnd/soundCommon.hpp"

namespace rsnd {
struct SoundWaveHeader : public BinaryFileHeader {
  u32 infoOffset;
  u32 infoLength;
  u32 dataOffset;
  u32 dataLength;

  void bswap();
};

struct SoundWaveInfo : public BinaryBlockHeader {
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

struct SoundWaveChannelInfo {
  u32 dataOffset;
  u32 adpcmOffset;
  u32 frontLeftVolume;
  u32 frontRightVolume;
  u32 backLeftVolume;
  u32 backRightVolume;

  void bswap();
};

typedef BinaryBlockHeader SoundWaveData;

class SoundWave {
private:
  void* data;
  size_t dataSize;

public:
  SoundWaveInfo* info;
  SoundWaveData* waveData;

  void* infoBase;
  void* waveDataBase;

  SoundWave(void* fileData, size_t fileSize);
  const u32* getChannelInfoOffsets() const { return getOffsetT<u32>(infoBase, info->channelInfoTableOffset); }
  const SoundWaveChannelInfo* getChannelInfo(u8 idx) const { return getOffsetT<SoundWaveChannelInfo>(infoBase, getChannelInfoOffsets()[idx]); }
  const AdpcParams* getChannelAdpcmParam(u8 idx) const { return getOffsetT<AdpcParams>(infoBase, getChannelInfo(idx)->adpcmOffset); }
  const u8* getChannelData(u32 idx) const {
    void* waveBase2;
    switch (info->dataLocType) {
    case SoundWaveInfo::LOC_OFFSET:
      waveBase2 = getOffset(waveDataBase, 0);
      break;
    case SoundWaveInfo::LOC_ADDR:
      waveBase2 = reinterpret_cast<void*>(info->dataLoc);
      break;
    default:
      return nullptr;
    }
    return getOffsetT<const u8>(waveBase2, getChannelInfo(idx)->dataOffset);
  }
  void decodeChannel(u8 channelIdx, s16* buffer, u8 offset = 0, u8 stride = 1) const;
  s16* getChannelPcm(u8 channelIdx) const;
  u8 getChannelCount() const { return info->channelCount; }
  u32 getLoopStart() const;
  u32 getLoopEnd() const;
  u32 getTrackSampleCount() const;
  u32 getTrackSampleRate() const { return info->sampleRate; }
  u32 getTrackSampleBufferSize() const { return getChannelCount() * getTrackSampleCount() * sizeof(s16); }
  s16* getTrackPcm() const;
  void toWaveFile(std::filesystem::path wavePath) const;
};
}