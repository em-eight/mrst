/*
* VGMTrans (c) 2002-2024
 * Licensed under the zlib license,
 * refer to the included LICENSE.txt file
 */
#include <cmath>
#include <iostream>
#include "version.h"
#include "ScaleConversion.h"
#include "SF2File.h"
//#include "Root.h"

#include "rsnd/SoundBank.hpp"
#include "rsnd/SoundWaveArchive.hpp"
#include "rsnd/SoundWave.hpp"
#include "common/fileUtil.hpp"

struct EnvelopeParams {
  double attack_time;
  double decay_time;
  double sustain_level;
  double release_time;
  double hold_time;
};

static float GetFallingRate(uint8_t DecayTime) {
  if (DecayTime == 0x7F)
    return 65535.0f;
  else if (DecayTime == 0x7E)
    return 120 / 5.0f;
  else if (DecayTime < 0x32)
    return ((DecayTime << 1) + 1) / 128.0f / 5.0f;
  else
    return ((60.0f) / (126 - DecayTime)) / 5.0f;
}

static EnvelopeParams envelopeFromInfo(const rsnd::InstrInfo* info) {
  EnvelopeParams envelope;

  static const float attackTable[128] = {
    0.9992175f, 0.9984326f, 0.9976452f, 0.9968553f,
    0.9960629f, 0.9952679f, 0.9944704f, 0.9936704f,
    0.9928677f, 0.9920625f, 0.9912546f, 0.9904441f,
    0.9896309f, 0.9888151f, 0.9879965f, 0.9871752f,
    0.9863512f, 0.9855244f, 0.9846949f, 0.9838625f,
    0.9830273f, 0.9821893f, 0.9813483f, 0.9805045f,
    0.9796578f, 0.9788081f, 0.9779555f, 0.9770999f,
    0.9762413f, 0.9753797f, 0.9745150f, 0.9736472f,
    0.9727763f, 0.9719023f, 0.9710251f, 0.9701448f,
    0.9692612f, 0.9683744f, 0.9674844f, 0.9665910f,
    0.9656944f, 0.9647944f, 0.9638910f, 0.9629842f,
    0.9620740f, 0.9611604f, 0.9602433f, 0.9593226f,
    0.9583984f, 0.9574706f, 0.9565392f, 0.9556042f,
    0.9546655f, 0.9537231f, 0.9527769f, 0.9518270f,
    0.9508732f, 0.9499157f, 0.9489542f, 0.9479888f,
    0.9470195f, 0.9460462f, 0.9450689f, 0.9440875f,
    0.9431020f, 0.9421124f, 0.9411186f, 0.9401206f,
    0.9391184f, 0.9381118f, 0.9371009f, 0.9360856f,
    0.9350659f, 0.9340417f, 0.9330131f, 0.9319798f,
    0.9309420f, 0.9298995f, 0.9288523f, 0.9278004f,
    0.9267436f, 0.9256821f, 0.9246156f, 0.9235442f,
    0.9224678f, 0.9213864f, 0.9202998f, 0.9192081f,
    0.9181112f, 0.9170091f, 0.9159016f, 0.9147887f,
    0.9136703f, 0.9125465f, 0.9114171f, 0.9102821f,
    0.9091414f, 0.9079949f, 0.9068427f, 0.9056845f,
    0.9045204f, 0.9033502f, 0.9021740f, 0.9009916f,
    0.8998029f, 0.8986080f, 0.8974066f, 0.8961988f,
    0.8949844f, 0.8900599f, 0.8824622f, 0.8759247f,
    0.8691861f, 0.8636406f, 0.8535788f, 0.8430189f,
    0.8286135f, 0.8149099f, 0.8002172f, 0.7780663f,
    0.7554750f, 0.7242125f, 0.6828239f, 0.6329169f,
    0.5592135f, 0.4551411f, 0.3298770f, 0.0000000f
  };
  
  /* Figure out how many msecs it takes to go from the initial decimal to our threshold. */
  float realAttack = attackTable[info->attack];
  int msecs = 0;

  const float VOLUME_DB_MIN = -90.4f;

  float vol = VOLUME_DB_MIN * 10.0f;
  while (vol <= -1.0f / 32.0f) {
    vol *= realAttack;
    msecs++;
  }
  envelope.attack_time = msecs / 1000.0;

  /* Scale is dB*10, so the first number is actually -72.3dB.
   * Minimum possible volume is -90.4dB. */
  const int16_t sustainTable[] = {
    -723, -722, -721, -651, -601, -562, -530, -503,
    -480, -460, -442, -425, -410, -396, -383, -371,
    -360, -349, -339, -330, -321, -313, -305, -297,
    -289, -282, -276, -269, -263, -257, -251, -245,
    -239, -234, -229, -224, -219, -214, -210, -205,
    -201, -196, -192, -188, -184, -180, -176, -173,
    -169, -165, -162, -158, -155, -152, -149, -145,
    -142, -139, -136, -133, -130, -127, -125, -122,
    -119, -116, -114, -111, -109, -106, -103, -101,
    -99,  -96,  -94,  -91,  -89,  -87,  -85,  -82,
    -80,  -78,  -76,  -74,  -72,  -70,  -68,  -66,
    -64,  -62,  -60,  -58,  -56,  -54,  -52,  -50,
    -49,  -47,  -45,  -43,  -42,  -40,  -38,  -36,
    -35,  -33,  -31,  -30,  -28,  -27,  -25,  -23,
    -22,  -20,  -19,  -17,  -16,  -14,  -13,  -11,
    -10,  -8,   -7,   -6,   -4,   -3,   -1,    0
  };

  float decayRate = GetFallingRate(info->decay);
  int16_t sustainLev = sustainTable[info->sustain];

  /* Decay time is the time it takes to get to the sustain level from max vol,
   * decaying by decayRate every 1ms. */
  envelope.decay_time = ((0.0f - sustainLev) / decayRate) / 1000.0;

  envelope.sustain_level = 1.0 - (sustainLev / VOLUME_DB_MIN);

  float releaseRate = GetFallingRate(info->release);

  /* Release time is the time it takes to get from sustain level to minimum volume. */
  envelope.release_time = ((sustainLev - VOLUME_DB_MIN) / releaseRate) / 1000.0;

  envelope.hold_time = (info->hold + 1) * (info->hold + 1) / 4000.0;

  return envelope;
}

