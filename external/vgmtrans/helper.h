/*
 * VGMTrans (c) 2002-2021
 * Licensed under the zlib license,
 * refer to the included LICENSE.txt file
 */

#pragma once

template <class T>
void DeleteList(std::list<T *> &list) {
  for (auto p : list) {
    delete p;
  }
  list.clear();
}

template <class T>
inline void PushTypeOnVect(std::vector<uint8_t> &theVector, T unit) {
  theVector.insert(theVector.end(), reinterpret_cast<uint8_t *>(&unit),
                   reinterpret_cast<uint8_t *>(&unit) + sizeof(T));
}

template <class T>
inline void PushTypeOnVectBE(std::vector<uint8_t> &theVector, T unit) {
  for (uint32_t i = 0; i < sizeof(T); i++) {
    theVector.push_back(*(reinterpret_cast<uint8_t *>(&unit) - i + sizeof(T) - 1));
  }
}