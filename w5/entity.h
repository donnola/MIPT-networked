#pragma once
#include <cstdint>

constexpr uint16_t invalid_entity = -1;
struct Entity
{
  uint32_t color = 0xff00ffff;
  float x = 0.f;
  float y = 0.f;
  float speed = 0.f;
  float ori = 0.f;

  float thr = 0.f;
  float steer = 0.f;

  uint16_t eid = invalid_entity;
  uint32_t timestamp = 0;
};

void simulate_entity(Entity &e, float dt);

constexpr uint32_t tick_time = 15;//ms
constexpr float dt = tick_time * 0.001f;


uint32_t time_to_tick(uint32_t time);

