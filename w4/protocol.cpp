#include "protocol.h"
#include "bitstream.h"
#include <cstring> // memcpy

void send_join(ENetPeer *peer)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t), ENET_PACKET_FLAG_RELIABLE);
  *packet->data = E_CLIENT_TO_SERVER_JOIN;

  enet_peer_send(peer, 0, packet);
}

void send_new_entity(ENetPeer *peer, const Entity &ent)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t) + sizeof(Entity),
                                                   ENET_PACKET_FLAG_RELIABLE);
  
  WtiteBitstream w_bs(packet->data);
  w_bs.write(E_SERVER_TO_CLIENT_NEW_ENTITY);
  w_bs.write(ent);

  enet_peer_send(peer, 0, packet);
}

void send_set_controlled_entity(ENetPeer *peer, uint16_t eid)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t) + sizeof(uint16_t),
                                                   ENET_PACKET_FLAG_RELIABLE);
  
  WtiteBitstream w_bs(packet->data);
  w_bs.write(E_SERVER_TO_CLIENT_SET_CONTROLLED_ENTITY);
  w_bs.write(eid);

  enet_peer_send(peer, 0, packet);
}

void send_entity_state(ENetPeer *peer, uint16_t eid, float x, float y)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t) + sizeof(uint16_t) +
                                                   2 * sizeof(float),
                                                   ENET_PACKET_FLAG_UNSEQUENCED);
  
  WtiteBitstream w_bs(packet->data);
  w_bs.write(E_CLIENT_TO_SERVER_STATE);
  w_bs.write(eid);
  w_bs.write(x);
  w_bs.write(y);

  enet_peer_send(peer, 1, packet);
}

void send_snapshot(ENetPeer *peer, uint16_t eid, float x, float y, float size)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t) + sizeof(uint16_t) +
                                                   2 * sizeof(float) + sizeof(size_t),
                                                   ENET_PACKET_FLAG_UNSEQUENCED);
  
  WtiteBitstream w_bs(packet->data);
  w_bs.write(E_SERVER_TO_CLIENT_SNAPSHOT);
  w_bs.write(eid);
  w_bs.write(x);
  w_bs.write(y);
  w_bs.write(size);

  enet_peer_send(peer, 1, packet);
}

MessageType get_packet_type(ENetPacket *packet)
{
  MessageType message;
  ReadBitstream r_bs(packet->data);
  r_bs.read(message);
  return message;
}

void deserialize_new_entity(ENetPacket *packet, Entity &ent)
{
  MessageType message;
  ReadBitstream r_bs(packet->data);
  r_bs.read(message);
  r_bs.read(ent);
}

void deserialize_set_controlled_entity(ENetPacket *packet, uint16_t &eid)
{
  MessageType message;
  ReadBitstream r_bs(packet->data);
  r_bs.read(message);
  r_bs.read(eid);
}

void deserialize_entity_state(ENetPacket *packet, uint16_t &eid, float &x, float &y)
{
  MessageType message;
  ReadBitstream r_bs(packet->data);
  r_bs.read(message);
  r_bs.read(eid);
  r_bs.read(x);
  r_bs.read(y);
}

void deserialize_snapshot(ENetPacket *packet, uint16_t &eid, float &x, float &y, float &size)
{
  MessageType message;
  ReadBitstream r_bs(packet->data);
  r_bs.read(message);
  r_bs.read(eid);
  r_bs.read(x);
  r_bs.read(y);
  r_bs.read(size);
}

