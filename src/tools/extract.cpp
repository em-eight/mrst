
#include <iostream>
#include <filesystem>
#include <cstring>
#include <algorithm>

#include "rsnd/SoundArchive.hpp"
#include "rsnd/SoundWaveArchive.hpp"
#include "rsnd/SoundBank.hpp"
#include "rsnd/soundCommon.hpp"
#include "common/cli.h"
#include "common/fileUtil.hpp"
#include "tools/common.hpp"
#include "tools/decode.hpp"

#include "vgmtrans/SF2File.h"

using namespace rsnd;

namespace rsnd {
void rsndExtractRwar(const SoundWaveArchive& waveArchive, const CliOpts& cliOpts) {
  auto contentsDir = cliOpts.outputPath;

  const int waveCount = waveArchive.getWaveCount();
  for (int i = 0; i < waveCount; i++) {
    size_t size;
    void* waveData = waveArchive.getWaveFile(i, size);
    if (size > 0) {
      auto magic = magicLowercase(waveData);
      auto wavPath = contentsDir / (std::to_string(i) + ".b" + magic);
      writeBinary(wavPath, waveData, size);

      if (cliOpts.extractOpts.decode) {
        CliOpts decodeOpts = cliOpts;
        decodeOpts.inputFile = wavPath;
        decodeOpts.outputPath = ""; // auto-figure out path from input
        rsndDecode(decodeOpts);
      }
    }
  }
}

void extract_brsar_sounds(const SoundArchive& soundArchive, const CliOpts& cliOpts) {
  auto contentsDir = cliOpts.outputPath;

  const SoundTable* soundTable = soundArchive.soundTable;
  for (int i = 0; i < soundTable->size; i++) {
    const SoundInfoEntry* soundInfo = soundArchive.getSoundInfo(i);
    // TODO
  }
}

void extract_rbnk_sf2(const std::filesystem::path filepath, void* fileData, size_t fileSize, void* waveData, size_t waveSize) {
  if (waveSize <= 0) {
    std::cerr << "Err: RBNK with embeddded wave files not yet supported\n";
    return;
  }
  
  SoundBank soundBank(fileData, fileSize);
  SoundWaveArchive waveArchive(waveData, waveSize);
  std::vector<SoundWave> waves;
  for (int i = 0; i < waveArchive.getWaveCount(); i++) {
    size_t rwavSize;
    void* rwavData = waveArchive.getWaveFile(i, rwavSize);
    waves.emplace_back(rwavData, rwavSize);
  }

  SF2File sf2file(&soundBank, waves);
  sf2file.SaveSF2File(filepath);
}

void extract_brsar_groups(const SoundArchive& soundArchive, const CliOpts& cliOpts) {
  auto contentsDir = cliOpts.outputPath;

  GroupTable* groupTable = soundArchive.groupTable;
  for (int i = 0; i < groupTable->size; i++) {
    const GroupInfo* groupInfo = soundArchive.getGroupInfo(i);
    const char* name = soundArchive.getString(groupInfo->nameIdx);
    if (!name) continue;
    if (soundArchive.isGroupExternal(i)) continue;

    std::filesystem::path groupPath = contentsDir / name;
    std::filesystem::create_directories(groupPath);

    const int groupSize = soundArchive.getGroupSize(groupInfo);
    for (int j = 0; j < groupSize; j++) {
      const GroupItemInfo* groupItemInfo = soundArchive.getGroupItemInfo(i, j);
    
      std::filesystem::path subGroupPath = groupPath / std::to_string(j);
      std::filesystem::create_directories(subGroupPath);

      size_t fileSize;
      void* fileData = soundArchive.getInternalFileData(groupInfo, groupItemInfo, &fileSize);
      // write main file data
      FileFormat fileFormat = detectFileFormat("", fileData, fileSize);
      if (fileSize > 0) {
        auto magic = magicLowercase(fileData);
        writeBinary(subGroupPath / ("file.b" + magic), fileData, fileSize);
      }

      size_t waveSize;
      void* waveData = soundArchive.getInternalWaveData(groupInfo, groupItemInfo, &waveSize);

      // write sf2 file for RBNK
      if (fileFormat == FMT_BRBNK && cliOpts.extractOpts.decode) {
        extract_rbnk_sf2(subGroupPath / "soundfont.sf2", fileData, fileSize, waveData, waveSize);
      }

      // write wave data
      if (waveSize > 0) {
        auto magic = magicLowercase(waveData);
        std::filesystem::path wavePath = subGroupPath / ("wave.b" + magic);
        writeBinary(wavePath, waveData, waveSize);

        if (cliOpts.extractOpts.rsarExtractOpts.extractRwars) {
          CliOpts waveOpts = cliOpts;
          waveOpts.outputPath = wavePath.string() + ".d";
          if (fileFormat == FMT_BRBNK) waveOpts.extractOpts.decode = false; // rwav samples would be already decoded to sf2
          std::filesystem::create_directories(waveOpts.outputPath);
          SoundWaveArchive waveArchive(waveData, waveSize);
          rsndExtractRwar(waveArchive, waveOpts);
        }
      }
    }
  }
}

void rsndExtractRsar(const SoundArchive& soundArchive, const CliOpts cliOpts) {
  switch (cliOpts.extractOpts.rsarExtractOpts.extractStyle)
  {
  case EXTRACT_GROUPS:
    extract_brsar_groups(soundArchive, cliOpts);
    break;
  
  default:
    std::cerr << "Unknown extraction style " << cliOpts.extractOpts.rsarExtractOpts.extractStyle << '\n';
    exit(-1);
    break;
  }
}

void rsndExtract(const CliOpts& cliOpts) {
  std::filesystem::create_directories(cliOpts.outputPath);

  size_t inputSize;
  void* inputData = readBinary(cliOpts.inputFile, inputSize);
  FileFormat inputFormat = detectFileFormat(cliOpts.inputFile.filename().string(), inputData, inputSize);
  switch (inputFormat)
  {
  case FMT_BRSAR: {
    SoundArchive soundArchive(inputData, inputSize);
    rsndExtractRsar(soundArchive, cliOpts);
    break;

  } case FMT_BRWAR: {
    SoundWaveArchive waveArchive(inputData, inputSize);
    rsndExtractRwar(waveArchive, cliOpts);
    break;

  } default:
    std::cerr << cliOpts.inputFile << " file format extraction not supported\n";
    exit(-1);
  }

  free(inputData);
}
}