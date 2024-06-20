
#pragma once

#include <cstdlib>
#include <vector>
#include <utility>
#include <cstring>
#include <iostream>

#include "rsnd/SoundBank.hpp"
#include "rsnd/SoundWave.hpp"

struct WaveAudio {
  int sampleRate;
  int loop;
  int loopStart;
  int loopEnd;

  void* data;
  int dataLength;

  WaveAudio() : data(nullptr), dataLength(0) {}
  WaveAudio(const WaveAudio& other) {
    if (other.data) {
      data = malloc(other.dataLength);
      memcpy(data, other.data, other.dataLength);
    }
    sampleRate = other.sampleRate;
    loop = other.loop;
    loopStart = other.loopStart;
    loopEnd = other.loopEnd;
    dataLength = other.dataLength;
  }
  WaveAudio(WaveAudio&& other) noexcept {
    data = nullptr;
    std::swap(data, other.data);
    sampleRate = other.sampleRate;
    loop = other.loop;
    loopStart = other.loopStart;
    loopEnd = other.loopEnd;
    dataLength = other.dataLength;
  }
  WaveAudio& operator=(WaveAudio& other) noexcept {
    if (other.data) {
      data = malloc(other.dataLength);
      memcpy(data, other.data, other.dataLength);
    }
    sampleRate = other.sampleRate;
    loop = other.loop;
    loopStart = other.loopStart;
    loopEnd = other.loopEnd;
    dataLength = other.dataLength;
    return *this;
  }
  WaveAudio& operator=(WaveAudio&& other) noexcept {
    if (data) {
      free(data);
      data = nullptr;
    }
    std::swap(data, other.data);
    data = other.data;
    sampleRate = other.sampleRate;
    loop = other.loop;
    loopStart = other.loopStart;
    loopEnd = other.loopEnd;
    dataLength = other.dataLength;
    return *this;
  }
  ~WaveAudio() { if (data) { free(data); data = nullptr; } }
};

std::vector<WaveAudio> toWaveCollection(const rsnd::SoundBank *bankfile, void* waveData);
WaveAudio toWaveAudio(const rsnd::SoundWave *waveFile);