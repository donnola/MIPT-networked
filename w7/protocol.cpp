#include "protocol.h"
#include "quantisation.h"
#include "bitstream.h"
#include <cstring> // memcpy
#include <iostream>

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
                                                   sizeof(uint8_t),
                                                   ENET_PACKET_FLAG_UNSEQUENCED);
  WtiteBitstream w_bs(packet->data);
  w_bs.write(E_CLIENT_TO_SERVER_INPUT);
  w_bs.write(eid);
  PackedFloat2<uint8_t, 4, 4> thrSteerPacked(float2(thr, steer), float2(-1.f, -1.f), float2(1.f, 1.f));
  w_bs.write(thrSteerPacked.packedVal);

  enet_peer_send(peer, 1, packet);
}

typedef PackedFloat<uint16_t, 11> PositionXQuantized;
typedef PackedFloat<uint16_t, 10> PositionYQuantized;

void send_snapshot(ENetPeer *peer, uint16_t eid, float x, float y, float ori)
{
  ENetPacket *packet = enet_packet_create(nullptr, sizeof(uint8_t) + sizeof(uint16_t) +
                                                   sizeof(uint16_t) +
                                                   sizeof(uint16_t) +
                                                   sizeof(uint8_t),
                                                   ENET_PACKET_FLAG_UNSEQUENCED);
  WtiteBitstream w_bs(packet->data);
  w_bs.write(E_SERVER_TO_CLIENT_SNAPSHOT);
  w_bs.write(eid);
  PositionXQuantized xPacked(x, -16, 16);
  PositionYQuantized yPacked(y, -8, 8);
  uint8_t oriPacked = pack_float<uint8_t>(ori, -PI, PI, 8);
  PackedFloat3<uint32_t, 11, 10, 8> xyOriPacked(float3(x, y, ori), float3(-16, -8, -PI), float3(16, 8, PI));
  w_bs.write(xyOriPacked.packedVal);

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
  uint8_t thrSteerPackedVal = 0;
  r_bs.read(thrSteerPackedVal);
  PackedFloat2<uint8_t, 4, 4> thrSteerPacked(thrSteerPackedVal);
  float2 thrSteerUnpacked = thrSteerPacked.unpack(float2(-1.f, -1.f), float2(1.f, 1.f));
  static uint8_t neutralPackedValue = pack_float<uint8_t>(0.f, -1.f, 1.f, 4);
  thr = (thrSteerPackedVal >> 4) == neutralPackedValue ? 0.f : thrSteerUnpacked.x;
  steer = (thrSteerPackedVal & 0x0f) == neutralPackedValue ? 0.f : thrSteerUnpacked.y;
}

void deserialize_snapshot(ENetPacket *packet, uint16_t &eid, float &x, float &y, float &ori)
{

  MessageType message;
  ReadBitstream r_bs(packet->data);
  r_bs.read(message);
  r_bs.read(eid);
  uint32_t xyOriPackedVal = 0;
  r_bs.read(xyOriPackedVal);
  PackedFloat3<uint32_t, 11, 10, 8> xyOriPacked(xyOriPackedVal);
  float3 xyOriUnpacked = xyOriPacked.unpack(float3(-16, -8, -PI), float3(16, 8, PI));
  x = xyOriUnpacked.x;
  y = xyOriUnpacked.y;
  ori = xyOriUnpacked.z;
}