SF2InfoListChunk::SF2InfoListChunk(const std::string& name)
    : LISTChunk("INFO") {
  // Create a date string
  time_t current_time = time(nullptr);
  char *c_time_string = ctime(&current_time);

  // Add the child info chunks
  Chunk *ifilCk = new Chunk("ifil");
  sfVersionTag versionTag;        //soundfont version 2.01
  versionTag.wMajor = 2;
  versionTag.wMinor = 1;
  ifilCk->SetData(&versionTag, sizeof(versionTag));
  AddChildChunk(ifilCk);
  AddChildChunk(new SF2StringChunk("isng", "EMU8000"));
  AddChildChunk(new SF2StringChunk("INAM", name));
  AddChildChunk(new SF2StringChunk("ICRD", std::string(c_time_string)));
  AddChildChunk(new SF2StringChunk("ISFT", std::string("VGMTrans " + std::string(VGMTRANS_VERSION))));
}


//  *******
//  SF2File
//  *******

SF2File::SF2File(const rsnd::SoundBank *bankfile, const std::vector<WaveAudio>& waves)
    : RiffFile("RSND bank", "sfbk") {

  //***********
  // INFO chunk
  //***********
  AddChildChunk(new SF2InfoListChunk(name));

  // sdta chunk and its child smpl chunk containing all samples
  LISTChunk *sdtaCk = new LISTChunk("sdta");
  Chunk *smplCk = new Chunk("smpl");

  // Concatanate all of the samples together and add the result to the smpl chunk data
  size_t numWaves = waves.size();
  smplCk->size = 0;
  for (size_t i = 0; i < numWaves; i++) {
    const WaveAudio& wav = waves[i];
    smplCk->size += wav.dataLength + (46 * 2);    // plus the 46 padding samples required by sf2 spec
  }
  smplCk->data = new uint8_t[smplCk->size];
  uint32_t bufPtr = 0;
  for (size_t i = 0; i < numWaves; i++) {
    const WaveAudio& wav = waves[i];
    size_t waveSz = wav.dataLength;

    void* pcm = wav.data;
    memcpy(smplCk->data + bufPtr, pcm, waveSz);
    memset(smplCk->data + bufPtr + waveSz, 0, 46 * 2);
    bufPtr += waveSz + (46 * 2);        // plus the 46 padding samples required by sf2 spec
  }

  sdtaCk->AddChildChunk(smplCk);
  this->AddChildChunk(sdtaCk);

  //***********
  // pdta chunk
  //***********

  LISTChunk *pdtaCk = new LISTChunk("pdta");

  //***********
  // phdr chunk
  //***********
  Chunk *phdrCk = new Chunk("phdr");
  size_t numInstrs = bankfile->getInstrCount();
  phdrCk->size = static_cast<uint32_t>((numInstrs + 1) * sizeof(sfPresetHeader));
  phdrCk->data = new uint8_t[phdrCk->size];

  for (size_t i = 0; i < numInstrs; i++) {
    std::string instrName = "instr" + std::to_string(i);
    int wPreset = i;
    // I don't think these are known for RBNK files, so set to zero
    int wBank = 0;

    sfPresetHeader presetHdr{};
    memcpy(presetHdr.achPresetName, instrName.c_str(), std::min(instrName.length(), static_cast<size_t>(20)));
    presetHdr.wPreset = static_cast<uint16_t>(wPreset);

    // Despite being a 16-bit value, SF2 only supports banks up to 127. Since
    // it's pretty common to have either MSB or LSB be 0, we'll use whatever
    // one is not zero, with preference for MSB.
    //uint16_t bank16 = static_cast<uint16_t>(instr->ulBank);

    if ((wBank & 0xFF00) == 0) {
      presetHdr.wBank = wBank & 0x7F;
    }
    else {
      presetHdr.wBank = (wBank >> 8) & 0x7F;
    }
    presetHdr.wPresetBagNdx = static_cast<uint16_t>(i);
    presetHdr.dwLibrary = 0;
    presetHdr.dwGenre = 0;
    presetHdr.dwMorphology = 0;

    memcpy(phdrCk->data + (i * sizeof(sfPresetHeader)), &presetHdr, sizeof(sfPresetHeader));
  }
  //  add terminal sfPresetBag
  sfPresetHeader presetHdr{};
  presetHdr.wPresetBagNdx = static_cast<uint16_t>(numInstrs);
  memcpy(phdrCk->data + (numInstrs * sizeof(sfPresetHeader)), &presetHdr, sizeof(sfPresetHeader));
  pdtaCk->AddChildChunk(phdrCk);

  //***********
  // pbag chunk
  //***********
  Chunk *pbagCk = new Chunk("pbag");
  constexpr size_t ITEMS_IN_PGEN = 2;
  pbagCk->size = static_cast<uint32_t>((numInstrs + 1) * sizeof(sfPresetBag));
  pbagCk->data = new uint8_t[pbagCk->size];
  for (size_t i = 0; i < numInstrs; i++) {
    sfPresetBag presetBag{};
    presetBag.wGenNdx = static_cast<uint16_t>(i * ITEMS_IN_PGEN);
    presetBag.wModNdx = 0;

    memcpy(pbagCk->data + (i * sizeof(sfPresetBag)), &presetBag, sizeof(sfPresetBag));
  }
  //  add terminal sfPresetBag
  sfPresetBag presetBag{};
  presetBag.wGenNdx = static_cast<uint16_t>(numInstrs * ITEMS_IN_PGEN);
  memcpy(pbagCk->data + (numInstrs * sizeof(sfPresetBag)), &presetBag, sizeof(sfPresetBag));
  pdtaCk->AddChildChunk(pbagCk);

  //***********
  // pmod chunk
  //***********
  Chunk *pmodCk = new Chunk("pmod");
  //  create the terminal field
  sfModList modList{};
  pmodCk->SetData(&modList, sizeof(sfModList));
  //modList.sfModSrcOper = cc1_Mod;
  //modList.sfModDestOper = startAddrsOffset;
  //modList.modAmount = 0;
  //modList.sfModAmtSrcOper = cc1_Mod;
  //modList.sfModTransOper = linear;
  pdtaCk->AddChildChunk(pmodCk);

  //***********
  // pgen chunk
  //***********
  Chunk *pgenCk = new Chunk("pgen");
  //pgenCk->size = (synthfile->vInstrs.size()+1) * sizeof(sfGenList);
  pgenCk->size = static_cast<uint32_t>((numInstrs * sizeof(sfGenList) * ITEMS_IN_PGEN) + sizeof(sfGenList));
  pgenCk->data = new uint8_t[pgenCk->size];
  uint32_t dataPtr = 0;
  for (size_t i = 0; i < numInstrs; i++) {
    // VGMTrans supports reverb, not used by BRBNK. Perhaps these entries can be omitted
    int instrReverb = 0;

    sfGenList genList{};

    // reverbEffectsSend - Value is in 0.1% units, so multiply by 1000. Ex: 250 == 25%.
    genList.sfGenOper = reverbEffectsSend;
    genList.genAmount.shAmount = instrReverb * 1000;
    memcpy(pgenCk->data + dataPtr, &genList, sizeof(sfGenList));
    dataPtr += sizeof(sfGenList);

    genList.sfGenOper = instrument;
    genList.genAmount.wAmount = static_cast<uint16_t>(i);
    memcpy(pgenCk->data + dataPtr, &genList, sizeof(sfGenList));
    dataPtr += sizeof(sfGenList);
  }
  //  add terminal sfGenList
  sfGenList genList{};
  memcpy(pgenCk->data + dataPtr, &genList, sizeof(sfGenList));

  pdtaCk->AddChildChunk(pgenCk);

  //***********
  // inst chunk
  //***********
  Chunk *instCk = new Chunk("inst");
  instCk->size = static_cast<uint32_t>((numInstrs + 1) * sizeof(sfInst));
  instCk->data = new uint8_t[instCk->size];
  uint16_t rgnCounter = 0;
  for (size_t i = 0; i < numInstrs; i++) {
    std::string instrName = "instr" + std::to_string(i);
    auto regions = bankfile->getInstrRegions(i);

    sfInst inst{};
    memcpy(inst.achInstName, instrName.c_str(), std::min(instrName.length(), static_cast<size_t>(20)));
    inst.wInstBagNdx = rgnCounter;
    rgnCounter += regions.size();

    memcpy(instCk->data + (i * sizeof(sfInst)), &inst, sizeof(sfInst));
  }
  //  add terminal sfInst
  uint32_t numTotalRgns = rgnCounter;
  sfInst inst{};
  inst.wInstBagNdx = numTotalRgns;
  memcpy(instCk->data + (numInstrs * sizeof(sfInst)), &inst, sizeof(sfInst));
  pdtaCk->AddChildChunk(instCk);

  //***********
  // ibag chunk - stores all zones (regions) for instruments
  //***********
  Chunk *ibagCk = new Chunk("ibag");


  ibagCk->size = (numTotalRgns + 1) * sizeof(sfInstBag);
  ibagCk->data = new uint8_t[ibagCk->size];

  rgnCounter = 0;
  int instGenCounter = 0;
  for (size_t i = 0; i < numInstrs; i++) {
    auto regions = bankfile->getInstrRegions(i);

    size_t numRgns = regions.size();
    for (size_t j = 0; j < numRgns; j++) {
      sfInstBag instBag{};
      instBag.wInstGenNdx = instGenCounter;
      instGenCounter += 12;
      instBag.wInstModNdx = 0;

      memcpy(ibagCk->data + (rgnCounter++ * sizeof(sfInstBag)), &instBag, sizeof(sfInstBag));
    }
  }
  //  add terminal sfInstBag
  sfInstBag instBag{};
  instBag.wInstGenNdx = instGenCounter;
  instBag.wInstModNdx = 0;
  memcpy(ibagCk->data + (rgnCounter * sizeof(sfInstBag)), &instBag, sizeof(sfInstBag));
  pdtaCk->AddChildChunk(ibagCk);

  //***********
  // imod chunk
  //***********
  Chunk *imodCk = new Chunk("imod");
  //  create the terminal field
  memset(&modList, 0, sizeof(sfModList));
  imodCk->SetData(&modList, sizeof(sfModList));
  pdtaCk->AddChildChunk(imodCk);

  //***********
  // igen chunk
  //***********
  Chunk *igenCk = new Chunk("igen");
  igenCk->size = (numTotalRgns * sizeof(sfInstGenList) * 12) + sizeof(sfInstGenList);
  igenCk->data = new uint8_t[igenCk->size];
  dataPtr = 0;
  for (size_t i = 0; i < numInstrs; i++) {
    auto instrRegions = bankfile->getInstrRegions(i);

    size_t numRgns = instrRegions.size();
    for (size_t j = 0; j < numRgns; j++) {
      //SynthRgn *rgn = instr->vRgns[j];
      auto instrRegion = instrRegions[j];

      sfInstGenList instGenList;
      // Key range - (if exists) this must be the first chunk
      instGenList.sfGenOper = keyRange;
      instGenList.genAmount.ranges.byLo = static_cast<uint8_t>(instrRegion.keyLo);
      instGenList.genAmount.ranges.byHi = static_cast<uint8_t>(instrRegion.keyHi);
      memcpy(igenCk->data + dataPtr, &instGenList, sizeof(sfInstGenList));
      dataPtr += sizeof(sfInstGenList);

      if (instrRegion.velHi)    // 0 means 'not set', fixes TriAce instruments
      {
        // Velocity range (if exists) this must be the next chunk
        instGenList.sfGenOper = velRange;
        instGenList.genAmount.ranges.byLo = static_cast<uint8_t>(instrRegion.velLo);
        instGenList.genAmount.ranges.byHi = static_cast<uint8_t>(instrRegion.velHi);
        memcpy(igenCk->data + dataPtr, &instGenList, sizeof(sfInstGenList));
        dataPtr += sizeof(sfInstGenList);
      }

      auto* instrInfo = instrRegion.instrInfo;
      EnvelopeParams envelope = envelopeFromInfo(instrInfo);
      const WaveAudio& wav = waves[instrInfo->waveIdx];

      // initialAttenuation
      instGenList.sfGenOper = initialAttenuation;
      instGenList.genAmount.shAmount = 127 - instrInfo->volume; // TODO: how do I transcribe volume?
      memcpy(igenCk->data + dataPtr, &instGenList, sizeof(sfInstGenList));
      dataPtr += sizeof(sfInstGenList);

      // pan
      instGenList.sfGenOper = pan;
      int16_t panConverted = std::round(1000 * (instrInfo->pan - 64) / 64.0);
      instGenList.genAmount.shAmount = panConverted;
      memcpy(igenCk->data + dataPtr, &instGenList, sizeof(sfInstGenList));
      dataPtr += sizeof(sfInstGenList);

      // sampleModes
      instGenList.sfGenOper = sampleModes;
      instGenList.genAmount.wAmount = wav.loop;
      memcpy(igenCk->data + dataPtr, &instGenList, sizeof(sfInstGenList));
      dataPtr += sizeof(sfInstGenList);

      // overridingRootKey
      instGenList.sfGenOper = overridingRootKey;
      instGenList.genAmount.wAmount = instrInfo->originalKey;
      memcpy(igenCk->data + dataPtr, &instGenList, sizeof(sfInstGenList));
      dataPtr += sizeof(sfInstGenList);

      // attackVolEnv
      instGenList.sfGenOper = attackVolEnv;
      instGenList.genAmount.shAmount =
          (envelope.attack_time == 0) ? -32768 : std::round(SecondsToTimecents(envelope.attack_time));
      memcpy(igenCk->data + dataPtr, &instGenList, sizeof(sfInstGenList));
      dataPtr += sizeof(sfInstGenList);

      // holdVolEnv
      instGenList.sfGenOper = holdVolEnv;
      instGenList.genAmount.shAmount =
          (envelope.hold_time == 0) ? -32768 : std::round(SecondsToTimecents(envelope.hold_time));
      memcpy(igenCk->data + dataPtr, &instGenList, sizeof(sfInstGenList));
      dataPtr += sizeof(sfInstGenList);

      // decayVolEnv
      instGenList.sfGenOper = decayVolEnv;
      instGenList.genAmount.shAmount =
          (envelope.decay_time == 0) ? -32768 : std::round(SecondsToTimecents(envelope.decay_time));
      memcpy(igenCk->data + dataPtr, &instGenList, sizeof(sfInstGenList));
      dataPtr += sizeof(sfInstGenList);

      // sustainVolEnv
      instGenList.sfGenOper = sustainVolEnv;
      if (envelope.sustain_level > 100.0)
        envelope.sustain_level = 100.0;
      instGenList.genAmount.shAmount = static_cast<int16_t>(envelope.sustain_level * 10);
      memcpy(igenCk->data + dataPtr, &instGenList, sizeof(sfInstGenList));
      dataPtr += sizeof(sfInstGenList);

      // releaseVolEnv
      instGenList.sfGenOper = releaseVolEnv;
      instGenList.genAmount.shAmount =
          (envelope.release_time == 0) ? -32768 : std::round(SecondsToTimecents(envelope.release_time));
      memcpy(igenCk->data + dataPtr, &instGenList, sizeof(sfInstGenList));
      dataPtr += sizeof(sfInstGenList);

      // reverbEffectsSend
      //instGenList.sfGenOper = reverbEffectsSend;
      //instGenList.genAmount.shAmount = 800;
      //memcpy(pgenCk->data + dataPtr, &instGenList, sizeof(sfInstGenList));
      //dataPtr += sizeof(sfInstGenList);

      // sampleID - this is the terminal chunk
      instGenList.sfGenOper = sampleID;
      instGenList.genAmount.wAmount = static_cast<uint16_t>(instrInfo->waveIdx);
      memcpy(igenCk->data + dataPtr, &instGenList, sizeof(sfInstGenList));
      dataPtr += sizeof(sfInstGenList);

      //int numConnBlocks = rgn->art->vConnBlocks.size();
      //for (int k = 0; k < numConnBlocks; k++)
      //{
      //	SynthConnectionBlock* connBlock = rgn->art->vConnBlocks[k];
      //	connBlock->
      //}
    }
  }
  //  add terminal sfInstBag
  sfInstGenList instGenList{};
  memcpy(igenCk->data + dataPtr, &instGenList, sizeof(sfInstGenList));
  //memset(ibagCk->data + (numRgns*sizeof(sfInstBag)), 0, sizeof(sfInstBag));
  //igenCk->SetData(&genList, sizeof(sfGenList));
  pdtaCk->AddChildChunk(igenCk);

  //***********
  // shdr chunk
  //***********
  Chunk *shdrCk = new Chunk("shdr");

  size_t numSamps = waves.size();
  shdrCk->size = static_cast<uint32_t>((numSamps + 1) * sizeof(sfSample));
  shdrCk->data = new uint8_t[shdrCk->size];

  uint32_t sampOffset = 0;
  for (size_t i = 0; i < numSamps; i++) {
    const WaveAudio& wav = waves[i];
    std::string waveName = "wav" + std::to_string(i);

    sfSample samp{};
    memcpy(samp.achSampleName, waveName.c_str(), std::min(waveName.length(), static_cast<size_t>(20)));
    samp.dwStart = sampOffset;
    samp.dwEnd = samp.dwStart + (wav.dataLength / sizeof(uint16_t));
    sampOffset = samp.dwEnd + 46;        // plus the 46 padding samples required by sf2 spec

    // Search through all regions for an associated sampInfo structure with this sample
    rsnd::InstrInfo *instrInfo = nullptr;
    for (size_t j = 0; j < numInstrs; j++) {
      auto instrRegions = bankfile->getInstrRegions(j);

      size_t numRgns = instrRegions.size();
      for (size_t k = 0; k < numRgns; k++) {
        auto instrRegion = instrRegions[k];
        if (instrRegion.instrInfo->waveIdx == i) {
          instrInfo = instrRegion.instrInfo;
          break;
        }
      }
      if (instrInfo != nullptr)
        break;
    }
    //  If we didn't find a rgn association, then idk.
    if (instrInfo == nullptr) {
      std::cout << "Warn: No instrument info for wave index " << i << '\n';
      continue;
    }
    assert (instrInfo != NULL);

    samp.dwStartloop = samp.dwStart + wav.loopStart;
    samp.dwEndloop = samp.dwStart + wav.loopEnd;
    samp.dwSampleRate = wav.sampleRate;
    samp.byOriginalKey = static_cast<uint8_t>(instrInfo->originalKey);
    samp.chCorrection = 0;
    samp.wSampleLink = 0;
    samp.sfSampleType = monoSample; // Do stereo samples exist in RBNKs?

    memcpy(shdrCk->data + (i * sizeof(sfSample)), &samp, sizeof(sfSample));
  }

  //  add terminal sfSample
  memset(shdrCk->data + (numSamps * sizeof(sfSample)), 0, sizeof(sfSample));
  pdtaCk->AddChildChunk(shdrCk);

  this->AddChildChunk(pdtaCk);
}

std::vector<uint8_t> SF2File::SaveToMem() {
  std::vector<uint8_t> buf(GetSize());
  Write(buf.data());
  return buf;
}

bool SF2File::SaveSF2File(const std::filesystem::path &filepath) {
  auto buf = SaveToMem();
  rsnd::writeBinary(filepath, buf.data(), buf.size());
  return true;
}
