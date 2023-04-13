// initial skeleton is a clone from https://github.com/jpcy/bgfx-minimal-example
//
#include <functional>
#include "raylib.h"
#include <enet/enet.h>
#include <math.h>
#include <algorithm> 

#include <vector>
#include <map>
#include <deque>
#include "entity.h"
#include "protocol.h"
#include "snapshot.h"


static std::map<uint16_t, Entity> entities;
static std::map<uint16_t, Snapshot> last_snapshots;
std::unordered_map<uint16_t, std::deque<Snapshot>> snapshots = {};
static uint16_t my_entity = invalid_entity;
const uint32_t offset = 200u;
constexpr uint32_t tick_time = 15;//ms
constexpr float dt = tick_time * 0.001f;


void on_new_entity_packet(ENetPacket *packet)
{
  Entity newEntity;
  deserialize_new_entity(packet, newEntity);
  if (!entities.contains(newEntity.eid) && newEntity.eid != my_entity)
  {
    entities[newEntity.eid] = std::move(newEntity);
    last_snapshots[newEntity.eid] = {newEntity.x, newEntity.y, newEntity.ori, enet_time_get()};
    snapshots[newEntity.eid] = std::deque<Snapshot>();
  }
}

void on_set_controlled_entity(ENetPacket *packet)
{
  deserialize_set_controlled_entity(packet, my_entity);
}

void on_snapshot(ENetPacket *packet)
{
  uint16_t eid = invalid_entity;
  float x = 0.f; float y = 0.f; float ori = 0.f;
  uint32_t tick = 0;
  deserialize_snapshot(packet, eid, x, y, ori, tick);
  if (eid != my_entity && snapshots.contains(eid))
  {
    snapshots[eid].emplace_back(x, y, ori, enet_time_get() + offset);
  }
}

void interpolation()
{
  for (auto & last_snapshot : last_snapshots)
  {
    uint16_t eid = last_snapshot.first;
    Snapshot & l_s = last_snapshot.second;
    auto curTime = enet_time_get();
    while (!snapshots[eid].empty())
    {
      Snapshot &s = snapshots[eid].front();
      if (curTime >= s.timestamp)
      {
        last_snapshots[eid] = s;
        snapshots[eid].pop_front();
      }
      else
      {
        float t = (float)(curTime - l_s.timestamp) / (s.timestamp - l_s.timestamp);
        entities[eid].x = (1.f - t) * l_s.x + t * s.x;
        entities[eid].y = (1.f - t) * l_s.y + t * s.y;
        entities[eid].ori = (1.f - t) * l_s.ori + t * s.ori;
        break;
      }
    }
  }
}

void simulation(float thr, float steer)
{
  Entity &entity = entities[my_entity];
  entity.thr = thr;
  entity.steer = steer;
  simulate_entity(entity, dt);
  entity.timestamp++;
}

int main(int argc, const char **argv)
{
  if (enet_initialize() != 0)
  {
    printf("Cannot init ENet");
    return 1;
  }

  ENetHost *client = enet_host_create(nullptr, 1, 2, 0, 0);
  if (!client)
  {
    printf("Cannot create ENet client\n");
    return 1;
  }

  ENetAddress address;
  enet_address_set_host(&address, "localhost");
  address.port = 10131;

  ENetPeer *serverPeer = enet_host_connect(client, &address, 2, 0);
  if (!serverPeer)
  {
    printf("Cannot connect to server");
    return 1;
  }

  int width = 600;
  int height = 600;

  InitWindow(width, height, "w5 networked MIPT");

  const int scrWidth = GetMonitorWidth(0);
  const int scrHeight = GetMonitorHeight(0);
  if (scrWidth < width || scrHeight < height)
  {
    width = std::min(scrWidth, width);
    height = std::min(scrHeight - 150, height);
    SetWindowSize(width, height);
  }

  Camera2D camera = { {0, 0}, {0, 0}, 0.f, 1.f };
  camera.target = Vector2{ 0.f, 0.f };
  camera.offset = Vector2{ width * 0.5f, height * 0.5f };
  camera.rotation = 0.f;
  camera.zoom = 10.f;


  SetTargetFPS(60);               // Set our game to run at 60 frames-per-second
  bool connected = false;
  while (!WindowShouldClose())
  {
    float dt = GetFrameTime();
    ENetEvent event;
    while (enet_host_service(client, &event, 0) > 0)
    {
      switch (event.type)
      {
      case ENET_EVENT_TYPE_CONNECT:
        printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);
        send_join(serverPeer);
        connected = true;
        break;
      case ENET_EVENT_TYPE_RECEIVE:
        switch (get_packet_type(event.packet))
        {
        case E_SERVER_TO_CLIENT_NEW_ENTITY:
          on_new_entity_packet(event.packet);
          break;
        case E_SERVER_TO_CLIENT_SET_CONTROLLED_ENTITY:
          on_set_controlled_entity(event.packet);
          break;
        case E_SERVER_TO_CLIENT_SNAPSHOT:
          on_snapshot(event.packet);
          break;
        };
        break;
      default:
        break;
      };
    }
    if (my_entity != invalid_entity)
    {
      bool left = IsKeyDown(KEY_LEFT);
      bool right = IsKeyDown(KEY_RIGHT);
      bool up = IsKeyDown(KEY_UP);
      bool down = IsKeyDown(KEY_DOWN);
      // TODO: Direct adressing, of course!
      if (entities.contains(my_entity))
      {
        // Update
        float thr = (up ? 1.f : 0.f) + (down ? -1.f : 0.f);
        float steer = (left ? -1.f : 0.f) + (right ? 1.f : 0.f);
        simulation(thr, steer);
        
        // Send
        send_entity_input(serverPeer, my_entity, thr, steer);
      }
    }
    interpolation();
    BeginDrawing();
      ClearBackground(GRAY);
      BeginMode2D(camera);
        for (const auto &e : entities)
        {
          const Rectangle rect = {e.second.x, e.second.y, 3.f, 1.f};
          DrawRectanglePro(rect, {0.f, 0.5f}, e.second.ori * 180.f / PI, GetColor(e.second.color));
        }

      EndMode2D();
    EndDrawing();
  }

  CloseWindow();
  return 0;
}
