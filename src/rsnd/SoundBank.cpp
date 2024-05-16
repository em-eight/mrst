
#include <iostream>

#include "rsnd/SoundBank.hpp"

static f32 bswap_float(const float inFloat) {
   float retVal;
   char *floatToConvert = ( char* ) & inFloat;
   char *returnFloat = ( char* ) & retVal;

   // swap the bytes into a temporary buffer
   returnFloat[0] = floatToConvert[3];
   returnFloat[1] = floatToConvert[2];
   returnFloat[2] = floatToConvert[1];
   returnFloat[3] = floatToConvert[0];

   return retVal;
}

static int roundUp(int numToRound, int multiple) {
  if (multiple == 0)
    return numToRound;

  int remainder = numToRound % multiple;
  if (remainder == 0)
    return numToRound;

  return numToRound + multiple - remainder;
}

namespace rsnd {
void SoundBankHeader::bswap() {
  this->BinaryFileHeader::bswap();
  dataOffset = std::byteswap(dataOffset);
  dataLength = std::byteswap(dataLength);
  waveOffset = std::byteswap(waveOffset);
  waveLength = std::byteswap(waveLength);
}

void InstrInfo::bswap() {
  waveIdx = std::byteswap(waveIdx);
  pitch = bswap_float(pitch);
  lfoTable.bswap();
  graphEnvTable.bswap();
  randomizerTable.bswap();
  _res = std::byteswap(_res);
}

void IndexRegion::bswap() {
  _2 = std::byteswap(_2);
  for (int i = 0; i < max - min; i++) {
    regionRefs[i].bswap();
  }
}

void SoundBankData::bswap() {
  this->BinaryBlockHeader::bswap();
  instrs.bswap();
}

void SoundBankWave::bswap() {
  this->BinaryBlockHeader::bswap();
  waveInfos.bswap();
}

SoundBank::SoundBank(void* fileData, size_t fileSize) {
  dataSize = fileSize;
  data = fileData;

  SoundBankHeader* bnkHdr = static_cast<SoundBankHeader*>(fileData);
  bool falseEndian = bnkHdr->byteOrder != 0xFEFF;
  if (falseEndian) bnkHdr->bswap();

  bankData = getOffsetT<SoundBankData>(data, bnkHdr->dataOffset);
  if (falseEndian) bankData->bswap();
  dataBase = getOffset(bankData, sizeof(BinaryBlockHeader));

  containsWaves = bnkHdr->waveOffset != 0;
  if (containsWaves) {
    bankWave = getOffsetT<SoundBankWave>(data, bnkHdr->waveOffset);
    if (falseEndian) bankWave->bswap();
    waveBase = getOffset(bankWave, sizeof(BinaryBlockHeader));
  } else {
    bankWave = nullptr;
  }

  // Each instrument has a region for note and a subregion for velocity
  // i is the index of the instrument. Regions are followed according to chosen key+velocity to get to InstrInfo
  for (int i = 0; i < bankData->instrs.size; i++) {
    auto& regionRef = bankData->instrs.elems[i];
    bswapRegionsRecurse(regionRef);
  }
}

void SoundBank::bswapRegionsRecurse(DataRef& regionRef) {
  RegionSet regionType = static_cast<RegionSet>(regionRef.dataType);
  switch (regionType) {
  case REGIONSET_RANGE: {
    RangeTable* rangeTable = regionRef.getAddr<RangeTable>(dataBase);
    rangeTable->bswap();
    for (int i = 0; i < rangeTable->rangeCount; i++) {
      DataRef* dataRef = getSubregionRef(&regionRef, rangeTable->key[i]);
      dataRef->bswap();
      bswapRegionsRecurse(*dataRef);
    }
    break;
  } case REGIONSET_INDEX: {
    IndexRegion* indexRegion = regionRef.getAddr<IndexRegion>(dataBase);
    indexRegion->bswap();
    for (int i = indexRegion->min; i < indexRegion->max; i++) {
      bswapRegionsRecurse(*getSubregionRef(&regionRef, i));
    }
    break;
  } case REGIONSET_DIRECT: {
    InstrInfo* instrInfo = regionRef.getAddr<InstrInfo>(dataBase);
    instrInfo->bswap();
    break;
  } case REGIONSET_NONE: {
    break;
  } default:
    std::cerr << "Warning: unknown subregion reference " << regionType << '\n';
  }
}

DataRef* SoundBank::getSubregionRef(const DataRef* ref, int idx) const {
  RegionSet regionType = static_cast<RegionSet>(ref->dataType);
  switch (regionType) {
  case REGIONSET_RANGE: {
    RangeTable* rangeTable = ref->getAddr<RangeTable>(dataBase);
    u8 i = 0;
    while (idx > rangeTable->key[i]) {
      i++;
    }
    int offset = roundUp(sizeof(rangeTable->rangeCount) + rangeTable->rangeCount, 4) + sizeof(DataRef) * i;
    return getOffsetT<DataRef>(rangeTable, offset);
  } case REGIONSET_INDEX: {
    IndexRegion* indexRegion = ref->getAddr<IndexRegion>(dataBase);
    return &indexRegion->regionRefs[idx - indexRegion->min];
  } case REGIONSET_DIRECT: {
    return const_cast<DataRef*>(ref);
  } case REGIONSET_NONE: {
    return nullptr;
  } default:
    std::cerr << "Warning: unknown subregion reference " << regionType << '\n';
    return nullptr;
  }
  return nullptr;
}

InstrInfo* SoundBank::getInstrInfo(int progIdx, int key, int velocity) {
  // programs -> keys -> velocities
  DataRef* ref = &bankData->instrs.elems[progIdx];

  if (ref->dataType == REGIONSET_NONE) return nullptr;
  if (ref->dataType != REGIONSET_DIRECT) {
    ref = getSubregionRef(ref, key);
  }

  if (ref->dataType == REGIONSET_NONE) return nullptr;
  if (ref->dataType != REGIONSET_DIRECT) {
    ref = getSubregionRef(ref, velocity);
  }

  return ref->getAddr<InstrInfo>(dataBase);
}

std::vector<SoundBank::Subregion> SoundBank::getSubregions(const DataRef* regionRef) const {
  RegionSet regionType = static_cast<RegionSet>(regionRef->dataType);
  switch (regionType) {
  case REGIONSET_RANGE: {
    RangeTable* rangeTable = regionRef->getAddr<RangeTable>(dataBase);
    std::vector<SoundBank::Subregion> subregions;
    for (int i = 0; i < rangeTable->rangeCount; i++) {
      DataRef* dataRef = getSubregionRef(regionRef, rangeTable->key[i]);
      Subregion region = {rangeTable->key[i], dataRef};
      subregions.push_back(region);
    }
    return subregions;
  } case REGIONSET_INDEX: {
    IndexRegion* indexRegion = regionRef->getAddr<IndexRegion>(dataBase);
    std::vector<SoundBank::Subregion> subregions;
    for (u8 i = indexRegion->min; i < indexRegion->max; i++) {
      DataRef* dataRef = getSubregionRef(regionRef, i);
      Subregion region = {i, dataRef};
      subregions.push_back(region);
    }
    return subregions;
  } case REGIONSET_DIRECT: {
    InstrInfo* instrInfo = regionRef->getAddr<InstrInfo>(dataBase);
    return { { 0x7F, regionRef } };
  } case REGIONSET_NONE: {
    return {};
  } default:
    std::cerr << "Warning: unknown subregion reference " << regionType << '\n';
    return {};
  }
}

std::vector<SoundBank::InstrumentRegion> SoundBank::getInstrRegions(int progIdx) const {
  std::vector<SoundBank::InstrumentRegion> instrRegions;
  DataRef* ref = &bankData->instrs.elems[progIdx];

  // key ranges
  std::vector<SoundBank::Subregion> keyRegions = getSubregions(ref);
  for (int i = 0; i < keyRegions.size(); i++) {
    u8 keyLo = (i > 0) ? keyRegions[i - 1].high + 1 : 0;
    u8 keyHi = keyRegions[i].high;

    // velocity ranges
    std::vector<SoundBank::Subregion> velRegions = getSubregions(keyRegions[i].ref);
    for (int j = 0; j < velRegions.size(); j++) {
      u8 velLo = (j > 0) ? velRegions[j - 1].high + 1 : 0;
      u8 velHi = velRegions[j].high;
      InstrInfo* instrInfo = velRegions[j].ref->getAddr<InstrInfo>(dataBase);

      InstrumentRegion instrRegion;
      instrRegion.keyLo = keyLo;
      instrRegion.keyHi = keyHi;
      instrRegion.velLo = velLo;
      instrRegion.velHi = velHi;
      instrRegion.instrInfo = instrInfo;
      instrRegions.push_back(instrRegion);
    }
  }

  return instrRegions;
}
}