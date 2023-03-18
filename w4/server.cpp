#include <enet/enet.h>
#include <iostream>
#include "entity.h"
#include "protocol.h"
#include <stdlib.h>
#include <vector>
#include <map>
#include <cmath>


static std::vector<Entity> entities;
static std::map<uint16_t, ENetPeer*> controlledMap;
static std::map<uint16_t, std::pair<float, float>> aiControlledMap;

void gen_ai_entities(ENetHost *host)
{
  std::vector<Entity> ai_entities;
  uint16_t maxEid = entities.empty() ? invalid_entity : entities[0].eid;
  for (const Entity &e : entities)
  {
    maxEid = std::max(maxEid, e.eid);
  }
  uint16_t newEid = maxEid + 1;

  size_t new_ai_ent = rand() % 2 + 1;
  for (size_t i = 0; i < new_ai_ent; ++i)
  {
    
    uint32_t color = 0x000000ff +
                     ((rand() % 255) << 8) +
                     ((rand() % 255) << 16) +
                     ((rand() % 255) << 24);
    float x = (rand() % win_width) - win_width / 2.f;
    float y = (rand() % win_height) - win_height / 2.f;
    float size = (rand() % int(init_size)) + min_size;
    Entity ai_ent = {color, x, y, newEid, size, EntityType::AI};
    entities.push_back(ai_ent);
    ai_entities.push_back(ai_ent);
    x = (rand() % win_width) - win_width / 2.f;
    y = (rand() % win_height) - win_height / 2.f;
    aiControlledMap[newEid] = std::pair(x, y);
    ++newEid;
  }

  for (size_t i = 0; i < host->peerCount; ++i)
  {
    for (const Entity &ent : ai_entities)
    {
      send_new_entity(&host->peers[i], ent);
    }
  }
}

void on_join(ENetPacket *packet, ENetPeer *peer, ENetHost *host)
{
  // send all entities
  gen_ai_entities(host);
  for (const Entity &ent : entities)
    send_new_entity(peer, ent);

  // find max eid
  uint16_t maxEid = entities.empty() ? invalid_entity : entities[0].eid;
  for (const Entity &e : entities)
    maxEid = std::max(maxEid, e.eid);
  uint16_t newEid = maxEid + 1;
  uint32_t color = 0x000000ff +
                   ((rand() % 255) << 8) +
                   ((rand() % 255) << 16) +
                   ((rand() % 255) << 24);
  float x = (rand() % win_width) - win_width / 2.f;
  float y = (rand() % win_height) - win_height / 2.f;
  float size = (rand() % int(min_size)) + init_size;
  Entity ent = {color, x, y, newEid, size, EntityType::PLAYER};
  entities.push_back(ent);

  controlledMap[newEid] = peer;


  // send info about new entity to everyone
  for (size_t i = 0; i < host->peerCount; ++i)
    send_new_entity(&host->peers[i], ent);
  // send info about controlled entity
  send_set_controlled_entity(peer, newEid);
}

void on_state(ENetPacket *packet)
{
  uint16_t eid = invalid_entity;
  float x = 0.f; float y = 0.f;
  deserialize_entity_state(packet, eid, x, y);
  for (Entity &e : entities)
    if (e.eid == eid)
    {
      e.x = x;
      e.y = y;
    }
}

void ai_control()
{
  static enet_uint32 last_time = enet_time_get();
  enet_uint32 cur_time = enet_time_get();
  enet_uint32 dt = cur_time - last_time;
  for (Entity &ent : entities)
  {
    if (ent.type == EntityType::AI)
    {
      std::pair<float, float>& target = aiControlledMap[ent.eid];
      std::pair<double, double> dir = std::pair(target.first - ent.x, target.second - ent.y);
      double dist = sqrt(dir.first * dir.first + dir.second * dir.second);
      if (dist < 1.f)
      {
        ent.x = target.first;
        ent.y = target.second;
        float x = (rand() % win_width) - win_width / 2.f;
        float y = (rand() % win_height) - win_height / 2.f;
        aiControlledMap[ent.eid] = std::pair(x, y);
        dir = std::pair(target.first - ent.x, target.second - ent.y);
        dist = sqrt(dir.first * dir.first + dir.second * dir.second);
      }

      dir.first /= dist;
      dir.second /= dist;
      ent.x += dir.first * dt * ai_speed;
      ent.y += dir.second * dt * ai_speed;
      
    }
  }

  last_time = cur_time;
}

void collisions()
{
  for (Entity& e1 : entities)
  for (Entity& e2 : entities)
  {
    if (e1.eid != e2.eid)
    {
      double dist = sqrt(pow(e1.x - e2.x, 2) + pow(e1.y - e2.y, 2));
      if (dist > e1.size + e2.size)
      {
        continue;
      }
      float x = (rand() % win_width) - win_width / 2.f;
      float y = (rand() % win_height) - win_height / 2.f;
      if (e1.size > e2.size)
      {
        e1.size += e2.size / 2.f;
        e2.size = std::max(min_size, e2.size / 2.f);
        e2.x = x;
        e2.y = y;
      }
      else
      {
        e2.size += e1.size / 2.f;
        e1.size = std::max(min_size, e1.size / 2.f);
        e1.x = x;
        e1.y = y;
      }
      if (e1.type == EntityType::PLAYER)
        send_snapshot(controlledMap[e1.eid], e1.eid, e1.x, e1.y, e1.size);
      if (e2.type == EntityType::PLAYER)
        send_snapshot(controlledMap[e2.eid], e2.eid, e2.x, e2.y, e2.size);
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
  ENetAddress address;

  address.host = ENET_HOST_ANY;
  address.port = 10131;

  ENetHost *server = enet_host_create(&address, 32, 2, 0, 0);

  if (!server)
  {
    printf("Cannot create ENet server\n");
    return 1;
  }

  srand(time(NULL));

  while (true)
  {
    ENetEvent event;
    while (enet_host_service(server, &event, 0) > 0)
    {
      switch (event.type)
      {
      case ENET_EVENT_TYPE_CONNECT:
        printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);
        break;
      case ENET_EVENT_TYPE_RECEIVE:
        switch (get_packet_type(event.packet))
        {
          case E_CLIENT_TO_SERVER_JOIN:
            on_join(event.packet, event.peer, server);
            break;
          case E_CLIENT_TO_SERVER_STATE:
            on_state(event.packet);
            break;
        };
        enet_packet_destroy(event.packet);
        break;
      default:
        break;
      };
    }
    static int t = 0;
    
    ai_control();
    collisions();
    for (const Entity &e : entities)
      for (size_t i = 0; i < server->peerCount; ++i)
      {
        ENetPeer *peer = &server->peers[i];
        if (e.type == EntityType::AI || controlledMap[e.eid] != peer)
          send_snapshot(peer, e.eid, e.x, e.y, e.size);
      }
    usleep(10000);
  }

  enet_host_destroy(server);

  atexit(enet_deinitialize);
  return 0;
}


