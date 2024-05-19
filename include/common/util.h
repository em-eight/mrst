
#pragma once

#include <type_traits>
#include <bit>

#include "types.h"

namespace rsnd {
struct BinaryBlockHeader {
  char magic[4];
  u32 length;

  void bswap();
};

struct BinaryFileHeader {
  char magic[4];
  u16 byteOrder;
  u16 version;
  u32 fileSize;
  u16 headerSize;
  u16 numBlocks;

  void bswap();
};

template<typename T>
struct Array {
  u32 size;
  T elems[1];

  void bswap() {
    size = std::byteswap(size);
    for (int i = 0; i < size; i++) {
      if constexpr(std::is_integral_v<T>) {
        elems[i] = std::byteswap(elems[i]);
      } else {
        elems[i].bswap();
      }
    }
  }
};

enum RefType {
  REFTYPE_ADDRESS = 0,
  REFTYPE_OFFSET = 1,
};

struct DataRef {
  u8 refType;
  u8 dataType;
  u32 value;

  void bswap();
  void* getAddr(void* ptr) const {if (refType == 0) return reinterpret_cast<void*>(value); else return (u8*)ptr+value;}
  template<typename T>
  T* getAddr(void* ptr) const {if (refType == 0) return reinterpret_cast<T*>(value); else return reinterpret_cast<T*>((u8*)ptr+value);}
};

inline void* getOffset(void* ptr, u32 offset) { return reinterpret_cast<u8*>(ptr) + offset; }
template<typename T>
inline T* getOffsetT(void* ptr, u32 offset) { return reinterpret_cast<T*>(reinterpret_cast<u8*>(ptr) + offset); }
template<typename T>
inline const T* getOffsetT(const void* ptr, u32 offset) { return reinterpret_cast<const T*>(reinterpret_cast<const u8*>(ptr) + offset); }
}
f32 bswap_float(const float inFloat);