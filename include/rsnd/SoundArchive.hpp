
#pragma once

#include <cstddef>

#include "common/types.h"
#include "common/util.h"

namespace rsnd {
// ==== RSAR ====
struct SoundArchiveHeader : public BinaryFileHeader {
  u32 symbBlockOffset;
  u32 symbBlockSize;
  u32 infoBlockOffset;
  u32 infoBlockSize;
  u32 fileBlockOffset;
  u32 fileBlockSize;

  void bswap();
};

// ==== SYMB ====
struct SymbHeader : public BinaryBlockHeader {
  u32 nameTableOffset;
  u32 soundTreeOffset;
  u32 playerTreeOffset;
  u32 groupTreeOffset;
  u32 bankTreeOffset;

  void bswap();
};

struct StringTreeNode {
  static const u16 FLAG_LEAF = ( 1 << 0 );

  u16 flags;
  u16 bit;
  u32 leftIdx;
  u32 rightIdx;
  s32 strIdx;
  s32 id;

  void bswap();
};

typedef Array<u32> StringTable;

struct StringTree {
  u32 rootIdx;
  Array<StringTreeNode> nodes;

  void bswap();
};

// ==== INFO ====
struct SoundArchiveInfo : public BinaryBlockHeader {
  DataRef soundTable;
  DataRef bankTable;
  DataRef playerTable;
  DataRef fileTable;
  DataRef groupTable;
  DataRef soundCountTable;

  void bswap();
};

// references to SoundInfoEntry
typedef Array<DataRef> SoundTable;

struct SoundInfoEntry {
  static const u8 TYPE_SEQ = 1;
  static const u8 TYPE_STRM = 2;
  static const u8 TYPE_WAVE = 3;

  u32 fileNameIdx;
  u32 fileIdx;
  u32 playerId;
  DataRef sound3dParam;
  u8 volume;
  u8 playerPriority;
  u8 soundType;
  u8 remoteFilter;
  DataRef extendedInfoRef;
  u32 _20;
  u32 _24;
  u8 panMode;
  u8 panCurve;
  u8 actorPlayerId;
  u8 _2a;

  void bswap();
};

struct SeqSoundInfo {
  u32 offset;
  u32 bankIdx;
  u32 _8;
  u8 _c;
  u8 _d;
  u8 _e[2];
  u32 _10;

  void bswap();
};

struct WsdSoundInfo {
  u32 idx;
  u32 _4;
  u8 _8;
  u8 _9;
  u8 _a[2];
  u32 _c;

  void bswap();
};

struct StrmSoundInfo {
  u32 startPos;
  u16 _4;
  u16 _6;
  u32 _8;

  void bswap();
};

// Refs to BankInfo
typedef Array<DataRef> BankTable;

struct BankInfo {
  u32 fileNameIdx;
  u32 fileIdx;
  u32 _c;

  void bswap();
};

// Refs to BankInfo
typedef Array<DataRef> PlayerTable;

struct PlayerInfo {
  u32 fileNameIdx;
  u8 soundCount;
  u8 _5[3];
  u32 _8;

  void bswap();
};

// Refs to FileInfo
typedef Array<DataRef> FileTable;

struct FileInfo {
  u32 fileSize;
  u32 waveDataSize;
  s32 _8;
  DataRef externalFileName;
  DataRef fileGroupInfo;

  void bswap();
};

// array of groups the file belngs to. DataRef to FileGroup
typedef Array<DataRef> FileGroupInfo;

struct FileGroup {
  // index of group
  u32 groupIdx;
  // index of file in group
  u32 idx;

  void bswap();
};

// Refs to GroupInfo
typedef Array<DataRef> GroupTable;

struct GroupInfo {
  // -1 indicates anonymous group
  s32 nameIdx;
  u32 entryNum;
  // null if embedded in archive
  DataRef externalFileName;
  u32 fileOffset;
  u32 fileSize;
  u32 waveDataOffset;
  u32 waveDataSize;
  DataRef groupItemTable;

  void bswap();
};

// Refs to GroupItemInfo
typedef Array<DataRef> GroupItemTable;

struct GroupItemInfo {
  u32 fileIdx;
  u32 fileOffset;
  u32 fileSize;
  u32 waveDataOffset;
  u32 waveDataSize;
  u32 _14;

