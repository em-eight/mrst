
#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <cstring>
#include <unordered_set>
#include <random>

#include "rsnd/SoundWaveArchive.hpp"
#include "rsnd/SoundArchive.hpp"
#include "rsnd/SoundWave.hpp"
#include "common/util.h"
#include "common/fileUtil.hpp"
#include "common/cli.h"
#include "tools/extract.hpp"
#include "tools/decode.hpp"
#include "tools/list.hpp"

void printUsage() {
  std::cout << "Usage: mrst [SUBCOMMAND] (opts) inputFile\n";
}

void printUsageExit() {
  printUsage();
  exit(-1);
}

CliOpts parseArgs(int argc, char** argv) {
  if (argc < 3) {
    printUsageExit();
  }

  CliOpts cliOpts;
  /// default values
  cliOpts.subcommand = "";
  cliOpts.outputPath = "";
  cliOpts.extractOpts.decode = false;
  cliOpts.extractOpts.rsarExtractOpts.extractRwars = false;
  cliOpts.extractOpts.rsarExtractOpts.extractStyle = EXTRACT_GROUPS;
  cliOpts.listOpts.groups = false;
  cliOpts.listOpts.sounds = false;
  cliOpts.listOpts.banks = false;
  /// default values
    
  cliOpts.subcommand = argv[1];
  for (int i = 2; i < argc; i++) {
    if (strcmp(argv[i], "--decode") == 0) {
      cliOpts.extractOpts.decode = true;
    } else if (strcmp(argv[i], "--out") == 0 || strcmp(argv[i], "-o") == 0) {
      if (i == argc - 1) printUsageExit();
      cliOpts.outputPath = argv[++i];
    } else if (strcmp(argv[i], "--extract-rwar") == 0) {
      cliOpts.extractOpts.rsarExtractOpts.extractRwars = true;
    } else if (strcmp(argv[i], "--style") == 0) {
      std::string extractStyle = argv[++i];
      if (extractStyle == "groups") {
        cliOpts.extractOpts.rsarExtractOpts.extractStyle = EXTRACT_GROUPS;
      } else if (extractStyle == "sounds") {
        cliOpts.extractOpts.rsarExtractOpts.extractStyle = EXTRACT_SOUNDS;
      } else {
        std::cout << "Unknown extraction style " << extractStyle << '\n';
      }
    } else if (strcmp(argv[i], "--groups") == 0) {
      cliOpts.listOpts.groups = true;
    } else if (strcmp(argv[i], "--banks") == 0) {
      cliOpts.listOpts.banks = true;
    } else {
      cliOpts.inputFile = argv[i];
    }
  }

  // opt dependent default values
  if (cliOpts.outputPath.empty()) {
    if (cliOpts.subcommand == "extract") {
      cliOpts.outputPath = cliOpts.inputFile.string() + ".d";
      std::filesystem::create_directories(cliOpts.outputPath);
    }
    // for decode default output is same filename with different extension
  }

  // check mandatory fields
  if (cliOpts.subcommand.empty() || cliOpts.inputFile.empty()) {
    printUsageExit();
  }
  
  return cliOpts;
}

void printBytesHex(void* ptr, int N) {
    std::cout << std::hex << std::setfill('0') << " ";
    for (int i = 0; i < N; ++i) {
        std::cout << std::setw(2) << static_cast<int>(static_cast<unsigned char>(((char*)ptr)[i])) << " ";
    }
}

using namespace rsnd;
namespace fs = std::filesystem;

int main(int argc, char** argv) {
  CliOpts cliOpts = parseArgs(argc, argv);

  if (cliOpts.subcommand == "extract") {
    rsndExtract(cliOpts);
  } else if (cliOpts.subcommand == "decode") {
    rsndDecode(cliOpts);
  } else if (cliOpts.subcommand == "list") {
    rsndList(cliOpts);
  } else {
    std::cerr << "Unknown subcommand " << cliOpts.subcommand << '\n';
    printUsageExit();
  }
}
