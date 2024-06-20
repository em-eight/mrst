
#include "WaveAudio.h"

#include "rsnd/soundCommon.hpp"

using namespace rsnd;

std::vector<WaveAudio> toWaveCollection(const rsnd::SoundBank *bankfile, void* waveData) {
  std::vector<WaveAudio> waveAudios;

  for (int i = 0; i < bankfile->bankWave->waveInfos.size; i++) {
    const WaveInfo* waveInfo = bankfile->getWaveInfo(i);
    u32 channelCount = bankfile->getChannelCount(waveInfo);
      
    u32 loopStart = dspAddressToSamples(waveInfo->loopStart);
    u32 loopEnd = dspAddressToSamples(waveInfo->loopEnd);
    u32 sampleBufferSize = channelCount * loopEnd * sizeof(s16);
    s16* pcmBuffer = static_cast<s16*>(malloc(sampleBufferSize));

    for (int j = 0; j < waveInfo->channelCount; j++) {
      const SoundWaveChannelInfo* chInfo = bankfile->getChannelInfo(waveInfo, j);
      const AdpcParams* adpcParams = bankfile->getAdpcParams(waveInfo, chInfo);

      const u8* blockData = (const u8*)waveData + waveInfo->dataLoc + chInfo->dataOffset;

      decodeBlock(blockData, loopEnd, pcmBuffer + j, channelCount, waveInfo->format, adpcParams);
    }

    waveAudios.emplace_back();
    WaveAudio& newWave = waveAudios[waveAudios.size() - 1];
    newWave.data = pcmBuffer;
    newWave.dataLength = sampleBufferSize;
    newWave.sampleRate = waveInfo->sampleRate;
    newWave.loop = waveInfo->loop;
    newWave.loopStart = loopStart;
    newWave.loopEnd = loopEnd;
  }

  return waveAudios;
}

WaveAudio toWaveAudio(const rsnd::SoundWave *waveFile) {
  WaveAudio waveAudio;
    
  const WaveInfo* waveInfo = waveFile->info;
    
  u32 loopStart = dspAddressToSamples(waveInfo->loopStart);
  u32 loopEnd = dspAddressToSamples(waveInfo->loopEnd);
  void* pcmBuffer = waveFile->getTrackPcm();
  u32 sampleBufferSize = waveFile->getTrackSampleBufferSize();

  waveAudio.data = pcmBuffer;
  waveAudio.dataLength = sampleBufferSize;
  waveAudio.sampleRate = waveInfo->sampleRate;
  waveAudio.loop = waveInfo->loop;
  waveAudio.loopStart = loopStart;
  waveAudio.loopEnd = loopEnd;

  return waveAudio;
}