  void bswap();
};

struct SoundCountTable {
  u16 seqSoundCount;
  u16 seqTrackCount;
  u16 strmSoundCount;
  u16 strmTrackCount;
  u16 strmChannelCount;
  u16 waveSoundCount;
  u16 waveTrackCount;
  u16 _e;
  u32 _10;

  void bswap();
};

// ==== FILE ====
struct SoundArchiveFile : BinaryBlockHeader {
};

class SoundArchive {
private:
  void* data;
  size_t dataSize;

public:
  // sections
  SymbHeader* soundArchiveSymb;
  SoundArchiveInfo* soundArchiveInfo;
  SoundArchiveFile* soundArchiveFile;

  void* symbBase;
  void* infoBase;
  void* fileBase;

  // SYMB
  StringTable* stringTable;
  StringTree* soundStringTree;
  StringTree* playerStringTree;
  StringTree* groupStringTree;
  StringTree* bankStringTree;

  // INFO
  SoundTable* soundTable;
  BankTable* bankTable;
  PlayerTable* playerTable;
  FileTable* fileTable;
  GroupTable* groupTable;
  SoundCountTable* soundCountTable;

  SoundArchive(void* fileData, size_t fileSize);

  const char* getString(s32 idx) const { return idx > 0 ? static_cast<const char*>(getOffset(symbBase, stringTable->elems[idx])) : nullptr; }
  const SoundInfoEntry* getSoundInfo(u32 idx) const { return static_cast<SoundInfoEntry*>(soundTable->elems[idx].getAddr(infoBase)); }

  const FileInfo* getFileInfo(u32 idx) const { return static_cast<FileInfo*>(fileTable->elems[idx].getAddr(infoBase)); }
  const FileGroupInfo* getFileGroupInfo(u32 idx) const { return static_cast<FileGroupInfo*>(getFileInfo(idx)->fileGroupInfo.getAddr(infoBase)); }
  const FileGroup* getFileGroup(u32 fileIdx, u32 fileGroupIdx) const;
  const char* getFileExternalPath(u32 idx) const { 
    return static_cast<char*>(static_cast<FileInfo*>(fileTable->elems[idx].getAddr(infoBase))->externalFileName.getAddr(infoBase));
  }
  bool isFileExternal(u32 fileIdx) const { return getFileExternalPath(fileIdx) != nullptr; }
  void* getInternalFileData(u32 fileIdx) const;
  void* getInternalFileData(const GroupInfo* groupInfo, const GroupItemInfo* groupItemInfo, size_t* fileSize=nullptr) const;
  void* getInternalWaveData(u32 fileIdx) const;
  void* getInternalWaveData(const GroupInfo* groupInfo, const GroupItemInfo* groupItemInfo, size_t* fileSize=nullptr) const;

  const GroupInfo* getGroupInfo(u32 idx) const { return static_cast<GroupInfo*>(groupTable->elems[idx].getAddr(infoBase)); }
  int getGroupSize(const GroupInfo* groupInfo) const { return groupInfo->groupItemTable.getAddr<GroupItemTable>(infoBase)->size; }
  const GroupItemInfo* getGroupItemInfo(u32 groupIdx, u32 fileIdx) const {
    return static_cast<GroupItemInfo*>(static_cast<GroupItemTable*>(getGroupInfo(groupIdx)->groupItemTable.getAddr(infoBase))->elems[fileIdx].getAddr(infoBase));
  }
  const char* getGroupExternalPath(u32 groupIdx) const { 
    return static_cast<char*>(getGroupInfo(groupIdx)->externalFileName.getAddr(infoBase));
  }
  bool isGroupExternal(u32 groupIdx) const { return getGroupExternalPath(groupIdx) != nullptr; }

  const BankInfo* getBankInfo(u32 idx) const { return static_cast<BankInfo*>(bankTable->elems[idx].getAddr(infoBase)); }
  
  const SeqSoundInfo* getSeqSoundInfo(const SoundInfoEntry* soundInfo) const { return soundInfo->extendedInfoRef.getAddr<SeqSoundInfo>(infoBase); }
  const WsdSoundInfo* getWsdSoundInfo(const SoundInfoEntry* soundInfo) const { return soundInfo->extendedInfoRef.getAddr<WsdSoundInfo>(infoBase); }
  const StrmSoundInfo* getStrmSoundInfo(const SoundInfoEntry* soundInfo) const { return soundInfo->extendedInfoRef.getAddr<StrmSoundInfo>(infoBase); }
};
}
