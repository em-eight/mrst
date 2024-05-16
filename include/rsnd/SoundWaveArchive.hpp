
#pragma once

#include <cstddef>

#include "common/util.h"

namespace rsnd {
struct SoundWaveArchiveHeader : public BinaryFileHeader {
  u32 tableOffset;
  u32 tableLength;
  u32 waveDataOffset;
  u32 waveDataLength;

  void bswap();
};

struct SoundWaveArchiveEntry {
  DataRef waveFileRef;
  u32 waveFileSize;

  void bswap();
};

struct SoundWaveArchiveTable : public BinaryBlockHeader {
  Array<SoundWaveArchiveEntry> entries;

  void bswap();
};

struct SoundWaveArchiveData : public BinaryBlockHeader {

  void bswap();
};

class SoundWaveArchive {
private:
  void* data;
  size_t dataSize;

public:
  SoundWaveArchiveTable* table;
  SoundWaveArchiveData* waveData;

  void* dataBase;

  SoundWaveArchive(void* fileData, size_t fileSize);
  u32 getWaveCount() const { return table->entries.size; }
  const SoundWaveArchiveEntry* getWaveEntry(u32 i) const { return &table->entries.elems[i]; }
  void* getWaveFile(u32 i, size_t& fileSize) const {
    auto waveEntry = getWaveEntry(i);
    fileSize = waveEntry->waveFileSize;
    return waveEntry->waveFileRef.getAddr(dataBase);
  }
};
}