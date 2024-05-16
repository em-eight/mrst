
#include <iostream>

#include "rsnd/soundCommon.hpp"
#include "rsnd/SoundArchive.hpp"
#include "rsnd/SoundBank.hpp"
#include "rsnd/SoundSequence.hpp"
#include "common/fileUtil.hpp"
#include "tools/list.hpp"

namespace rsnd {
void rsndListRsarGroups(const SoundArchive& soundArchive, CliOpts& cliOpts) {
  GroupTable* groupTable = soundArchive.groupTable;
  for (int i = 0; i < groupTable->size; i++) {
    const GroupInfo* groupInfo = soundArchive.getGroupInfo(i);
    const char* name = soundArchive.getString(groupInfo->nameIdx);
    if (name) {
      std::cout << name << '\n';
    } else {
      std::cout << "<anonymous group>" << '\n';
    }
  }
}

void rsndListRsarBanks(const SoundArchive& soundArchive, CliOpts& cliOpts) {
  BankTable* bankTable = soundArchive.bankTable;
  for (int i = 0; i < bankTable->size; i++) {
    const BankInfo* bankInfo = soundArchive.getBankInfo(i);
    const char* name = soundArchive.getString(bankInfo->fileNameIdx);
    if (name) {
      std::cout << name << '\n';
    } else {
      std::cout << "<anonymous bank>" << '\n';
    }
  }
}

void rsndListRsar(const SoundArchive& soundArchive, CliOpts& cliOpts) {
  if (cliOpts.listOpts.groups) {
    rsndListRsarGroups(soundArchive, cliOpts);
  }
  if (cliOpts.listOpts.banks) {
    rsndListRsarBanks(soundArchive, cliOpts);
  }
}

void printSubregionRecurse(const SoundBank& soundBank, DataRef* ref, int depth) {
  switch (ref->dataType) {
  case REGIONSET_DIRECT: {
    for (int i = 0; i < depth; i++) std::cout << "    ";
    InstrInfo* instrInfo = ref->getAddr<InstrInfo>(soundBank.dataBase);
    std::cout << "sample #: " << std::to_string(instrInfo->waveIdx) << '\n';
    break;
  
  } case REGIONSET_RANGE: {
    RangeTable* rangeTable = ref->getAddr<RangeTable>(soundBank.dataBase);
    for (int i = 0; i < rangeTable->rangeCount; i++) {
      for (int i = 0; i < depth; i++) std::cout << "    ";
      std::cout << "range: up to " << (int)rangeTable->key[i] << '\n';
      DataRef* dataRef = const_cast<SoundBank&>(soundBank).getSubregionRef(ref, rangeTable->key[i]);
      printSubregionRecurse(soundBank, dataRef, depth + 1);
    }
    
    break;

  } case REGIONSET_INDEX: {
    IndexRegion* indexRegion = ref->getAddr<IndexRegion>(soundBank.dataBase);
    for (int i = indexRegion->min; i < indexRegion->max; i++) {
      for (int i = 0; i < depth; i++) std::cout << "    ";
      std::cout << "index " << std::to_string(i) << '\n';
      printSubregionRecurse(soundBank, const_cast<SoundBank&>(soundBank).getSubregionRef(ref, i), depth + 1);
    }
  
    break;
  
  } case REGIONSET_NONE: {
    for (int i = 0; i < depth; i++) std::cout << "    ";
    std::cout << "sample: none" << '\n';

    break;
  } default:
    break;
  }
}

void rsndListRbnk(const SoundBank& soundBank, CliOpts& cliOpts) {
  const int progCount = soundBank.bankData->instrs.size;
  std::cout << "Program count: " << progCount << '\n';

  for (int i = 0; i < progCount; i++) {
    std::cout << "Program " << std::to_string(i) << '\n';
    printSubregionRecurse(soundBank, &soundBank.bankData->instrs.elems[i], 1);
  }

  if (soundBank.containsWaves) {

  } else {
    std::cout << "Bank file does not embed any WAVE files\n";
  }
}

void rsndListRseq(const SoundSequence& soundSeq, CliOpts& cliOpts) {
  const int labelCount = soundSeq.label->labelOffs.size;
  std::cout << "Label count: " << labelCount << '\n';

  for (int i = 0; i < labelCount; i++) {
    const auto* seqLabel = soundSeq.getSeqLabel(i);
    std::cout << "- " << seqLabel->nameStr() << '\n';
  }
}

void rsndList(CliOpts& cliOpts) {
  size_t inputSize;
  void* inputData = readBinary(cliOpts.inputFile, inputSize);
  FileFormat inputFormat = detectFileFormat(cliOpts.inputFile.filename().string(), inputData, inputSize);
  switch (inputFormat)
  {
  case FMT_BRSAR: {
    SoundArchive soundWave(inputData, inputSize);
    rsndListRsar(soundWave, cliOpts);
    break;

  } case FMT_BRBNK: {
    SoundBank soundBank(inputData, inputSize);
    rsndListRbnk(soundBank, cliOpts);
    break;

  } case FMT_BRSEQ: {
    SoundSequence soundSeq(inputData, inputSize);
    rsndListRseq(soundSeq, cliOpts);
    break;

  } default:
    std::cerr << cliOpts.inputFile << " file format list not supported\n";
    exit(-1);
  }

  free(inputData);
}
}