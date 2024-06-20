
#include <iostream>

#include "rsnd/soundCommon.hpp"
#include "rsnd/SoundArchive.hpp"
#include "rsnd/SoundBank.hpp"
#include "rsnd/SoundSequence.hpp"
#include "rsnd/SoundStream.hpp"
#include "rsnd/SoundWave.hpp"
#include "rsnd/SoundWaveArchive.hpp"
#include "rsnd/SoundWsd.hpp"
#include "common/fileUtil.hpp"
#include "tools/list.hpp"

namespace rsnd {
void rsndListFileInfo(const SoundArchive& soundArchive, int fileIdx) {
  const FileInfo* fileInfo = soundArchive.getFileInfo(fileIdx);
  if (soundArchive.isFileExternal(fileIdx)) {
    std::cout << '\t' << soundArchive.getFileExternalPath(fileIdx) << '\n';
  } else {
    const FileGroupInfo* fileGroupInfo = soundArchive.getFileGroupInfo(fileIdx);
    int instanceCount = fileGroupInfo->size;
    for (int j = 0; j < instanceCount; j++) {
      const FileGroup* fileGroup = soundArchive.getFileGroup(fileIdx, j);
      const GroupInfo* group = soundArchive.getGroupInfo(fileGroup->groupIdx);
      const char* name = soundArchive.getString(group->nameIdx);
      if (name) {
        std::cout << '\t' << name << ":" << fileGroup->idx << '\n';
      } else {
        std::cout << '\t' << "<anonymous group>" << ":" << fileGroup->idx << '\n';
      }
    }
  }
}

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
      std::cout << i << ") " << name << '\n';
    } else {
      std::cout << i << ") " << "<anonymous bank>" << '\n';
    }
    rsndListFileInfo(soundArchive, bankInfo->fileIdx);
  }
}

std::string rseqShortDesc(const SoundArchive& soundArchive, const SeqSoundInfo* seqSoundInfo) {
  std::string desc;
  const BankInfo* bankInfo = soundArchive.getBankInfo(seqSoundInfo->bankIdx);
  const char* name = soundArchive.getString(bankInfo->fileNameIdx);
  if (name) {
    desc += name;
  } else {
    desc += "<anonymous bank>";
  }
  return "off: " + std::to_string(seqSoundInfo->offset) + ", " + desc;
}

std::string rstmShortDesc(const SoundArchive& SoundArchive, const StrmSoundInfo* strmSoundInfo) {
  return "+" + std::to_string(strmSoundInfo->startPos);
}

std::string rwsdShortDesc(const SoundArchive& SoundArchive, const WsdSoundInfo* wsdSoundInfo) {
  return "#" + std::to_string(wsdSoundInfo->idx);
}

void rsndListRsarSounds(const SoundArchive& soundArchive, CliOpts& cliOpts) {
  SoundTable* soundTable = soundArchive.soundTable;
  for (int i = 0; i < soundTable->size; i++) {
    const SoundInfoEntry* soundInfo = soundArchive.getSoundInfo(i);
    const char* name = soundArchive.getString(soundInfo->fileNameIdx);
    const char* typeStr;
    std::string description;
    switch (soundInfo->soundType)
    {
    case SoundInfoEntry::TYPE_SEQ:
      typeStr = "SEQ";
      description = rseqShortDesc(soundArchive, soundArchive.getSeqSoundInfo(soundInfo));
      break;
    
    case SoundInfoEntry::TYPE_STRM:
      typeStr = "STRM";
      description = rstmShortDesc(soundArchive, soundArchive.getStrmSoundInfo(soundInfo));
      break;
    
    case SoundInfoEntry::TYPE_WAVE:
      typeStr = "WAVE";
      description = rwsdShortDesc(soundArchive, soundArchive.getWsdSoundInfo(soundInfo));
      break;
    
    default:
      typeStr = "UNK";
      description = "";
      break;
    }
    if (name) {
      std::cout << i << ") " << name << " | " << typeStr << " | " << description << '\n';
    } else {
      std::cout << i << ") " << "<anonymous sound>" << '\n';
    }

    rsndListFileInfo(soundArchive, soundInfo->fileIdx);
  }
}

