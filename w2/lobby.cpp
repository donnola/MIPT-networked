#include <enet/enet.h>
#include <iostream>
#include <cstring>
#include <set>

void send_game_port_to_players(const std::set<ENetPeer*> &players, int game_addres_port)
{
  for (const auto & p : players)
  {
    std::string message = std::to_string(game_addres_port);
    const char *baseMsg = message.c_str();
    ENetPacket *packet = enet_packet_create(baseMsg, strlen(baseMsg) + 1, ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(p, 0, packet);
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
  address.port = 10887;

  ENetHost *server = enet_host_create(&address, 32, 2, 0, 0);

  if (!server)
  {
    printf("Cannot create ENet server\n");
    return 1;
  }
  std::set<ENetPeer*> players;
  int game_addres_port = 10888;
  bool is_game_start = false;

  while (true)
  {
    ENetEvent event;
    while (enet_host_service(server, &event, 10) > 0)
    {
      switch (event.type)
      {
      case ENET_EVENT_TYPE_CONNECT:
        printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);
        players.insert(event.peer);
        if(is_game_start)
        {
          std::string message = std::to_string(game_addres_port);
          const char *baseMsg = message.c_str();
          ENetPacket *packet = enet_packet_create(baseMsg, strlen(baseMsg) + 1, ENET_PACKET_FLAG_RELIABLE);
          enet_peer_send(event.peer, 0, packet);
        }
        break;
      case ENET_EVENT_TYPE_RECEIVE:
        printf("Packet received '%s'\n", event.packet->data);
        if (strcmp((const char*)event.packet->data, "start") == 0)
        {
          is_game_start = true;
          send_game_port_to_players(players, game_addres_port);
        }
        enet_packet_destroy(event.packet);
        break;
      default:
        break;
      };
    }
  }

  enet_host_destroy(server);

  atexit(enet_deinitialize);
  return 0;
}

