
#include "rsnd/SoundWsd.hpp"

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

SoundWsd::SoundWsd(void* fileData, size_t fileSize) {
  dataSize = fileSize;
  data = fileData;

  WsdHeader* wsdHdr = static_cast<WsdHeader*>(fileData);
  bool falseEndian = wsdHdr->byteOrder != 0xFEFF;
  if (falseEndian) wsdHdr->bswap();

  wsdData = getOffsetT<WsdData>(data, wsdHdr->dataOffset);
  if (falseEndian) wsdData->bswap();
  dataBase = getOffset(wsdData, sizeof(BinaryBlockHeader));
  
  containsWaveInfo = wsdHdr->waveOffset != 0;
  if (containsWaveInfo) {
    wsdWave = getOffsetT<WsdWave>(data, wsdHdr->waveOffset);
    if (falseEndian) wsdWave->bswap();
    waveBase = getOffset(wsdWave, sizeof(BinaryBlockHeader));
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
}