
#pragma once

#include <filesystem>
#include <string>

enum ExtractionStyle {
  EXTRACT_GROUPS,
  EXTRACT_SOUNDS,
};

struct RsarExtractOpts {
  ExtractionStyle extractStyle;
  bool extractRwars;
};

struct ExtractOpts {
  // extract as is or decode to popular format
  bool decode;
  RsarExtractOpts rsarExtractOpts;
};

struct ListOpts {
  bool sounds;
  bool groups;
  bool banks;
};

struct CliOpts {
  std::filesystem::path inputFile;
  std::string subcommand;
  std::filesystem::path outputPath;
  // specific to the extract subcommand
  ExtractOpts extractOpts;
  // specific to the list subcommand
  ListOpts listOpts;
};