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
std::deque<Snapshot> my_entity_snapshots;
std::deque<Input> my_entity_input;
static uint16_t my_entity = invalid_entity;
const uint32_t offset = 200u;

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

void decreaseMyEntityInput()
{
  int num = my_entity_input.size() - 100;
  if (num > 0)
  {  
    my_entity_input.erase(my_entity_input.begin(), my_entity_input.begin() + num);
  }
}

void simulation(float thr, float steer, uint32_t ticks) 
{
  Entity &entity = entities[my_entity];
  entity.thr = thr;
  entity.steer = steer;
  for (uint32_t t = 0; t < ticks; ++t)
  {
    simulate_entity(entity, dt);
    entity.timestamp++;
    my_entity_snapshots.emplace_back(entity.x, entity.y, entity.ori, entity.timestamp);
  }
}

void recalculate_state(uint32_t old_timestamp, uint32_t cur_timestamp)
{
 auto iterErase = my_entity_input.begin();
  for (auto iter = my_entity_input.begin(); (iter != my_entity_input.end() && iter->timestamp < cur_timestamp);)
  {
    if (iter->timestamp >= old_timestamp)
    {
      if (iter + 1 < my_entity_input.end())
        simulation(iter->thr, iter->steer, (iter + 1)->timestamp - iter->timestamp);
      else
        simulation(iter->thr, iter->steer, cur_timestamp - iter->timestamp);
    }
    else
    {
      iterErase = iter;
    }
    ++iter;
  }

  my_entity_input.erase(my_entity_input.begin(), iterErase);
}

void on_snapshot(ENetPacket *packet)
{
  uint16_t eid = invalid_entity;
  float x = 0.f; float y = 0.f; float ori = 0.f;
  uint32_t cur_timestamp = 0;
  deserialize_snapshot(packet, eid, x, y, ori, cur_timestamp);
  if (eid != my_entity && snapshots.contains(eid))
  {
    snapshots[eid].emplace_back(x, y, ori, enet_time_get() + offset);
  }
  else
  {
    while (!my_entity_snapshots.empty())
    {
      Snapshot &s = snapshots[eid].front();
      if (s.timestamp > cur_timestamp)
      {
        break;
      }
      if (s.timestamp == cur_timestamp && (s.x != x || s.y != y || s.ori != ori))
      {
        entities[my_entity].x = x;
        entities[my_entity].y = y;
        entities[my_entity].ori = ori;
        uint32_t old_timestamp = entities[my_entity].timestamp;
        recalculate_state(old_timestamp, cur_timestamp);
        entities[my_entity].timestamp = cur_timestamp;
        my_entity_snapshots.clear();
        break;
      }
      my_entity_snapshots.pop_front();
    }
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
  uint32_t lastTime = enet_time_get();
  while (!WindowShouldClose())
  {
    uint32_t curTime = enet_time_get();
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
        uint32_t last_timestamp = entities[my_entity].timestamp;
        auto input = Input{thr, steer, entities[my_entity].timestamp};
        simulation(thr, steer, time_to_tick(curTime - lastTime));
        lastTime += time_to_tick(curTime - lastTime) * tick_time;
        // Send
        if (last_timestamp != entities[my_entity].timestamp)
        {
          my_entity_input.emplace_back(input);
          // Send
          send_entity_input(serverPeer, my_entity, thr, steer);
        }
      }
    }
    interpolation();
    BeginDrawing();
    decreaseMyEntityInput();
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