void rsndListRsar(const SoundArchive& soundArchive, CliOpts& cliOpts) {
  if (cliOpts.listOpts.groups) {
    rsndListRsarGroups(soundArchive, cliOpts);
  }
  if (cliOpts.listOpts.banks) {
    rsndListRsarBanks(soundArchive, cliOpts);
  }
  if (cliOpts.listOpts.sounds) {
    rsndListRsarSounds(soundArchive, cliOpts);
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

void rsndListRstm(const SoundStream& soundStream, CliOpts& cliOpts) {
  const int trackCount = soundStream.trackTable->trackCount;
  std::cout << "Track count: " << trackCount << '\n';
  std::cout << "Sample rate: " << (int)soundStream.strmDataInfo->sampleRate << '\n';
  std::cout << "Sample count: " << (int)soundStream.getSampleCount() << '\n';

  for (int i = 0; i < trackCount; i++) {
    std::cout << "Track " << std::to_string(i) << '\n';
    u8 chCount = soundStream.getTrackInfoType() == TrackTable::EXTENDED ?
                soundStream.getTrackInfoExtended(i)->channelCount :
                soundStream.getTrackInfoSimple(i)->channelCount;
    std::cout << "\t# channels: " << (int)chCount << '\n';
  }
}

void rsndListRwav(const SoundWave& soundWave, CliOpts& cliOpts) {
  std::cout << "Channel count: " << (int)soundWave.getChannelCount() << '\n';
  std::cout << "Sample rate: " << (int)soundWave.info->sampleRate << '\n';
  std::cout << "Sample count: " << (int)soundWave.getTrackSampleCount() << '\n';
}

void rsndListRwar(const SoundWaveArchive& soundArchive, CliOpts& cliOpts) {
  std::cout << "WAVE count: " << (int)soundArchive.getWaveCount() << '\n';
}

void rsndListRseq(const SoundSequence& soundSeq, CliOpts& cliOpts) {
  const int labelCount = soundSeq.label->labelOffs.size;
  std::cout << "Label count: " << labelCount << '\n';

  for (int i = 0; i < labelCount; i++) {
    const auto* seqLabel = soundSeq.getSeqLabel(i);
    std::cout << i << ") " << seqLabel->nameStr() << ", offset: " << seqLabel->dataOffset << '\n';
  }
}

void rsndListRwsd(const SoundWsd& soundWsd, CliOpts& cliOpts) {
  int wsdCount = soundWsd.getWsdCount();
  std::cout << "Sound count: " << wsdCount << '\n';
  for (int i = 0; i < wsdCount; i++) {
    const Wsd* wsd = soundWsd.getWsd(i);
    int trackCount = soundWsd.getTrackCount(wsd);
    std::cout << "\t" << i << ") " << trackCount << " tracks" << '\n';
  }

  if (soundWsd.containsWaveInfo) {

  } else {
    std::cout << "WSD file does not embed any WAVE info\n";
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

  } case FMT_BRSTM: {
    SoundStream soundStream(inputData, inputSize);
    rsndListRstm(soundStream, cliOpts);
    break;

  } case FMT_BRWAV: {
    SoundWave soundWave(inputData, inputSize);
    rsndListRwav(soundWave, cliOpts);
    break;

  } case FMT_BRWAR: {
    SoundWaveArchive soundArchive(inputData, inputSize);
    rsndListRwar(soundArchive, cliOpts);
    break;

  } case FMT_BRWSD: {
    SoundWsd soundWsd(inputData, inputSize);
    rsndListRwsd(soundWsd, cliOpts);
    break;

  } default:
    std::cerr << cliOpts.inputFile << " file format list not supported\n";
    exit(-1);
  }

  free(inputData);
}
}