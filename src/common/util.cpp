
#include <bit>
#include <concepts>

#include "common/util.h"

namespace rsnd {
void BinaryBlockHeader::bswap() {
  length = std::byteswap(length);
}

void BinaryFileHeader::bswap() {
  byteOrder = std::byteswap(byteOrder);
  version = std::byteswap(version);
  fileSize = std::byteswap(fileSize);
  headerSize = std::byteswap(headerSize);
  numBlocks = std::byteswap(numBlocks);
}

void DataRef::bswap() {
  value = std::byteswap(value);
}
}
