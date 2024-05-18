
#pragma once

#include <cstddef>
#include <string>

#include "common/util.h"
#include "rsnd/soundCommon.hpp"

namespace rsnd {
// from https://github.com/libertyernie/brawltools/blob/cad9a29a49d8884c9ec2f15e5c21a7dbb5fe0ac2/BrawlLib/SSBB/Types/Audio/RSEQ.cs#L124
enum Mml {
  // Variable length parameter commands.
  MML_WAIT = 0x80,
  MML_PRG = 0x81,

  // Control commands.
  MML_OPEN_TRACK = 0x88,
  MML_JUMP = 0x89,
  MML_CALL = 0x8a,

  // prefix commands
  MML_RANDOM = 0xa0,
  MML_VARIABLE = 0xa1,
  MML_IF = 0xa2,
  MML_TIME = 0xa3,
  MML_TIME_RANDOM = 0xa4,
  MML_TIME_VARIABLE = 0xa5,

  // u8 parameter commands.
  MML_TIMEBASE = 0xb0,
  MML_ENV_HOLD = 0xb1,
  MML_MONOPHONIC = 0xb2,
  MML_VELOCITY_RANGE = 0xb3,
  MML_BIQUAD_TYPE = 0xb4,
  MML_BIQUAD_VALUE = 0xb5,
  MML_PAN = 0xc0,
  MML_VOLUME = 0xc1,
  MML_MAIN_VOLUME = 0xc2,
  MML_TRANSPOSE = 0xc3,
  MML_PITCH_BEND = 0xc4,
  MML_BEND_RANGE = 0xc5,
  MML_PRIO = 0xc6,
  MML_NOTE_WAIT = 0xc7,
  MML_TIE = 0xc8,
  MML_PORTA = 0xc9,
  MML_MOD_DEPTH = 0xca,
  MML_MOD_SPEED = 0xcb,
  MML_MOD_TYPE = 0xcc,
  MML_MOD_RANGE = 0xcd,
  MML_PORTA_SW = 0xce,
  MML_PORTA_TIME = 0xcf,
  MML_ATTACK = 0xd0,
  MML_DECAY = 0xd1,
  MML_SUSTAIN = 0xd2,
  MML_RELEASE = 0xd3,
  MML_LOOP_START = 0xd4,
  MML_VOLUME2 = 0xd5,
  MML_PRINTVAR = 0xd6,
  MML_SURROUND_PAN = 0xd7,
  MML_LPF_CUTOFF = 0xd8,
  MML_FXSEND_A = 0xd9,
  MML_FXSEND_B = 0xda,
  MML_MAINSEND = 0xdb,
  MML_INIT_PAN = 0xdc,
  MML_MUTE = 0xdd,
  MML_FXSEND_C = 0xde,
  MML_DAMPER = 0xdf,

  // s16 parameter commands.
  MML_MOD_DELAY = 0xe0,
  MML_TEMPO = 0xe1,
  MML_SWEEP_PITCH = 0xe3,

  // Extended commands.
  MML_EX_COMMAND = 0xf0,

  // Other
  MML_ENV_RESET = 0xfb,
  MML_LOOP_END = 0xfc,
  MML_RET = 0xfd,
  MML_ALLOC_TRACK = 0xfe,
  MML_FIN = 0xff
};

enum MmlEx {
  MML_SETVAR = 0x80,
  MML_ADDVAR = 0x81,
  MML_SUBVAR = 0x82,
  MML_MULVAR = 0x83,
  MML_DIVVAR = 0x84,
  MML_SHIFTVAR = 0x85,
  MML_RANDVAR = 0x86,
  MML_ANDVAR = 0x87,
  MML_ORVAR = 0x88,
  MML_XORVAR = 0x89,
  MML_NOTVAR = 0x8a,
  MML_MODVAR = 0x8b,

  MML_CMP_EQ = 0x90,
  MML_CMP_GE = 0x91,
  MML_CMP_GT = 0x92,
  MML_CMP_LE = 0x93,
  MML_CMP_LT = 0x94,
  MML_CMP_NE = 0x95,

  MML_USERPROC = 0xe0
};

enum SeqArgType {
  SEQ_ARG_NONE,
  SEQ_ARG_U8,
  SEQ_ARG_S16,
  SEQ_ARG_VMIDI,
  SEQ_ARG_RANDOM,
  SEQ_ARG_VARIABLE
};

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
  const void* getSeqData() const { return getOffsetT<const void>(dataBase, 0); }
  const u32 getLabelOffset(const SeqLabel* label) const { return label->dataOffset; }
};
}