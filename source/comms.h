#pragma once

#include <memory>
#include <cstdint>
#include <enet/enet.h>
#include <glm/glm.hpp>

#define mymax(a, b) ((a) > (b) ? (a) : (b))

inline constexpr enet_uint16 COMMS_PORT = 37777;
inline constexpr float POS_SCALE = 100000.0f;
inline constexpr float MovementSpeed =  3.75f;
inline constexpr size_t MAX_NAME_LEN = 32;
inline constexpr size_t MAX_CHAT_LINE_LEN_TXT = 32;
inline constexpr size_t MAX_CHAT_LINE_LEN = mymax(MAX_CHAT_LINE_LEN_TXT, MAX_NAME_LEN);

struct EnetHostDeleter {
    void operator()(ENetHost* h)
    {
        enet_host_destroy(h);
    }
};
using EnetHostPtr = std::unique_ptr<ENetHost, EnetHostDeleter>;

struct ENetPacketDeleter {
    void operator()(ENetPacket* p)
    {
        enet_packet_destroy(p);
    }
};
using EnetPacketPtr = std::unique_ptr<ENetPacket, ENetPacketDeleter>;


// on connection, server send this
struct SCPacketDataInit {
    unsigned char players, your_id;
    unsigned char width, height;
    enet_uint16 bombs;
};
// and player sends back this
struct PacketDataPlayerInit {
    unsigned char cross_r, cross_g, cross_b, cross_a;
    char username[MAX_NAME_LEN];
};
// --------------------------------------

// every tick, player sends this
struct CSPacketData {
    enet_uint32 x, y;
    enet_uint16 yaw, pitch;
    signed char looking_at_x, looking_at_y;
    unsigned char action;
};
// and server sends back this
struct SCPacketData {
    enet_uint16 placed_flags;
    unsigned char seconds, minutes;
    signed char result;
};
// followed by NUM_PLAYERS of these
struct SCPacketDataPlayer {
    enet_uint32 x, y;
    enet_uint16 yaw, pitch;
    signed char looking_at_x, looking_at_y;
};
// followed by modified terrain data
// --------------------------------------

// when everyone joined, server sends NUM_PLAYERS of this
struct LaunchData {
    SCPacketDataPlayer info;
    PacketDataPlayerInit meta;
};
// --------------------------------------

struct PlayerData {
    glm::vec3 position{0.0f, 0.0f, 0.0f}; // x0z
    int16_t yaw, pitch;
    signed char looking_at_x, looking_at_y; // y is when looking from sky, z in 3d space
    glm::vec4 color;
    char username[MAX_NAME_LEN];

    void fill(const PacketDataPlayerInit& p);
    void fill(const SCPacketDataPlayer& p);
    PacketDataPlayerInit fill_meta() const;
    SCPacketDataPlayer fill_info() const;
};
