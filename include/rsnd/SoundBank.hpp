
/**
 * Credits to https://github.com/magcius/vgmtrans/blob/siiva/src/main/formats/RSARInstrSet.cpp for a lot of this
*/

#pragma once

#include <cstddef>
#include <filesystem>
#include <vector>

#include "common/util.h"
#include "rsnd/soundCommon.hpp"
#include "rsnd/SoundWave.hpp"

namespace rsnd {
struct SoundBankHeader : public BinaryFileHeader {
  u32 dataOffset;
  u32 dataLength;
  // if non-zero, references to waves are within bank file, external RWAR otherwise
  u32 waveOffset;
  u32 waveLength;

  void bswap();
};

struct InstrInfo {
  u32 waveIdx;
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

enum RegionSet {
  REGIONSET_DIRECT = 1,
  REGIONSET_RANGE = 2,
  REGIONSET_INDEX = 3,
  REGIONSET_NONE = 4
};

struct IndexRegion {
  u8 min;
  u8 max;
  u16 _2;
  // References to subregions
  DataRef regionRefs[1];

  void bswap();
};

struct RangeTable {
  u8 rangeCount;
  u8 key[1];

  void bswap() {}
};

struct SoundBankData : public BinaryBlockHeader {
  // defines key and velocity regions
  // reference to InstrInfo, RangeTable, or Index region
  Array<DataRef> instrs;

  void bswap();
};

struct SoundBankWave : public BinaryBlockHeader {
  // references to WaveInfo
  Array<DataRef> waveInfos;

  void bswap();
};

class SoundBank {
private:
  struct Subregion {
    s16 high;
    const DataRef* ref;
  };

  void* data;
  size_t dataSize;

  void bswapRegionsRecurse(DataRef& regionRef);
  std::vector<Subregion> getSubregions(const DataRef* ref) const;

public:
  struct InstrumentRegion {
    s16 keyLo;
    s16 keyHi;
    s16 velLo;
    s16 velHi;

    InstrInfo* instrInfo;
  };

  SoundBankData* bankData;
  SoundBankWave* bankWave;

  void* dataBase;
  void* waveBase;

  bool containsWaves;

  SoundBank(void* fileData, size_t fileSize, void* waveData=nullptr);

  DataRef* getSubregionRef(const DataRef* ref, int idx) const;
  u32 getInstrCount() const { return bankData->instrs.size; }
  InstrInfo* getInstrInfo(int progIdx, int key, int velocity);
  std::vector<InstrumentRegion> getInstrRegions(int progIdx) const;

  const WaveInfo* getWaveInfo(int i) const { return bankWave->waveInfos.elems[i].getAddr<WaveInfo>(waveBase); }
  int getWaveInfoCount() const { return bankWave->waveInfos.size; }
  const SoundWaveChannelInfo* getChannelInfo(const WaveInfo* waveInfo, int i) const { 
    const u32* channelInfoOffsets = getOffsetT<u32>(waveInfo, waveInfo->channelInfoTableOffset);
    return getOffsetT<SoundWaveChannelInfo>(waveInfo, channelInfoOffsets[i]);
  }
  int getChannelCount(const WaveInfo* waveInfo) const { return waveInfo->channelCount; }
  const AdpcParams* getAdpcParams(const WaveInfo* waveInfo, const SoundWaveChannelInfo* chInfo) const { return getOffsetT<AdpcParams>(waveInfo, chInfo->adpcmOffset); }
};
}