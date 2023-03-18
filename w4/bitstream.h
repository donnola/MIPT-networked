#pragma once

#include <cstring>

class WtiteBitstream
{
public:
  WtiteBitstream(uint8_t* p) : ptr(p) {}
  template<typename T>
  void write(const T& data)
  {
    memcpy(ptr + offset, reinterpret_cast<const uint8_t*>(&data), sizeof(T));
    offset += sizeof(T);
  }

private:
  uint8_t* ptr;
  uint32_t offset = 0;
};

class ReadBitstream
{
public:
  ReadBitstream(uint8_t* p) : ptr(p) {}

  template<typename T>
  void read(T& data) 
  {
    memcpy(reinterpret_cast<uint8_t*>(&data), ptr + offset, sizeof(T));
    offset += sizeof(T);
  }

private:
  uint8_t* ptr;
  uint32_t offset = 0;
};