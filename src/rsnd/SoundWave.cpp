
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
  this->WaveInfo::bswap();
}

u32 sampleByDspAddress(u32 sample, u8 format) {
  switch (format)
  {
  case SoundWaveInfo::FORMAT_PCM8:
  case SoundWaveInfo::FORMAT_PCM16:
    return sample;
  
  case SoundWaveInfo::FORMAT_ADPCM:
    return dspAddressToSamples(sample);
  
  default:
    std::cerr << "Warning: unknown track format " << (int)format << '\n';
    return 0;
  }
}

SoundWave::SoundWave(void* fileData, size_t fileSize) {
  dataSize = fileSize;
  data = fileData;

  SoundWaveHeader* wavHdr = static_cast<SoundWaveHeader*>(fileData);
  bool falseEndian = wavHdr->byteOrder != 0xFEFF;
  if (falseEndian) wavHdr->bswap();

  info = getOffsetT<SoundWaveInfo>(data, wavHdr->infoOffset);
  if (falseEndian) info->bswap();
  info->loopStart = sampleByDspAddress(info->loopStart, info->format);
  info->loopEnd = sampleByDspAddress(info->loopStart, info->format) + 1;
  infoBase = getOffset(info, sizeof(BinaryBlockHeader));

  u32* channelInfoOffsets = getOffsetT<u32>(infoBase, info->channelInfoTableOffset);
  for (int i = 0; i < info->channelCount; i++) {
    if (falseEndian) channelInfoOffsets[i] = std::byteswap(channelInfoOffsets[i]);
    SoundWaveChannelInfo* chInfo = getOffsetT<SoundWaveChannelInfo>(infoBase, channelInfoOffsets[i]);
    if (falseEndian) chInfo->bswap();

    if (info->format == WaveInfo::FORMAT_ADPCM) {
      AdpcParams* adpcParams = getOffsetT<AdpcParams>(infoBase, chInfo->adpcmOffset);
      if (falseEndian) adpcParams->bswap();
    }
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

u32 SoundWave::getTrackSampleCount() const {
  return sampleByDspAddress(waveData->length, info->format) / info->channelCount;
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
  createWaveFile(wavePath, data, getTrackSampleCount(), info->getSampleRate(), info->channelCount);
  free(data);
}
}
