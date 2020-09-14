#pragma once

#include <memory>
#include <cstdint>
#include <enet/enet.h>
#include <glm/glm.hpp>

#define mymax(a, b) ((a) > (b) ? (a) : (b))

inline constexpr enet_uint16 COMMS_PORT = 37777;
inline constexpr float POS_SCALE = 10000.0f;
inline constexpr int TICKS_PER_SEC = 15;
inline constexpr float TIME_PER_TICK = 1.0f/TICKS_PER_SEC;
inline constexpr float MovementSpeed = 2.0f;
inline constexpr float MaxSwingAmplitude = 45.0f; // max degrees
inline constexpr float SecondsPerSwing = 0.25f;
inline constexpr float SwingSpeed = MaxSwingAmplitude/SecondsPerSwing;
inline constexpr size_t MAX_NAME_LEN = 32;
inline constexpr size_t MAX_CHAT_LINE_LEN_TXT = 32;
inline constexpr size_t MAX_CHAT_LINE_LEN = mymax(MAX_CHAT_LINE_LEN_TXT, MAX_NAME_LEN);

#undef mymax

struct EnetHostDeleter {
    void operator()(ENetHost* h)
    {
        enet_host_destroy(h);
    }
};
using ENetHostPtr = std::unique_ptr<ENetHost, EnetHostDeleter>;

struct ENetPacketDeleter {
    void operator()(ENetPacket* p)
    {
        enet_packet_destroy(p);
    }
};
using ENetPacketPtr = std::unique_ptr<ENetPacket, ENetPacketDeleter>;

// on connection, server send this
struct ServerWorldPacketInit {
    unsigned char players, your_id;
    unsigned char width, height;
    enet_uint16 bombs;
};
// and player sends back this
struct PlayerMetaPacket {
    char username[MAX_NAME_LEN];
    unsigned char cross_r, cross_g, cross_b, cross_a;
};
// --------------------------------------

// every tick, player sends this
struct ClientPlayerPacket {
    enet_uint32 x, y;
    enet_uint16 yaw, pitch;
    signed char looking_at_x, looking_at_y;
    unsigned char action;
};
// and server sends back this
struct ServerWorldPacket {
    enet_uint16 placed_flags;
    signed char result;
    unsigned char seconds, minutes;
};
// followed by NUM_PLAYERS of these
struct ServerPlayerPacket {
    enet_uint32 x, y;
    enet_uint16 yaw, pitch;
    signed char looking_at_x, looking_at_y;
};
// followed by terrain data
// --------------------------------------

// when everyone joined, server sends NUM_PLAYERS of this
struct StartDataPacket {
    ServerPlayerPacket info;
    PlayerMetaPacket meta;
};
// --------------------------------------

struct PlayerData {
    glm::vec3 position{0.0f, 0.0f, 0.0f}; // x0z
    float movedDistance = 0.0f;
    float movementSwing = 0.0f;
    float currentSwingDirection = SwingSpeed;
    int16_t yaw, pitch;
    signed char looking_at_x, looking_at_y; // y is when looking from sky, z in 3d space
    glm::vec4 color;
    char username[MAX_NAME_LEN];

    void fill(const PlayerMetaPacket& p);
    void fill(const ServerPlayerPacket& p);
    PlayerMetaPacket fill_meta() const;
    ServerPlayerPacket fill_info() const;
};
