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

void send_entity_input(ENetPeer *peer, uint16_t eid, float thr, float steer)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t) + sizeof(uint16_t) +
                                                   2 * sizeof(float),
                                                   ENET_PACKET_FLAG_UNSEQUENCED);
  WtiteBitstream w_bs(packet->data);
  w_bs.write(E_CLIENT_TO_SERVER_INPUT);
  w_bs.write(eid);
  w_bs.write(thr);
  w_bs.write(steer);

  enet_peer_send(peer, 1, packet);
}

void send_snapshot(ENetPeer *peer, MessageType msg, uint16_t eid, float x, float y, float ori, uint32_t tick)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t) + sizeof(uint16_t) +
                                                   2 * sizeof(float) + sizeof(size_t) + sizeof(uint32_t),
                                                   ENET_PACKET_FLAG_UNSEQUENCED);
  
  WtiteBitstream w_bs(packet->data);
  w_bs.write(msg);
  w_bs.write(eid);
  w_bs.write(x);
  w_bs.write(y);
  w_bs.write(ori);
  w_bs.write(tick);

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

void deserialize_entity_input(ENetPacket *packet, uint16_t &eid, float &thr, float &steer)
{
  MessageType message;
  ReadBitstream r_bs(packet->data);
  r_bs.read(message);
  r_bs.read(eid);
  r_bs.read(thr);
  r_bs.read(steer);
}

void deserialize_snapshot(ENetPacket *packet, uint16_t &eid, float &x, float &y, float &ori, uint32_t &tick)
{
  MessageType message;
  ReadBitstream r_bs(packet->data);
  r_bs.read(message);
  r_bs.read(eid);
  r_bs.read(x);
  r_bs.read(y);
  r_bs.read(ori);
  r_bs.read(tick);
}


