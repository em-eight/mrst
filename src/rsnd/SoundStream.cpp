
#include <bit>
#include <concepts>
#include <array>
#include <algorithm>
#include <cstring>
#include <iostream>
#include <filesystem>

#include "rsnd/SoundStream.hpp"
#include "rsnd/soundCommon.hpp"
#include "common/fileUtil.hpp"

namespace rsnd {
void SoundStreamHeader::bswap() {
  this->BinaryFileHeader::bswap();
  headOffset = std::byteswap(headOffset);
  headSize = std::byteswap(headSize);
  adpcOffset = std::byteswap(adpcOffset);
  adpcSize = std::byteswap(adpcSize);
  dataOffset = std::byteswap(dataOffset);
  dataSize = std::byteswap(dataSize);
}

void SoundStreamHead::bswap() {
  this->BinaryBlockHeader::bswap();
  streamDataInfo.bswap();
  trackTable.bswap();
  channelTable.bswap();
};

void AdpcEntry::bswap() {
  yn1 = std::byteswap(yn1);
  yn2 = std::byteswap(yn2);
}

void SoundStreamAdpc::bswap() {
  this->BinaryBlockHeader::bswap();
  u16* adpcData = reinterpret_cast<u16*>((u8*)this + sizeof(BinaryBlockHeader));
  while (adpcData < reinterpret_cast<u16*>((u8*)this + length)) {
    *adpcData = std::byteswap(*adpcData);
    adpcData++;
  }
}

void SoundStreamData::bswap() {
  this->BinaryBlockHeader::bswap();
  dataOffset = std::byteswap(dataOffset);
}

void StreamDataInfo::bswap() {
  sampleRate = std::byteswap(sampleRate);
  loopStart = std::byteswap(loopStart);
  loopEnd = std::byteswap(loopEnd);
  dataOffset = std::byteswap(dataOffset);
  blockCount = std::byteswap(blockCount);
  blockSize = std::byteswap(blockSize);
  blockSamples = std::byteswap(blockSamples);
  finalBlockSize = std::byteswap(finalBlockSize);
  finalBlockSamples = std::byteswap(finalBlockSamples);
  finalBlockPaddedSize = std::byteswap(finalBlockPaddedSize);
  adpcmInterval = std::byteswap(adpcmInterval);
  adpcmDataSize = std::byteswap(adpcmDataSize);
}

void TrackTable::bswap() {
  for (int i = 0; i < trackCount; i++) {
    trackInfo[i].bswap();
  }
}

void ChannelTable::bswap() {
  for (int i = 0; i < channelCount; i++) {
    channelInfo[i].bswap();
  }
}

void ChannelInfo::bswap() {
  adpcParams.bswap();
}

SoundStream::SoundStream(void* fileData, size_t fileSize) {
  dataSize = fileSize;
  data = fileData;

  SoundStreamHeader* strmHdr = static_cast<SoundStreamHeader*>(data);
  strmHdr->bswap();
  strmHead = reinterpret_cast<SoundStreamHead*>((u8*)data + strmHdr->headOffset);
  strmHead->bswap();
  strmData = reinterpret_cast<SoundStreamData*>((u8*)data + strmHdr->dataOffset);
  strmData->bswap();
  strmAdpc = reinterpret_cast<SoundStreamAdpc*>((u8*)data + strmHdr->adpcOffset);
  strmAdpc->bswap();

  strmDataInfo = reinterpret_cast<StreamDataInfo*>(strmHead->streamDataInfo.getAddr((u8*)strmHead + 8));
  strmDataInfo->bswap();

  trackTable = reinterpret_cast<TrackTable*>(strmHead->trackTable.getAddr((u8*)strmHead + 8));
  trackTable->bswap();
  for (int i = 0; i < trackTable->trackCount; i++) {
    TrackInfoExtended* trackInfo = reinterpret_cast<TrackInfoExtended*>(trackTable->trackInfo[i].getAddr((u8*)strmHead + 8));
    trackInfo->bswap();
  }

  channelTable = reinterpret_cast<ChannelTable*>(strmHead->channelTable.getAddr((u8*)strmHead + 8));
  channelTable->bswap();
  for (int i = 0; i < channelTable->channelCount; i++) {
    ChannelInfo* channelInfo = reinterpret_cast<ChannelInfo*>(channelTable->channelInfo[i].getAddr((u8*)strmHead + sizeof(BinaryBlockHeader)));
    channelInfo->bswap();
    AdpcParams* adpcParams = reinterpret_cast<AdpcParams*>(channelInfo->adpcParams.getAddr((u8*)strmHead + sizeof(BinaryBlockHeader)));
    adpcParams->bswap();
  }

  if (strmDataInfo->format == StreamDataInfo::FORMAT_PCM16) {
    for (int c = 0; c < strmDataInfo->channelCount; c++) {
      for (int b = 0; b < strmDataInfo->blockCount; b++) {
        s16* blockData = reinterpret_cast<s16*>(const_cast<u8*>(getBlockData(c, b)));
        u32 blockSamples = b + 1 == strmDataInfo->blockCount ? strmDataInfo->finalBlockSamples : strmDataInfo->blockSamples;
        for (int i = 0; i < blockSamples; i++) {
          blockData[i] = std::byteswap(blockData[i]);
        }
      }
    }
  }
}

const TrackInfoSimple* SoundStream::getTrackInfoSimple(u8 idx) const {
  return reinterpret_cast<TrackInfoSimple*>(trackTable->trackInfo[idx].getAddr((u8*)strmHead + sizeof(BinaryBlockHeader)));
}

const TrackInfoExtended* SoundStream::getTrackInfoExtended(u8 idx) const {
  return reinterpret_cast<TrackInfoExtended*>(trackTable->trackInfo[idx].getAddr((u8*)strmHead + sizeof(BinaryBlockHeader)));
}

const ChannelInfo* SoundStream::getChannelInfo(u8 idx) const {
  return reinterpret_cast<ChannelInfo*>(channelTable->channelInfo[idx].getAddr((u8*)strmHead + sizeof(BinaryBlockHeader)));
}

const AdpcParams* SoundStream::getAdpcParams(u8 channelIdx) const {
  const ChannelInfo* chInfo = getChannelInfo(channelIdx);
  return reinterpret_cast<AdpcParams*>(chInfo->adpcParams.getAddr((u8*)strmHead + sizeof(BinaryBlockHeader)));
}

const AdpcEntry* SoundStream::getAdpcEntry(u8 b, u8 c) const {
  return reinterpret_cast<AdpcEntry*>((u8*)strmAdpc + sizeof(BinaryBlockHeader) + (b * strmDataInfo->channelCount + c) * sizeof(AdpcEntry));
}

const u32 SoundStream::getSampleCount() const {
  return (strmDataInfo->blockCount - 1) * strmDataInfo->blockSamples + strmDataInfo->finalBlockSamples;
}

const u8* SoundStream::getBlockData(u8 channelIdx, u8 blockIdx) const {
  u8 c = channelIdx;
  u8 b = blockIdx;
  u32 blockCount = strmDataInfo->blockCount;
  u8 channelCount = strmDataInfo->channelCount;
  u32 blockSize = strmDataInfo->blockSize;
  u32 finalBlockSize = strmDataInfo->finalBlockSize;
  u32 finalBlockPaddedSize = strmDataInfo->finalBlockPaddedSize;

  const u32 rawDataOffset =
    // Final block on non-zero channel: need to consider the previous channels' finalBlockSizeWithPadding!
    c != 0 && b + 1 == blockCount
      ? b * channelCount * blockSize + c * finalBlockPaddedSize
      : (b * channelCount + c) * blockSize;
  const u32 rawDataEnd =
    b + 1 == blockCount
      ? rawDataOffset + finalBlockSize
      : rawDataOffset + blockSize;
  return reinterpret_cast<u8*>(strmData) + sizeof(BinaryBlockHeader) + strmData->dataOffset + rawDataOffset;
}

void SoundStream::decodeChannelPcm8(u8 channelIdx, s16* buffer, u8 offset, u8 sampleStride) const {
  u32 blockCount = strmDataInfo->blockCount;
  u32 usualBlockSize = strmDataInfo->blockSize;
  u32 finalBlockSize = strmDataInfo->finalBlockSize;
  u32 usualBlockSamples = strmDataInfo->blockSamples;
  u32 finalBlockSamples = strmDataInfo->finalBlockSamples;

  for (u32 b = 0; b < strmDataInfo->blockCount; b++) {
    const u8* blockData = getBlockData(channelIdx, b);
    u32 blockSize = b + 1 == blockCount ? finalBlockSize : usualBlockSize;
    u32 blockSamples = b + 1 == blockCount ? finalBlockSamples : usualBlockSamples;

    s16* blockBuffer = buffer + b * usualBlockSamples * sampleStride + offset;
    decodePcm8Block(blockData, blockSamples, blockBuffer, sampleStride);
  }
}

void SoundStream::decodeChannelPcm16(u8 channelIdx, s16* buffer, u8 offset, u8 sampleStride) const {
  u32 blockCount = strmDataInfo->blockCount;
  u32 usualBlockSize = strmDataInfo->blockSize;
  u32 finalBlockSize = strmDataInfo->finalBlockSize;
  u32 usualBlockSamples = strmDataInfo->blockSamples;
  u32 finalBlockSamples = strmDataInfo->finalBlockSamples;

  for (u32 b = 0; b < strmDataInfo->blockCount; b++) {
    const u8* blockData = getBlockData(channelIdx, b);
    u32 blockSize = b + 1 == blockCount ? finalBlockSize : usualBlockSize;
    u32 blockSamples = b + 1 == blockCount ? finalBlockSamples : usualBlockSamples;

    s16* blockBuffer = buffer + b * usualBlockSamples * sampleStride + offset;
    decodePcm16Block(blockData, blockSamples, blockBuffer, sampleStride);
  }
}

void SoundStream::decodeChannelAdpcm(u8 channelIdx, s16* buffer, u8 offset, u8 sampleStride) const {
  u8 c = channelIdx;
  u32 blockCount = strmDataInfo->blockCount;
  u32 usualBlockSize = strmDataInfo->blockSize;
  u32 finalBlockSize = strmDataInfo->finalBlockSize;
  u32 usualBlockSamples = strmDataInfo->blockSamples;
  u32 finalBlockSamples = strmDataInfo->finalBlockSamples;

  const AdpcParams* adpcParams = getAdpcParams(channelIdx);

  for (u32 b = 0; b < strmDataInfo->blockCount; b++) {
    const AdpcEntry* adpcEntry = getAdpcEntry(b, c);
    s16 yn1 = adpcEntry->yn1;
    s16 yn2 = adpcEntry->yn2;

    u32 blockSize = b + 1 == blockCount ? finalBlockSize : usualBlockSize;
    u32 blockSamples = b + 1 == blockCount ? finalBlockSamples : usualBlockSamples;
    const u8* blockData = getBlockData(c, b);

    s16* blockBuffer = buffer + b * usualBlockSamples * sampleStride + offset;
    decodeAdpcmBlock(blockData, blockSamples, adpcParams->params.coeffs, yn1, yn2, blockBuffer, sampleStride);
  }
}

void SoundStream::decodeChannel(u8 channelIdx, s16* buffer, u8 offset, u8 sampleStride) const {
  switch (strmDataInfo->format)
  {
  case StreamDataInfo::FORMAT_PCM16:
    decodeChannelPcm16(channelIdx, buffer, offset, sampleStride);
    break;
  
  case StreamDataInfo::FORMAT_PCM8:
    decodeChannelPcm8(channelIdx, buffer, offset, sampleStride);
    break;
  
  case StreamDataInfo::FORMAT_ADPCM:
    decodeChannelAdpcm(channelIdx, buffer, offset, sampleStride);
    break;
  
  default:
    std::cerr << "Warning: unknown track format " << strmDataInfo->format << '\n';
    break;
  }
}

s16* SoundStream::getChannelPcm(u8 channelIdx) const {
  u32 sampleCount = getSampleCount();
  s16* pcmBuffer = static_cast<s16*>(malloc(sampleCount * sizeof(s16)));
  decodeChannel(channelIdx, pcmBuffer);

  return pcmBuffer;
}

s16* SoundStream::getTrackPcm(u8 trackIdx, u8& channelCount) const {
  const u8* channelIndices;
  switch (trackTable->trackInfoType) {
  case TrackTable::SIMPLE: {
    const TrackInfoSimple* trackInfoSimple = getTrackInfoSimple(trackIdx);
    channelCount = trackInfoSimple->channelCount;
    channelIndices = trackInfoSimple->channelIndices;
    break;

  } case TrackTable::EXTENDED: {
    const TrackInfoExtended* trackInfoExtended = getTrackInfoExtended(trackIdx);
    channelCount = trackInfoExtended->channelCount;
    channelIndices = trackInfoExtended->channelIndices;
    break;
  
  } default:
    std::cout << "Invalid track info type value " << trackTable->trackInfoType << std::endl;
    exit(-1);
  }

  u32 sampleCount = getSampleCount();
  s16* pcmBuffer = static_cast<s16*>(malloc(channelCount * sampleCount * sizeof(s16)));

  for (int i = 0; i < channelCount; i++) {
    u8 channelIdx = channelIndices[i];
    decodeChannel(channelIdx, pcmBuffer, i, channelCount);
  }

  return pcmBuffer;
}

void SoundStream::trackToWaveFile(u8 trackIdx, std::filesystem::path wavePath) const {
  u8 channelCount;
  void* data = getTrackPcm(trackIdx, channelCount);
  createWaveFile(wavePath, data, getSampleCount(), strmDataInfo->sampleRate, channelCount);
  free(data);
}
}