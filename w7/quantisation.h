#pragma once
#include "mathUtils.h"
#include <limits>

struct float2
{
  float x;
  float y;

  float2(float a, float b) : x(a), y(b) {}
};

struct float3
{
  float x;
  float y;
  float z;

  float3(float a, float b, float c) : x(a), y(b), z(c) {}
};

template<typename T>
T pack_float(float v, float lo, float hi, int num_bits)
{
  T range = (1 << num_bits) - 1;//std::numeric_limits<T>::max();
  return range * ((clamp(v, lo, hi) - lo) / (hi - lo));
}

template<typename T>
float unpack_float(T c, float lo, float hi, int num_bits)
{
  T range = (1 << num_bits) - 1;//std::numeric_limits<T>::max();
  return float(c) / range * (hi - lo) + lo;
}

template<typename T, int num_bits>
struct PackedFloat
{
  T packedVal;

  PackedFloat(float v, float lo, float hi) { pack(v, lo, hi); }
  PackedFloat(T compressed_val) : packedVal(compressed_val) {}

  void pack(float v, float lo, float hi) { packedVal = pack_float<T>(v, lo, hi, num_bits); }
  float unpack(float lo, float hi) { return unpack_float<T>(packedVal, lo, hi, num_bits); }
};

typedef PackedFloat<uint8_t, 4> float4bitsQuantized;


template<typename T, int num_bits_x, int num_bits_y>
struct PackedFloat2
{
  T packedVal;

  PackedFloat2(float2 v, float2 lo, float2 hi) { pack(v, lo, hi); }
  PackedFloat2(T compressed_val) : packedVal(compressed_val) {}

  void pack(float2 v, float2 lo, float2 hi) 
  { 
    T packedValX = pack_float<T>(v.x, lo.x, hi.x, num_bits_x); 
    T packedValY = pack_float<T>(v.y, lo.y, hi.y, num_bits_y); 
    packedVal = packedValX << num_bits_y | packedValY;
  }
  float2 unpack(float2 lo, float2 hi) 
  { 
    float x = unpack_float<T>(packedVal >> num_bits_y, lo.x, hi.x, num_bits_x);
    float y = unpack_float<T>(packedVal & ((1 << num_bits_y) - 1), lo.y, hi.y, num_bits_y);
    return float2(x, y);
  }
};

typedef PackedFloat2<uint32_t, 16, 16> float2_16_16bitsQuantized;

template<typename T, int num_bits_x, int num_bits_y, int num_bits_z>
struct PackedFloat3
{
  T packedVal;

  PackedFloat3(float3 v, float3 lo, float3 hi) { pack(v, lo, hi); }
  PackedFloat3(T compressed_val) : packedVal(compressed_val) {}

  void pack(float3 v, float3 lo, float3 hi) 
  { 
    T packedValX = pack_float<T>(v.x, lo.x, hi.x, num_bits_x); 
    T packedValY = pack_float<T>(v.y, lo.y, hi.y, num_bits_y); 
    T packedValZ = pack_float<T>(v.z, lo.z, hi.z, num_bits_z); 
    packedVal = (packedValX << (num_bits_y + num_bits_z)) | (packedValY << num_bits_z) | packedValZ;
  }
  float3 unpack(float3 lo, float3 hi) 
  { 
    float x = unpack_float<T>(packedVal >> (num_bits_y + num_bits_z), lo.x, hi.x, num_bits_x);
    float y = unpack_float<T>((packedVal >> num_bits_z) & ((1 << num_bits_y) - 1), lo.y, hi.y, num_bits_y);
    float z = unpack_float<T>(packedVal & ((1 << num_bits_z) - 1), lo.z, hi.z, num_bits_z);
    return float3(x, y, z); 
  }
};

typedef PackedFloat3<uint64_t, 22, 20, 22> float3_22_20_22bitsQuantized;

// void packed_int32(int32_t n)
// {

// };