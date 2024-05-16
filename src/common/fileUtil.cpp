
#include <iostream>
#include <bit>

#include "common/fileUtil.hpp"
#include "common/util.h"

namespace rsnd {
void* readBinary(const std::filesystem::path& filepath, size_t& size) {
  std::ifstream file(filepath, std::ios::binary | std::ios::ate);
  if (!file.is_open()) {
      std::cerr << "Failed to open file " << filepath << std::endl;
      exit(-1);
  }

  size = file.tellg(); // Get the size of the file
  file.seekg(0, std::ios::beg); // Reset file pointer to beginning

  void* fileData = malloc(size);
  if (!fileData) {
      std::cerr << "Failed to allocate memory for file " << filepath << std::endl;
      exit(-1);
  }

  file.read(static_cast<char*>(fileData), size); // Read file into buffer
  file.close();

  return fileData;
}

void writeBinary(const std::filesystem::path& filepath, void* data, size_t size) {
  std::ofstream outFile(filepath, std::ios::out | std::ios::binary);
  if (!outFile) {
    std::cerr << "Error opening file " << filepath << " for writing!" << std::endl;
    exit(-1);
  }

  outFile.write(reinterpret_cast<const char*>(data), size);
  outFile.close();
}

void createWaveFile(const std::filesystem::path& filepath, void* pcmData, int numSamples, int sampleRate, int numChannels) {
  std::ofstream wavFile(filepath, std::ios::binary);
  if (!wavFile.is_open()) {
    std::cerr << "Failed to create WAV file: " << filepath << std::endl;
    return;
  }

  // Write WAV header
  const int bitsPerSample = 8 * sizeof(s16);
  const int byteRate = sampleRate * numChannels * sizeof(s16);
  const int blockAlign = numChannels * sizeof(s16);
  const int dataSectionSize = numSamples * numChannels * sizeof(s16);
  const int fileSize = dataSectionSize + 36;

  wavFile.write("RIFF", 4);                                  // RIFF header
  wavFile.write(reinterpret_cast<const char*>(&fileSize), 4);  // File size
  wavFile.write("WAVE", 4);                                  // WAVE header
  wavFile.write("fmt ", 4);                                  // fmt subchunk header
  wavFile.write(reinterpret_cast<const char*>("\x10\x00\x00\x00"), 4);  // Subchunk size (16 for PCM)
  wavFile.write(reinterpret_cast<const char*>("\x01\x00"), 2);  // Audio format (PCM)
  wavFile.write(reinterpret_cast<const char*>(&numChannels), 2);  // Number of channels
  wavFile.write(reinterpret_cast<const char*>(&sampleRate), 4);   // Sample rate
  wavFile.write(reinterpret_cast<const char*>(&byteRate), 4);      // Byte rate
  wavFile.write(reinterpret_cast<const char*>(&blockAlign), 2);    // Block align
  wavFile.write(reinterpret_cast<const char*>(&bitsPerSample), 2); // Bits per sample
  wavFile.write("data", 4);                                  // Data subchunk header
  wavFile.write(reinterpret_cast<const char*>(&dataSectionSize), 4);  // Data size

  // WAV data
  wavFile.write(reinterpret_cast<const char*>(pcmData), dataSectionSize);
}
}