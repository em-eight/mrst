
#pragma once

#include "common/util.h"
#include "rsnd/soundCommon.hpp"

namespace rsnd {
struct WsdHeader : public BinaryFileHeader {
  u32 dataOffset;
  u32 dataLength;
  u32 waveOffset;
  u32 waveLength;

  void bswap();
};

struct WsdData : public BinaryBlockHeader {
  Array<DataRef> refs;

  void bswap();
};

struct Wsd {
  DataRef wsdInfo;
  DataRef trackTable;
  DataRef noteTable;

  void bswap();
};

struct WsdInfo {
  f32 pitch;
  u8 pan;
  u8 surroundPan;
  u8 fxSendA;
  u8 fxSendB;
  u8 fxSendC;
  s8 mainSend;
  s8 _a[2];
  DataRef _c;
  DataRef _14;
  u32 _1c;

  void bswap();
};

typedef Array<DataRef> WsdTrackTable;

struct TrackInfo {
  DataRef noteEventTable;

  void bswap();
};

typedef Array<DataRef> NoteEventTable;

struct NoteEvent {
  f32 position;
  f32 length;
  u32 noteIdx;
  u32 _c;

  void bswap();
};

typedef Array<DataRef> NoteTable;

struct NoteInformationEntry {
  s32 waveIdx;
  s8 attack;
  s8 decay;
  s8 sustain;
  s8 release;
  s8 hold;
  u8 waveDataLocationType;
  u8 noteOffType;
  u8 alternateAssign;
  u8 originalKey;
  u8 volume;
  u8 pan;
  u8 surroundPan;
  f32 pitch;
  DataRef lfoTable;
  DataRef graphEnvTable;
  DataRef randomizerTable;
  u32 _res;

  void bswap();
};

struct WsdWave : public BinaryBlockHeader {
  Array<u32> waveInfos;
};

class SoundWsd {
private:
  void* data;
  size_t dataSize;

public:
  WsdData* wsdData;
  WsdWave* wsdWave;

  void* dataBase;
  void* waveBase;

  bool containsWaveInfo;

  SoundWsd(void* fileData, size_t fileSize);

  u32 getWsdCount() const { return wsdData->refs.size; }
  const Wsd* getWsd(u32 i) const { return wsdData->refs.elems[i].getAddr<const Wsd>(dataBase); }
  u32 getTrackCount(const Wsd* wsd) const { return wsd->trackTable.getAddr<WsdTrackTable>(dataBase)->size; }
  const TrackInfo* getTrackInfo(const Wsd* wsd, int i) const { return wsd->trackTable.getAddr<WsdTrackTable>(dataBase)->elems[i].getAddr<TrackInfo>(dataBase); }
  const NoteEventTable* getTrackNoteEventTable(const Wsd* wsd, int i) const { return getTrackInfo(wsd, i)->noteEventTable.getAddr<NoteEventTable>(dataBase); }
};
}