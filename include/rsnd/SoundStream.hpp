
#pragma once

#include <cstddef>

#include "common/types.h"
#include "common/util.h"

#include "rsnd/soundCommon.hpp"

namespace rsnd {
struct SoundStreamHeader : BinaryFileHeader {
  u32 headOffset;
  u32 headSize;
  u32 adpcOffset;
  u32 adpcSize;
  u32 dataOffset;
  u32 dataSize;

  void bswap();
};

struct SoundStreamHead : public BinaryBlockHeader {
  DataRef streamDataInfo;
  DataRef trackTable;
  DataRef channelTable;

  void bswap();
};

struct AdpcEntry {
  s16 yn1;
  s16 yn2;

  void bswap();
};

struct SoundStreamAdpc : public BinaryBlockHeader {
  // one for each block and each channel
  AdpcEntry adpcEntries[1];

  void bswap();
};

struct SoundStreamData : public BinaryBlockHeader {
  u32 dataOffset;

  void bswap();
};

struct StreamDataInfo {
  static const u8 FORMAT_PCM8 = 0;
  static const u8 FORMAT_PCM16 = 1;
  static const u8 FORMAT_ADPCM = 2;

  u8 format;
  u8 loop;
  u8 channelCount;
  u8 sampleRate24;
  u16 sampleRate;
  u16 blockHeaderOffset;
  u32 loopStart;
  u32 loopEnd;
  u32 dataOffset;
  u32 blockCount;
  u32 blockSize;
  u32 blockSamples;
  u32 finalBlockSize;
  u32 finalBlockSamples;
  u32 finalBlockPaddedSize;
  u32 adpcmInterval;
  u32 adpcmDataSize;

  void bswap();
  u32 getSampleRate() const { return (sampleRate24 << 16) + sampleRate; }
};

struct TrackTable {
  static const u8 SIMPLE = 0;
  static const u8 EXTENDED = 1;

  u8 trackCount;
  u8 trackInfoType;
  DataRef trackInfo[1];

  void bswap();
};

struct TrackInfoSimple {
  u8 channelCount;
  u8 channelIndices[1];

  void bswap(){}
};

struct TrackInfoExtended {
  u8 volume;
  u8 pan;
  u16 _unk2;
  u32 _unk4;
  u8 channelCount;
  u8 channelIndices[1];

  void bswap(){}
};

struct ChannelTable {
  u8 channelCount;
  u8 padding[3];
  DataRef channelInfo[1];

  void bswap();
};

struct ChannelInfo {
  DataRef adpcParams;

  void bswap();
};

class SoundStream {
private:
  void* data;
  size_t dataSize;

  void decodeChannelPcm8(u8 channelIdx, s16* buffer, u8 offset = 0, u8 stride = 1) const;
  void decodeChannelPcm16(u8 channelIdx, s16* buffer, u8 offset = 0, u8 stride = 1) const;
  void decodeChannelAdpcm(u8 channelIdx, s16* buffer, u8 offset = 0, u8 stride = 1) const;
public:
  SoundStreamHead* strmHead;
  SoundStreamData* strmData;
  SoundStreamAdpc* strmAdpc;

  StreamDataInfo* strmDataInfo;
  TrackTable* trackTable;
  ChannelTable* channelTable;

  SoundStream(void* fileData, size_t fileSize);
  const ChannelInfo* getChannelInfo(u8 channelIdx) const;
  const u8 getTrackInfoType() const { return trackTable->trackInfoType; };
  const TrackInfoExtended* getTrackInfoExtended(u8 trackIdx) const;
  const TrackInfoSimple* getTrackInfoSimple(u8 trackIdx) const;
  const AdpcParams* getAdpcParams(u8 channelIdx) const;
  const AdpcEntry* getAdpcEntry(u32 b, u8 c) const;
  const u32 getBlockSize(u32 b) const { return b + 1 == strmDataInfo->blockCount ? strmDataInfo->finalBlockSize : strmDataInfo->blockSize; }
  const u32 getSampleCount() const;
  const u8* getBlockData(u8 channelIdx, u32 blockIdx) const;
  void decodeChannel(u8 channelIdx, s16* buffer, u8 offset = 0, u8 stride = 1) const;
  s16* getChannelPcm(u8 channelIdx) const;
  s16* getTrackPcm(u8 trackIdx, u8& channelCount) const;
  void trackToWaveFile(u8 trackIdx, std::filesystem::path wavePath) const;
};
}
