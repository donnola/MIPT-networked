#include <enet/enet.h>
#include <iostream>
#include <cstring>
#include <ctime>
#include <map>

struct PlayerInfo
{
  int id;
  std::string name;
  PlayerInfo() = default;
  PlayerInfo(int i, std::string n) : id(i), name(n) {}
};

void send_connection_info_to_players(const std::map<ENetPeer*, PlayerInfo> &players, ENetPeer *peer)
{
  std::string message = "player " + players.at(peer).name + " connected to game";
  const char *baseMsg = message.c_str();
  for (const auto &p : players)
  {
    if (p.first != peer)
    {
      ENetPacket *packet = enet_packet_create(baseMsg, strlen(baseMsg) + 1, ENET_PACKET_FLAG_RELIABLE);
      enet_peer_send(p.first, 0, packet);
    }
  }
}

void send_list_of_players(const std::map<ENetPeer*, PlayerInfo> &players, ENetPeer *peer)
{
  std::string message;
  for (const auto &p : players)
  {
    if (p.first != peer)
    {
      message += "player " + p.second.name + " in game game\n";
    }
  }
  const char *baseMsg = message.c_str();
  ENetPacket *packet = enet_packet_create(baseMsg, strlen(baseMsg) + 1, ENET_PACKET_FLAG_RELIABLE);
  enet_peer_send(peer, 0, packet);
}

void send_sys_time(const std::map<ENetPeer*, PlayerInfo> &players, uint32_t curTime)
{
  std::string message = std::to_string(curTime);
  const char *baseMsg = message.c_str();
  for (const auto &p : players)
  {
    ENetPacket *packet = enet_packet_create(baseMsg, strlen(baseMsg) + 1, ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(p.first, 0, packet);
  }
}

void send_ping_from_player(const std::map<ENetPeer*, PlayerInfo> &players, ENetPeer *peer, int data)
{
  std::string message = "ping from " + players.at(peer).name + " :" + std::to_string(data);
  const char *baseMsg = message.c_str();
  for (const auto &p : players)
  {
    if (p.first != peer)
    {
      ENetPacket *packet = enet_packet_create(baseMsg, strlen(baseMsg) + 1, ENET_PACKET_FLAG_UNSEQUENCED);
      enet_peer_send(p.first, 1, packet);
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
  address.port = 10888;

  ENetHost *server = enet_host_create(&address, 32, 2, 0, 0);

  if (!server)
  {
    printf("Cannot create ENet server\n");
    return 1;
  }

  std::map<ENetPeer*, PlayerInfo> players;
  int last_id = 0;
  uint32_t timeStart = enet_time_get();
  uint32_t lastMicroSendTime = timeStart;
  while (true)
  {
    ENetEvent event;
    while (enet_host_service(server, &event, 10) > 0)
    {
      switch (event.type)
      {
      case ENET_EVENT_TYPE_CONNECT:
        printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);
        players[event.peer] = PlayerInfo(last_id + 1, "player_" + std::to_string(last_id + 1));
        ++last_id;
        send_connection_info_to_players(players, event.peer);
        send_list_of_players(players, event.peer);
        break;
      case ENET_EVENT_TYPE_RECEIVE:
        printf("Packet received '%s' from '%s'\n", event.packet->data, players.at(event.peer).name.c_str());
        send_ping_from_player(players, event.peer, std::atoi((char *)event.packet->data));
        enet_packet_destroy(event.packet);
        break;
      default:
        break;
      };
    }
    uint32_t curTime = enet_time_get();
    if (!players.empty() && curTime - lastMicroSendTime > 1000)
    {
      lastMicroSendTime = curTime;
      send_sys_time(players, std::time(0));
    }
  }
  enet_host_destroy(server);

  atexit(enet_deinitialize);
  return 0;
}