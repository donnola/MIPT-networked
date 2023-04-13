#pragma once

struct Snapshot
{
  float x;
  float y;
  float ori;
  uint32_t timestamp;
};

struct Input
{
  float thr;
  float steer;
  uint32_t timestamp; 
};