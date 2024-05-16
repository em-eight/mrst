
#pragma once

#include <cstddef>
#include <string>

#include "common/util.h"
#include "rsnd/soundCommon.hpp"

namespace rsnd {
struct SoundSequenceHeader : public BinaryFileHeader {
  u32 dataOffset;
  u32 dataLength;
  u32 lablOffset;
  u32 lablLength;

  void bswap();
};

struct SoundSequenceData : public BinaryBlockHeader {
  u32 offset;

  void bswap();
};

struct SoundSequenceLabel : public BinaryBlockHeader {
  Array<u32> labelOffs;

  void bswap();
};

struct SeqLabel {
  u32 dataOffset;
  u32 nameLength;
  char name[1];

  void bswap();
  std::string nameStr() const;
};

class SoundSequence {
private:
  void* data;
  size_t dataSize;

public:
  SoundSequenceData* seqData;
  SoundSequenceLabel* label;

  void* dataBase;
  void* labelBase;

  SoundSequence(void* fileData, size_t fileSize);

  const SeqLabel* getSeqLabel(u32 i) const { return getOffsetT<SeqLabel>(labelBase, label->labelOffs.elems[i]); }
};
}