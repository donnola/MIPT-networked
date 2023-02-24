#include <enet/enet.h>
#include <iostream>
#include <thread>
#include <cstring>

void send_sys_time(ENetPeer *peer, uint32_t curTime)
{
  std::string message = std::to_string(curTime);
  const char *baseMsg = message.c_str();
  ENetPacket *packet = enet_packet_create(baseMsg, strlen(baseMsg) + 1, ENET_PACKET_FLAG_RELIABLE);
  enet_peer_send(peer, 0, packet);
}

void send_message(ENetPeer *peer)
{
  while(true)
  {
    std::thread t_sender([&]() { 
    std::string message;
    std::getline(std::cin, message);
    const char *baseMsg = message.c_str();
    ENetPacket *packet = enet_packet_create(baseMsg, strlen(baseMsg) + 1, ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(peer, 0, packet);
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    t_sender.join();
  }
}

int main(int argc, const char **argv)
{
  if (enet_initialize() != 0)
  {
    printf("Cannot init ENet");
    return 1;
  }
  int time_shift;

  ENetHost *client = enet_host_create(nullptr, 2, 2, 0, 0);
  if (!client)
  {
    printf("Cannot create ENet client\n");
    return 1;
  }

  ENetAddress address_lobby;
  enet_address_set_host(&address_lobby, "localhost");
  address_lobby.port = 10887;

  ENetPeer *lobbyPeer = enet_host_connect(client, &address_lobby, 2, 0);
  if (!lobbyPeer)
  {
    printf("Cannot connect to lobby");
    return 1;
  }

  std::thread t0(send_message, lobbyPeer);

  // ENetAddress address_game;
  // enet_address_set_host(&address_game, "localhost");
  // address_lobby.port = 10888;
  // ENetPeer *gamePeer = enet_host_connect(client, &address_game, 2, 0);
  // if (!gamePeer)
  // {
  //   printf("Cannot connect to game");
  //   return 1;
  // }

  srand(time(0));
  time_shift = std::rand();
  uint32_t timeStart = enet_time_get();
  uint32_t lastMicroSendTime = timeStart;
  bool connected_with_game = false;
  ENetPeer *gamePeer = nullptr;
  while (true)
  {
    ENetEvent event;
    while (enet_host_service(client, &event, 10) > 0)
    {
      switch (event.type)
      {
      case ENET_EVENT_TYPE_CONNECT:
        printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);
        break;
      case ENET_EVENT_TYPE_RECEIVE:
        printf("Packet received '%s'\n", event.packet->data);
        if (event.peer == lobbyPeer)
        {
          printf("connection to game\n");
          ENetAddress address_game;
          enet_address_set_host(&address_game, "localhost");
          address_game.port = std::atoi((const char *)(event.packet->data));
          gamePeer = enet_host_connect(client, &address_game, 2, 0);
          if (!gamePeer)
          {
            printf("Cannot connect to game");
            return 1;
          }
          connected_with_game = true;
        }
        enet_packet_destroy(event.packet);
        break;
      default:
        break;
      };
    }
    if (connected_with_game)
    {
      uint32_t curTime = enet_time_get();
      if (curTime - lastMicroSendTime > 1000)
      {
        lastMicroSendTime = curTime;
        send_sys_time(gamePeer, std::time(0) + time_shift);
      }
    }
  }

  t0.join();
  return 0;
}
