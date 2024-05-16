
#include <bit>

#include "rsnd/soundCommon.hpp"
#include "rsnd/SoundWaveArchive.hpp"

namespace rsnd {
void SoundWaveArchiveHeader::bswap() {
  this->BinaryFileHeader::bswap();
  tableOffset = std::byteswap(tableOffset);
  tableLength = std::byteswap(tableLength);
  waveDataOffset = std::byteswap(waveDataOffset);
  waveDataLength = std::byteswap(waveDataLength);
}

void SoundWaveArchiveEntry::bswap() {
  waveFileRef.bswap();
  waveFileSize = std::byteswap(waveFileSize);
}

void SoundWaveArchiveTable::bswap() {
  this->BinaryBlockHeader::bswap();
  entries.bswap();
}

void SoundWaveArchiveData::bswap() {
  this->BinaryBlockHeader::bswap();
}

SoundWaveArchive::SoundWaveArchive(void* fileData, size_t fileSize) {
  dataSize = fileSize;
  data = fileData;

  SoundWaveArchiveHeader* warHdr = static_cast<SoundWaveArchiveHeader*>(fileData);
  bool falseEndian = isFalseEndian(warHdr->byteOrder);
  if (falseEndian) warHdr->bswap();

  table = getOffsetT<SoundWaveArchiveTable>(data, warHdr->tableOffset);
  if (falseEndian) table->bswap();

  waveData = getOffsetT<SoundWaveArchiveData>(data, warHdr->waveDataOffset);
  if (falseEndian) waveData->bswap();
  dataBase = getOffset(waveData, 0);
}
}