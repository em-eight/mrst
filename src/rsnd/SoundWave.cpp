
#include <bit>
#include <cstdlib>
#include <iostream>

#include "rsnd/SoundWave.hpp"
#include "common/fileUtil.hpp"

namespace rsnd {
void SoundWaveHeader::bswap() {
  this->BinaryFileHeader::bswap();
  infoOffset = std::byteswap(infoOffset);
  infoLength = std::byteswap(infoLength);
  dataOffset = std::byteswap(dataOffset);
  dataLength = std::byteswap(dataLength);
}

void SoundWaveInfo::bswap() {
  this->BinaryBlockHeader::bswap();
  sampleRate = std::byteswap(sampleRate);
  loopStart = std::byteswap(loopStart);
  loopEnd = std::byteswap(loopEnd);
  channelInfoTableOffset = std::byteswap(channelInfoTableOffset);
  dataLoc = std::byteswap(dataLoc);
  _18 = std::byteswap(_18);
}

void SoundWaveChannelInfo::bswap() {
  dataOffset = std::byteswap(dataOffset);
  adpcmOffset = std::byteswap(adpcmOffset);
  frontLeftVolume = std::byteswap(frontLeftVolume);
  frontRightVolume = std::byteswap(frontRightVolume);
  backLeftVolume = std::byteswap(backLeftVolume);
  backRightVolume = std::byteswap(backRightVolume);
}

SoundWave::SoundWave(void* fileData, size_t fileSize) {
  dataSize = fileSize;
  data = fileData;

  SoundWaveHeader* wavHdr = static_cast<SoundWaveHeader*>(fileData);
  bool falseEndian = wavHdr->byteOrder != 0xFEFF;
  if (falseEndian) wavHdr->bswap();

  info = getOffsetT<SoundWaveInfo>(data, wavHdr->infoOffset);
  if (falseEndian) info->bswap();
  infoBase = getOffset(info, sizeof(BinaryBlockHeader));

  u32* channelInfoOffsets = getOffsetT<u32>(infoBase, info->channelInfoTableOffset);
  for (int i = 0; i < info->channelCount; i++) {
    if (falseEndian) channelInfoOffsets[i] = std::byteswap(channelInfoOffsets[i]);
    SoundWaveChannelInfo* chInfo = getOffsetT<SoundWaveChannelInfo>(infoBase, channelInfoOffsets[i]);
    if (falseEndian) chInfo->bswap();

    AdpcParams* adpcParams = getOffsetT<AdpcParams>(infoBase, chInfo->adpcmOffset);
    if (falseEndian) adpcParams->bswap();
  }

  waveData = getOffsetT<SoundWaveData>(data, wavHdr->dataOffset);
  if (falseEndian) waveData->bswap();
  waveDataBase = getOffset(waveData, sizeof(BinaryBlockHeader));

  if (info->format == SoundWaveInfo::FORMAT_PCM16 && falseEndian) {
    u32 sampleCount = getTrackSampleCount();
    for (int i = 0; i < info->channelCount; i++) {
      s16* blockData = reinterpret_cast<s16*>(const_cast<u8*>(getChannelData(i)));
      for (int sample = 0; sample < sampleCount; sample++) {
        blockData[sample] = std::byteswap(blockData[sample]);
      }
    }
  }
}

u32 SoundWave::getLoopStart() const {
  switch (info->format)
  {
  case SoundWaveInfo::FORMAT_PCM8:
    return info->loopStart;
  
  case SoundWaveInfo::FORMAT_PCM16:
    return info->loopStart;
  
  // info->dataLoc or loopEnd playing a role here?
  case SoundWaveInfo::FORMAT_ADPCM:
    return (14*(info->loopStart-8)/8)/info->channelCount;
  
  default:
    std::cerr << "Warning: unknown track format " << info->format << '\n';
    return 0;
  }
}

u32 SoundWave::getLoopEnd() const {
  switch (info->format)
  {
  case SoundWaveInfo::FORMAT_PCM8:
    return info->loopEnd;
  
  case SoundWaveInfo::FORMAT_PCM16:
    return info->loopEnd;
  
  // info->dataLoc or loopEnd playing a role here?
  case SoundWaveInfo::FORMAT_ADPCM:
    return (14*(info->loopEnd-8)/8)/info->channelCount;
  
  default:
    std::cerr << "Warning: unknown track format " << info->format << '\n';
    return 0;
  }
}

u32 SoundWave::getTrackSampleCount() const {
  switch (info->format)
  {
  case SoundWaveInfo::FORMAT_PCM8:
    return info->loopEnd;
  
  case SoundWaveInfo::FORMAT_PCM16:
    return info->loopEnd;
  
  // info->dataLoc or loopEnd playing a role here?
  case SoundWaveInfo::FORMAT_ADPCM:
    return (14*(waveData->length-8)/8)/info->channelCount;
  
  default:
    std::cerr << "Warning: unknown track format " << info->format << '\n';
    return 0;
  }
}

void SoundWave::decodeChannel(u8 channelIdx, s16* buffer, u8 offset, u8 stride) const {
  const u8* blockData = getChannelData(channelIdx);
  u32 sampleCount = getTrackSampleCount();
  // decode as one large block
  s16* blockBuffer = buffer + offset;

  switch (info->format)
  {
  case SoundWaveInfo::FORMAT_PCM8:
    decodePcm8Block(blockData, sampleCount, blockBuffer, stride);
    break;
  
  case SoundWaveInfo::FORMAT_PCM16:
    decodePcm16Block(blockData, sampleCount, blockBuffer, stride);
    break;
  
  case SoundWaveInfo::FORMAT_ADPCM: {
    const AdpcParams* adpcParams = getChannelAdpcmParam(channelIdx);
    decodeAdpcmBlock(blockData, sampleCount, adpcParams->params.coeffs, adpcParams->params.yn1, adpcParams->params.yn2, blockBuffer, stride);
    break;
  
  } default:
    std::cerr << "Warning: unknown track format " << info->format << '\n';
  }
}

s16* SoundWave::getChannelPcm(u8 channelIdx) const {
  u32 sampleCount = getTrackSampleCount();
  s16* pcmBuffer = static_cast<s16*>(malloc(sampleCount * sizeof(s16)));
  decodeChannel(channelIdx, pcmBuffer);

  return pcmBuffer;
}

s16* SoundWave::getTrackPcm() const {
  u8 channelCount = info->channelCount;
  u32 sampleCount = getTrackSampleCount();
  s16* pcmBuffer = static_cast<s16*>(malloc(channelCount * sampleCount * sizeof(s16)));

  for (int i = 0; i < channelCount; i++) {
    decodeChannel(i, pcmBuffer, i, channelCount);
  }

  return pcmBuffer;
}

void SoundWave::toWaveFile(std::filesystem::path wavePath) const {
  void* data = getTrackPcm();
  createWaveFile(wavePath, data, getTrackSampleCount(), info->sampleRate, info->channelCount);
  free(data);
}
}