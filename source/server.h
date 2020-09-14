#pragma once

#include "comms.h"
#include <vector>
#include <string>
#include <ctime>

struct ServClient {
    char idx;
    bool set = false;
    bool connected = false;
    PlayerData data;
    ClientPlayerPacket doing;
};

struct WorldTile {
    char hidden = '0';
    char visible = '.';
    bool visited = false;
    bool fresh_visit = true;

    void visit()
    {
        visited = true;
        fresh_visit = true;
    }
    void reveal()
    {
        visible = hidden;
    }

    unsigned char prepare()
    {
        using u8 = unsigned char;
        u8 out = visible;
        out |= u8(fresh_visit) << 7;
        return out;
    }
};

struct MineServer {
    MineServer(int map_width, int map_height, int bombs_percent, int player_amount);

    bool is_all_set;

    void update(const float deltatime);
    void send_update();

    void receive();
    bool should_shutdown() const;

private:
    bool all_set() const;
    int find_not_connected();

    unsigned char width, height;
    bool had_first;
    enet_uint16 bombs;
    std::vector<WorldTile> world;
    std::vector<ServClient> clients;
    std::vector<unsigned char> data_to_send;
    ENetHostPtr host;
    ENetAddress address;

    ServerWorldPacketInit init;
    ServerWorldPacket cur_state;
    time_t start_time;
    std::string chatted;
    bool generated;
};
