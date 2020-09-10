#pragma once

#include "comms.h"
#include <vector>
#include <ctime>

struct ServClient {
    char idx;
    bool set = false;
    bool connected = false;
    PlayerData data;
    CSPacketData doing;
    EnetPacketPtr init_packet;
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
    EnetHostPtr host;
    ENetAddress address;

    SCPacketDataInit init;
    SCPacketData cur_state;
    EnetPacketPtr first_packet;
    std::array<EnetPacketPtr, 2> upd_packets;
    int upd_packet_id;
    time_t start_time;
    bool generated;
};
