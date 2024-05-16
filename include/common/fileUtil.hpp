
#pragma once

#include <string>
#include <filesystem>
#include <fstream>

#include "types.h"

namespace rsnd {
void* readBinary(const std::filesystem::path& path, size_t& size);
void writeBinary(const std::filesystem::path& path, void* data, size_t size);

void createWaveFile(const std::filesystem::path& filepath, void* pcm, int numSamples, int sampleRate, int numChannels);
}