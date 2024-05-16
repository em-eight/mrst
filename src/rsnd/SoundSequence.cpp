
#include <bit>

#include "rsnd/SoundSequence.hpp"

namespace rsnd {
void SoundSequenceHeader::bswap() {
  this->BinaryFileHeader::bswap();
  dataOffset = std::byteswap(dataOffset);
  dataLength = std::byteswap(dataLength);
  lablOffset = std::byteswap(lablOffset);
  lablLength = std::byteswap(lablLength);
}

void SoundSequenceData::bswap() {
  this->BinaryBlockHeader::bswap();
  offset = std::byteswap(offset);
}

void SoundSequenceLabel::bswap() {
  this->BinaryBlockHeader::bswap();
  labelOffs.bswap();
}

void SeqLabel::bswap() {
  dataOffset = std::byteswap(dataOffset);
  nameLength = std::byteswap(nameLength);
}

std::string SeqLabel::nameStr() const {
  return std::string(name, nameLength);
}

SoundSequence::SoundSequence(void* fileData, size_t fileSize) {
  dataSize = fileSize;
  data = fileData;

  SoundSequenceHeader* seqHdr = static_cast<SoundSequenceHeader*>(fileData);
  bool falseEndian = seqHdr->byteOrder != 0xFEFF;
  if (falseEndian) seqHdr->bswap();

  seqData = getOffsetT<SoundSequenceData>(data, seqHdr->dataOffset);
  if (falseEndian) seqData->bswap();
  dataBase = getOffset(seqData, sizeof(BinaryBlockHeader));

  label = getOffsetT<SoundSequenceLabel>(data, seqHdr->lablOffset);
  if (falseEndian) label->bswap();
  labelBase = getOffset(label, sizeof(BinaryBlockHeader));

  for (int i = 0; i < label->labelOffs.size; i++) {
    auto labelOff = label->labelOffs.elems[i];
    auto seqLabel = getOffsetT<SeqLabel>(labelBase, labelOff);
    if (falseEndian) seqLabel->bswap();
    // TODO: byteswap seq data if needed
  }
}
}