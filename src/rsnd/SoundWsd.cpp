
#include "rsnd/SoundWsd.hpp"
#include "common/fileUtil.hpp"

namespace rsnd {
void WsdHeader::bswap() {
  this->BinaryFileHeader::bswap();

  dataOffset = std::byteswap(dataOffset);
  dataLength = std::byteswap(dataLength);
  waveOffset = std::byteswap(waveOffset);
  waveLength = std::byteswap(waveLength);
}

void WsdData::bswap() {
  this->BinaryBlockHeader::bswap();
  refs.bswap();
}

void Wsd::bswap() {
  wsdInfo.bswap();
  trackTable.bswap();
  noteTable.bswap();
}

void WsdInfo::bswap() {
  pitch = bswap_float(pitch);
  _c.bswap();
  _14.bswap();
  _1c = std::byteswap(_1c);
}

void TrackInfo::bswap() {
  noteEventTable.bswap();
}

void NoteEvent::bswap() {
  position = bswap_float(position);
  length = bswap_float(length);
  noteIdx = std::byteswap(noteIdx);
  _c = std::byteswap(_c);
}

void NoteInformationEntry::bswap() {
  waveIdx = std::byteswap(waveIdx);
  pitch = bswap_float(pitch);
  lfoTable.bswap();
  graphEnvTable.bswap();
  randomizerTable.bswap();
  _res = std::byteswap(_res);
}

void WsdWave::bswap() {
  this->BinaryBlockHeader::bswap();
  waveInfos.bswap();
}

void WsdWaveOld::bswap() {
  this->BinaryBlockHeader::bswap();
  int count = (this->length - 8) / sizeof(u32);
  for (int i = 0; i < count; i++) {
    this->elems[i] = std::byteswap(this->elems[i]);
  }
}

SoundWsd::SoundWsd(void* fileData, size_t fileSize, void* waveData) {
  dataSize = fileSize;
  data = fileData;

  wsdHdr = static_cast<WsdHeader*>(fileData);
  bool falseEndian = wsdHdr->byteOrder != 0xFEFF;
  if (falseEndian) wsdHdr->bswap();

  wsdData = getOffsetT<WsdData>(data, wsdHdr->dataOffset);
  if (falseEndian) wsdData->bswap();
  dataBase = getOffset(wsdData, sizeof(BinaryBlockHeader));
  
  containsWaveInfo = wsdHdr->waveOffset != 0;
  if (containsWaveInfo) {
    wsdWave = getOffsetT<void>(data, wsdHdr->waveOffset);

    u32* waveInfoOffs;
    u32 waveInfoCount;
    if (wsdHdr->version >= SoundWsd::FILE_VERSION_NEW_WAVE_BLOCK) {
      WsdWave* waveNew = static_cast<WsdWave*>(wsdWave);
      if (falseEndian) waveNew->bswap();

      waveInfoOffs = waveNew->waveInfos.elems;
      waveInfoCount = waveNew->waveInfos.size;
    } else {
      WsdWaveOld* waveOld = static_cast<WsdWaveOld*>(wsdWave);
      if (falseEndian) waveOld->bswap();

      waveInfoOffs = waveOld->elems;
      waveInfoCount = (waveOld->length - 8) / sizeof(u32);
    }
    waveBase = getOffset(wsdWave, 0);

    for (int i = 0; i < waveInfoCount; i++) {
      WaveInfo* waveInfo = getOffsetT<WaveInfo>(waveBase, waveInfoOffs[i]);
      if (falseEndian) waveInfo->bswap();
      waveInfo->loopStart = sampleByDspAddress(waveInfo->loopStart, waveInfo->format);
      waveInfo->loopEnd = sampleByDspAddress(waveInfo->loopEnd, waveInfo->format) + 1;

      u32* channelInfoOffsets = getOffsetT<u32>(waveInfo, waveInfo->channelInfoTableOffset);
      for (int j = 0; j < waveInfo->channelCount; j++) {
        if (falseEndian) channelInfoOffsets[j] = std::byteswap(channelInfoOffsets[j]);
        SoundWaveChannelInfo* chInfo = getOffsetT<SoundWaveChannelInfo>(waveInfo, channelInfoOffsets[j]);
        if (falseEndian) chInfo->bswap();

        if (waveInfo->format == WaveInfo::FORMAT_ADPCM) {
          AdpcParams* adpcParams = getOffsetT<AdpcParams>(waveInfo, chInfo->adpcmOffset);
          if (falseEndian) adpcParams->bswap();
        }

        if (falseEndian && waveInfo->format == WaveInfo::FORMAT_PCM16 && waveData != nullptr) {
          // audio samples also need byteswap in this case
          s16* blockData = reinterpret_cast<s16*>((u8*)waveData + waveInfo->dataLoc + chInfo->dataOffset);
          u32 sampleCount = waveInfo->loopEnd;
          for (int sample = 0; sample < sampleCount; sample++) {
            blockData[sample] = std::byteswap(blockData[sample]);
          }
        }
      }
    }
  } else {
    wsdWave = nullptr;
  }

  for (int i = 0; i < wsdData->refs.size; i++) {
    Wsd* wsd = wsdData->refs.elems[i].getAddr<Wsd>(dataBase);
    if (falseEndian) wsd->bswap();
    
    WsdInfo* wsdInfo = wsd->wsdInfo.getAddr<WsdInfo>(dataBase);
    if (falseEndian) wsdInfo->bswap();

    WsdTrackTable* trackTable = wsd->trackTable.getAddr<WsdTrackTable>(dataBase);
    if (falseEndian) trackTable->bswap();
    for (int j = 0; j < trackTable->size; j++) {
      TrackInfo* trackInfo = trackTable->elems[j].getAddr<TrackInfo>(dataBase);
      if (falseEndian) trackInfo->bswap();

      NoteEventTable* noteEventTable = trackInfo->noteEventTable.getAddr<NoteEventTable>(dataBase);
      if (falseEndian) noteEventTable->bswap();
      for (int k = 0; k < noteEventTable->size; k++) {
        NoteEvent* noteEvent = noteEventTable->elems[k].getAddr<NoteEvent>(dataBase);
        if (falseEndian) noteEvent->bswap();
      }
    }

    NoteTable* noteTable = wsd->noteTable.getAddr<NoteTable>(dataBase);
    if (falseEndian) noteTable->bswap();
    for (int j = 0; j < noteTable->size; j++) {
      NoteInformationEntry* noteInformationEntry = noteTable->elems[j].getAddr<NoteInformationEntry>(dataBase);
      if (falseEndian) noteInformationEntry->bswap();
    }
  }
}

void SoundWsd::trackToWaveFile(u8 trackIdx, void* waveData, std::filesystem::path wavePath) const {
  const WaveInfo* waveInfo = getWaveInfo(trackIdx);
  u32 channelCount = waveInfo->channelCount;
    
  u32 loopStart = waveInfo->loopStart;
  u32 loopEnd = waveInfo->loopEnd;
  u32 sampleBufferSize = channelCount * loopEnd * sizeof(s16);
  s16* pcmBuffer = static_cast<s16*>(malloc(sampleBufferSize));

  for (int j = 0; j < waveInfo->channelCount; j++) {
    const SoundWaveChannelInfo* chInfo = getChannelInfo(waveInfo, j);
    const AdpcParams* adpcParams = getAdpcParams(waveInfo, chInfo);

    const u8* blockData = (const u8*)waveData + waveInfo->dataLoc + chInfo->dataOffset;

    decodeBlock(blockData, loopEnd, pcmBuffer + j, channelCount, waveInfo->format, adpcParams);
  }

  createWaveFile(wavePath, pcmBuffer, loopEnd, waveInfo->getSampleRate(), channelCount);

  free(pcmBuffer);
}
}
