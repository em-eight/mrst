
#include <algorithm>

#include "rsnd/soundCommon.hpp"
#include "tools/common.hpp"

namespace rsnd {
std::string magicLowercase(void* fileData) {
  std::string magic = getFileFourcc(fileData);
  std::transform(magic.begin(), magic.end(), magic.begin(), [](unsigned char c){ return std::tolower(c); });
  return magic;
}
}