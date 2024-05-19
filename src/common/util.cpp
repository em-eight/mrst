
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

f32 bswap_float(const float inFloat) {
   float retVal;
   char *floatToConvert = ( char* ) & inFloat;
   char *returnFloat = ( char* ) & retVal;

   // swap the bytes into a temporary buffer
   returnFloat[0] = floatToConvert[3];
   returnFloat[1] = floatToConvert[2];
   returnFloat[2] = floatToConvert[1];
   returnFloat[3] = floatToConvert[0];

   return retVal;
}
