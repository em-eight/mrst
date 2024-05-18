
#include <iostream>

#include "rsnd/soundCommon.hpp"
#include "rsnd/SoundWave.hpp"
#include "rsnd/SoundStream.hpp"
#include "rsnd/SoundSequence.hpp"
#include "common/fileUtil.hpp"
#include "tools/decode.hpp"
#include "tools/common.hpp"
#include "vgmtrans/MidiFile.h"

namespace rsnd {
void rsndDecodeWave(const SoundWave& soundWave, CliOpts& cliOpts) {
  if (cliOpts.outputPath.empty()) {
    auto tmp = cliOpts.inputFile;
    tmp.replace_extension(".wav");
    cliOpts.outputPath = tmp;
  }
  soundWave.toWaveFile(cliOpts.outputPath);
}

void rsndDecodeStream(const SoundStream& soundStream, CliOpts& cliOpts) {
  if (cliOpts.outputPath.empty()) {
    auto tmp = cliOpts.inputFile;
    if (soundStream.trackTable->trackCount > 1) {
      tmp.replace_extension(".d");
    } else {
      tmp.replace_extension(".wav");
    }
    cliOpts.outputPath = tmp;
  }
  if (soundStream.trackTable->trackCount > 1) {
    std::filesystem::create_directories(cliOpts.outputPath);
  }
  for (int i = 0; i < soundStream.trackTable->trackCount; i++) {
    const std::filesystem::path outpath = soundStream.trackTable->trackCount > 0 ? cliOpts.outputPath / (std::to_string(i) + ".wav") : cliOpts.outputPath;
    soundStream.trackToWaveFile(i, outpath);
  }
}

void rsndDecodeSequence(const SoundSequence& soundSequence, CliOpts& cliOpts) {
  if (cliOpts.outputPath.empty()) {
    auto tmp = cliOpts.inputFile;
    tmp.replace_extension(".mid");
    cliOpts.outputPath = tmp;
  }
  MidiFile midiFile(&soundSequence);
  midiFile.SaveMidiFile(cliOpts.outputPath);
}

void rsndDecode(CliOpts& cliOpts) {
  size_t inputSize;
  void* inputData = readBinary(cliOpts.inputFile, inputSize);
  FileFormat inputFormat = detectFileFormat(cliOpts.inputFile.filename().string(), inputData, inputSize);
  switch (inputFormat)
  {
  case FMT_BRWAV: {
    SoundWave soundWave(inputData, inputSize);
    rsndDecodeWave(soundWave, cliOpts);
    break;

  } case FMT_BRSTM: {
    SoundStream soundStream(inputData, inputSize);
    rsndDecodeStream(soundStream, cliOpts);
    break;

  } case FMT_BRSEQ: {
    SoundSequence soundSequence(inputData, inputSize);
    rsndDecodeSequence(soundSequence, cliOpts);
    break;

  } default:
    std::cerr << cliOpts.inputFile << " file format decode not supported\n";
    exit(-1);
  }

  free(inputData);
}
}