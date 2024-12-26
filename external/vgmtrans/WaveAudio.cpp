
#include "WaveAudio.h"

#include "rsnd/soundCommon.hpp"

using namespace rsnd;

std::vector<WaveAudio> toWaveCollection(const rsnd::SoundBank *bankfile, void* waveData) {
  std::vector<WaveAudio> waveAudios;

  for (int i = 0; i < bankfile->bankWave->waveInfos.size; i++) {
    const WaveInfo* waveInfo = bankfile->getWaveInfo(i);
    u32 channelCount = waveInfo->channelCount;
      
    u32 sampleBufferSize = channelCount * waveInfo->loopEnd * sizeof(s16);
    s16* pcmBuffer = static_cast<s16*>(malloc(sampleBufferSize));

    for (int j = 0; j < waveInfo->channelCount; j++) {
      const SoundWaveChannelInfo* chInfo = bankfile->getChannelInfo(waveInfo, j);
      const AdpcParams* adpcParams = bankfile->getAdpcParams(waveInfo, chInfo);

      const u8* blockData = (const u8*)waveData + waveInfo->dataLoc + chInfo->dataOffset;

      decodeBlock(blockData, waveInfo->loopEnd, pcmBuffer + j, channelCount, waveInfo->format, adpcParams);
    }

    waveAudios.emplace_back();
    WaveAudio& newWave = waveAudios[waveAudios.size() - 1];
    newWave.data = pcmBuffer;
    newWave.dataLength = sampleBufferSize;
    newWave.sampleRate = waveInfo->sampleRate;
    newWave.loop = waveInfo->loop;
    newWave.loopStart = waveInfo->loopStart;
    newWave.loopEnd = waveInfo->loopEnd;
  }

  return waveAudios;
}

WaveAudio toWaveAudio(const rsnd::SoundWave *waveFile) {
  WaveAudio waveAudio;
    
  const WaveInfo* waveInfo = waveFile->info;
    
  void* pcmBuffer = waveFile->getTrackPcm();
  u32 sampleBufferSize = waveFile->getTrackSampleBufferSize();

  waveAudio.data = pcmBuffer;
  waveAudio.dataLength = sampleBufferSize;
  waveAudio.sampleRate = waveInfo->sampleRate;
  waveAudio.loop = waveInfo->loop;
  waveAudio.loopStart = waveInfo->loopStart;
  waveAudio.loopEnd = waveInfo->loopEnd;

  return waveAudio;
}
