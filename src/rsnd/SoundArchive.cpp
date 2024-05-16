
#include <bit>
#include <iostream>

#include "rsnd/SoundArchive.hpp"

namespace rsnd {
void SoundArchiveHeader::bswap() {
  this->BinaryFileHeader::bswap();

  symbBlockOffset = std::byteswap(symbBlockOffset);
  symbBlockSize = std::byteswap(symbBlockSize);
  infoBlockOffset = std::byteswap(infoBlockOffset);
  infoBlockSize = std::byteswap(infoBlockSize);
  fileBlockOffset = std::byteswap(fileBlockOffset);
  fileBlockSize = std::byteswap(fileBlockSize);
}

void SymbHeader::bswap() {
  this->BinaryBlockHeader::bswap();

  nameTableOffset = std::byteswap(nameTableOffset);
  soundTreeOffset = std::byteswap(soundTreeOffset);
  playerTreeOffset = std::byteswap(playerTreeOffset);
  groupTreeOffset = std::byteswap(groupTreeOffset);
  bankTreeOffset = std::byteswap(bankTreeOffset);
}

void StringTreeNode::bswap() {
  flags = std::byteswap(flags);
  bit = std::byteswap(bit);
  leftIdx = std::byteswap(leftIdx);
  rightIdx = std::byteswap(rightIdx);
  strIdx = std::byteswap(strIdx);
  id = std::byteswap(id);
}

void StringTree::bswap() {
  rootIdx = std::byteswap(rootIdx);
  nodes.bswap();
}

void SoundArchiveInfo::bswap() {
  this->BinaryBlockHeader::bswap();

  soundTable.bswap();
  bankTable.bswap();
  playerTable.bswap();
  fileTable.bswap();
  groupTable.bswap();
  soundCountTable.bswap();
}

void SoundInfoEntry::bswap() {
  fileNameIdx = std::byteswap(fileNameIdx);
  fileIdx = std::byteswap(fileIdx);
  playerId = std::byteswap(playerId);
  sound3dParam.bswap();
  extendedInfoRef.bswap();
  _20 = std::byteswap(_20);
  _24 = std::byteswap(_24);
}

void BankInfo::bswap() {
  fileNameIdx = std::byteswap(fileNameIdx);
  fileIdx = std::byteswap(fileIdx);
  _c = std::byteswap(_c);
}

void PlayerInfo::bswap() {
  fileNameIdx = std::byteswap(fileNameIdx);
  _8 = std::byteswap(_8);
}

void FileInfo::bswap() {
  fileSize = std::byteswap(fileSize);
  waveDataSize = std::byteswap(waveDataSize);
  _8 = std::byteswap(_8);
  externalFileName.bswap();
  fileGroupInfo.bswap();
}

void FileGroup::bswap() {
  groupIdx = std::byteswap(groupIdx);
  idx = std::byteswap(idx);
}

void GroupInfo::bswap() {
  nameIdx = std::byteswap(nameIdx);
  entryNum = std::byteswap(entryNum);
  externalFileName.bswap();
  fileOffset = std::byteswap(fileOffset);
  fileSize = std::byteswap(fileSize);
  waveDataOffset = std::byteswap(waveDataOffset);
  waveDataSize = std::byteswap(waveDataSize);
  groupItemTable.bswap();
}

void GroupItemInfo::bswap() {
  fileIdx = std::byteswap(fileIdx);
  fileOffset = std::byteswap(fileOffset);
  fileSize = std::byteswap(fileSize);
  waveDataOffset = std::byteswap(waveDataOffset);
  waveDataSize = std::byteswap(waveDataSize);
  _14 = std::byteswap(_14);
}

void SoundCountTable::bswap() {
  seqSoundCount = std::byteswap(seqSoundCount);
  seqTrackCount = std::byteswap(seqTrackCount);
  strmSoundCount = std::byteswap(strmSoundCount);
  strmTrackCount = std::byteswap(strmTrackCount);
  strmChannelCount = std::byteswap(strmChannelCount);
  waveSoundCount = std::byteswap(waveSoundCount);
  waveTrackCount = std::byteswap(waveTrackCount);
}

SoundArchive::SoundArchive(void* fileData, size_t fileSize) {
  dataSize = fileSize;
  data = fileData;

  SoundArchiveHeader* sarHdr = static_cast<SoundArchiveHeader*>(fileData);
  sarHdr->bswap();

  // ===== SYMB
  soundArchiveSymb = static_cast<SymbHeader*>(getOffset(fileData, sarHdr->symbBlockOffset));
  soundArchiveSymb->bswap();
  symbBase = getOffset(soundArchiveSymb, sizeof(BinaryBlockHeader));

  stringTable = static_cast<StringTable*>(getOffset(symbBase, soundArchiveSymb->nameTableOffset));
  stringTable->bswap();

  soundStringTree = static_cast<StringTree*>(getOffset(symbBase, soundArchiveSymb->soundTreeOffset));
  soundStringTree->bswap();

  playerStringTree = static_cast<StringTree*>(getOffset(symbBase, soundArchiveSymb->playerTreeOffset));
  playerStringTree->bswap();

  groupStringTree = static_cast<StringTree*>(getOffset(symbBase, soundArchiveSymb->groupTreeOffset));
  groupStringTree->bswap();

  bankStringTree = static_cast<StringTree*>(getOffset(symbBase, soundArchiveSymb->bankTreeOffset));
  bankStringTree->bswap();

  // ==== FILE
  soundArchiveFile = static_cast<SoundArchiveFile*>(getOffset(fileData, sarHdr->fileBlockOffset));
  soundArchiveFile->bswap();
  fileBase = getOffset(soundArchiveFile, sizeof(BinaryBlockHeader));

  // ==== INFO
  soundArchiveInfo = static_cast<SoundArchiveInfo*>(getOffset(fileData, sarHdr->infoBlockOffset));
  soundArchiveInfo->bswap();
  infoBase = getOffset(soundArchiveInfo, sizeof(BinaryBlockHeader));

  soundTable = static_cast<SoundTable*>(soundArchiveInfo->soundTable.getAddr(infoBase));
  soundTable->bswap();

  bankTable = static_cast<BankTable*>(soundArchiveInfo->bankTable.getAddr(infoBase));
  bankTable->bswap();

  playerTable = static_cast<PlayerTable*>(soundArchiveInfo->playerTable.getAddr(infoBase));
  playerTable->bswap();

  fileTable = static_cast<FileTable*>(soundArchiveInfo->fileTable.getAddr(infoBase));
  fileTable->bswap();

  groupTable = static_cast<GroupTable*>(soundArchiveInfo->groupTable.getAddr(infoBase));
  groupTable->bswap();

  for (int i = 0; i < bankTable->size; i++) {
    BankInfo* bankInfo = static_cast<BankInfo*>(bankTable->elems[i].getAddr(infoBase));
    bankInfo->bswap();
  }

  for (int i = 0; i < playerTable->size; i++) {
    PlayerInfo* playerInfo = static_cast<PlayerInfo*>(playerTable->elems[i].getAddr(infoBase));
    playerInfo->bswap();
  }

  for (int i = 0; i < fileTable->size; i++) {
    FileInfo* fileInfo = static_cast<FileInfo*>(fileTable->elems[i].getAddr(infoBase));
    fileInfo->bswap();

    FileGroupInfo* fileGroupInfo = static_cast<FileGroupInfo*>(fileInfo->fileGroupInfo.getAddr(infoBase));
    fileGroupInfo->bswap();
    for (int j = 0; j < fileGroupInfo->size; j++) {
      FileGroup* fileGroup = static_cast<FileGroup*>(fileGroupInfo->elems[j].getAddr(infoBase));
      fileGroup->bswap();
    }
  }

  for (int i = 0; i < groupTable->size; i++) {
    GroupInfo* groupInfo = static_cast<GroupInfo*>(groupTable->elems[i].getAddr(infoBase));
    groupInfo->bswap();

    GroupItemTable* groupItemTable = static_cast<GroupItemTable*>(groupInfo->groupItemTable.getAddr(infoBase));
    groupItemTable->bswap();
    for (int j = 0; j < groupItemTable->size; j++) {
      GroupItemInfo* groupItemInfo = static_cast<GroupItemInfo*>(groupItemTable->elems[j].getAddr(infoBase));
      groupItemInfo->bswap();
    }
  }

  soundCountTable = static_cast<SoundCountTable*>(soundArchiveInfo->soundCountTable.getAddr(infoBase));
  soundCountTable->bswap();

  for (int i = 0; i < soundTable->size; i++) {
    SoundInfoEntry* soundInfoEntry = static_cast<SoundInfoEntry*>(soundTable->elems[i].getAddr(infoBase));
    soundInfoEntry->bswap();
  }
}

const FileGroup* SoundArchive::getFileGroup(u32 fileIdx, u32 fileGroupIdx) const {
  const FileGroupInfo* fileGroupInfo = getFileGroupInfo(fileIdx);
  return static_cast<FileGroup*>(fileGroupInfo->elems[fileGroupIdx].getAddr(infoBase));
}

void* SoundArchive::getInternalFileData(u32 fileIdx) const {
  const FileGroup* fileGroup = getFileGroup(fileIdx, 0);
  const GroupInfo* groupInfo = getGroupInfo(fileGroup->groupIdx);
  char* externalFileName = static_cast<char*>(groupInfo->externalFileName.getAddr(infoBase));
  if (externalFileName) return nullptr; // file belongs to external group

  const GroupItemInfo* groupItemInfo = getGroupItemInfo(fileGroup->groupIdx, fileGroup->idx);
  u32 offset = groupInfo->fileOffset + groupItemInfo->fileOffset;
  return getOffset(data, offset);
}

void* SoundArchive::getInternalFileData(const GroupInfo* groupInfo, const GroupItemInfo* groupItemInfo, size_t* fileSize) const {
  u32 offset = groupInfo->fileOffset + groupItemInfo->fileOffset;
  if (fileSize) *fileSize = groupItemInfo->fileSize;
  return getOffset(data, offset);
}

void* SoundArchive::getInternalWaveData(u32 fileIdx) const {
  const FileGroup* fileGroup = getFileGroup(fileIdx, 0);
  const GroupInfo* groupInfo = getGroupInfo(fileGroup->groupIdx);
  char* externalFileName = static_cast<char*>(groupInfo->externalFileName.getAddr(infoBase));
  if (externalFileName) return nullptr; // file belongs to external group

  const GroupItemInfo* groupItemInfo = getGroupItemInfo(fileGroup->groupIdx, fileGroup->idx);
  u32 offset = groupInfo->waveDataOffset + groupItemInfo->waveDataOffset;
  return getOffset(data, offset);
}

void* SoundArchive::getInternalWaveData(const GroupInfo* groupInfo, const GroupItemInfo* groupItemInfo, size_t* fileSize) const {
  u32 offset = groupInfo->waveDataOffset + groupItemInfo->waveDataOffset;
  if (fileSize) *fileSize = groupItemInfo->waveDataSize;
  return getOffset(data, offset);
}
}
