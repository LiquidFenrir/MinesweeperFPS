#include "server.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdio>

class Coord {
    int width, height;

public: 
    int x, y;

    Coord(int x_, int y_, const int w, const int h)
    :
    width(w), height(h), x(x_), y(y_)
    {

    }

    static Coord from_idx(int idx, const int w, const int h)
    {
        const div_t d = div(idx, w);
        return Coord(d.rem, d.quot, w, h);
    }
    int to_idx() const
    {
        return x + width * y;
    }

    Coord move(const int dx, const int dy) const
    {
        return Coord(x + dx, y + dy, width, height);
    }
    Coord move_left() const
    {
        return move(-1, 0);
    }
    Coord move_right() const
    {
        return move(+1, 0);
    }
    Coord move_up() const
    {
        return move(0, +1);
    }
    Coord move_down() const
    {
        return move(0, -1);
    }

    bool is_bottom() const
    {
        return y == 0;
    }
    bool is_top() const
    {
        return y == height - 1;
    }
    bool is_left() const
    {
        return x == 0;
    }
    bool is_right() const
    {
        return x == width - 1;
    }
};

namespace {
    inline constexpr float diam_of_spawn_circle = 7.0f;
    inline constexpr float radius_of_spawn_circle = diam_of_spawn_circle / 2.0f;
    void fill_pos_and_angle_start(PlayerData& p, const int idx, const int players_total, const int width, const int height)
    {
        p.looking_at_x = -1;
        p.looking_at_y = -1;
        p.pitch = 0;
        const float center_x = width / 2.0f;
        const float center_y = height / 2.0f;
        if(players_total == 1)
        {
            p.position = {center_x, 0.0f, center_y};
            p.yaw = 90;
        }
        else
        {
            const int16_t ang = ((idx / float(players_total)) * 360.0f);
            const auto ang_rads = (ang * 3.14159265f)/180.0f;
            const float x = (cosf(ang_rads) * radius_of_spawn_circle) + center_x;
            const float y = (sinf(ang_rads) * radius_of_spawn_circle) + center_y;

            p.position = {x, 0.0f, y};
            p.yaw = (ang + 360 + 180) % 360;
        }
    }

    struct MineInfo {
        std::vector<WorldTile>& world;
        const unsigned char width, height;
        const enet_uint16 bombs;
        enet_uint16& placed_flags;
    };

    void check_around(MineInfo& mines, Coord c)
    {
        auto& cur = mines.world[c.to_idx()];
        if(cur.visited) return;

        if(cur.visible == 'f') mines.placed_flags--;

        cur.reveal();
        cur.visit();

        if(cur.hidden == ' ')
        {
            if(!c.is_top())
                check_around(mines, c.move_up());
            if(!c.is_bottom())
                check_around(mines, c.move_down());
            if(!c.is_left())
                check_around(mines, c.move_left());
            if(!c.is_right())
                check_around(mines, c.move_right());
                
            if(!c.is_top() && !c.is_left())
                check_around(mines, c.move_up().move_left());
            if(!c.is_top() && !c.is_right())
                check_around(mines, c.move_up().move_right());
            if(!c.is_bottom() && !c.is_left())
                check_around(mines, c.move_down().move_left());
            if(!c.is_bottom() && !c.is_right())
                check_around(mines, c.move_down().move_right());
        }
    }

    void generate_bombs(MineInfo& mines, const Coord center)
    {
        srand(time(NULL));
        const auto size = mines.world.size();
        for(int i = 0; i < mines.bombs; i++)
        {
            int pos = rand() % size;
            Coord pos_p = Coord::from_idx(pos, mines.width, mines.height);
            while(mines.world[pos].hidden == '.' || ( \
                (pos_p.x >= (center.x - 1) && pos_p.x <= (center.x + 1)) && \
                (pos_p.y >= (center.y - 1) && pos_p.y <= (center.y + 1)))
            ) {
                pos = rand() % size;
                pos_p = Coord::from_idx(pos, mines.width, mines.height);
            }

            mines.world[pos].hidden = '.';
            const int x_beg = pos_p.x - 1;
            const int x_end = x_beg + 2;
            const int y_beg = pos_p.y - 1;
            const int y_end = y_beg + 2;
            for(int x = x_beg; x <= x_end; x++)
            {
                if(x < 0 || x >= mines.width)
                    continue;

                for(int y = y_beg; y <= y_end; y++)
                {
                    if(y < 0 || y >= mines.height)
                        continue;
                    
                    const int idx = Coord(x, y, mines.width, mines.height).to_idx();
                    if(mines.world[idx].hidden != '.')
                        mines.world[idx].hidden++;
                }
            }
        }

        for(auto& s : mines.world)
        {
            if(s.hidden == '0')
                s.hidden = ' ';
        }
    }

    int reveal(MineInfo& mines, bool& generated, time_t& start_time, Coord at)
    {
        if(!generated)
        {
            generate_bombs(mines, at);
            generated = true;
            start_time = time(nullptr);
        }

        auto& cur = mines.world[at.to_idx()];
        if(cur.visible != 'o' && cur.visible != '.') return 0;

        if(cur.hidden == ' ')
        {
            check_around(mines, at);
        }
        else
        {
            cur.reveal();
            cur.fresh_visit = true;
        }

        if(cur.hidden == '.')
        {
            return -1;
        }
        else
        {
            for(int y = 0; y < mines.height; y++)
            {
                for(int x = 0; x < mines.width; x++)
                {
                    const auto idx = Coord(x, y, mines.width, mines.height).to_idx();
                    const auto& at_idx = mines.world[idx];
                    if(at_idx.hidden == '.' && (at_idx.visible == '.' || at_idx.visible == 'o' || at_idx.visible == 'f')) // bomb and it's (not found) or (flagged) or (was flagged)
                    {
                        continue;
                    }
                    else if(at_idx.hidden != at_idx.visible) // not revealed yet
                    {
                        return 0;
                    }
                }
            }
            return 1;
        }
    }

    void toggle_flag(std::vector<WorldTile>& world, enet_uint16& placed_flags, Coord at)
    {
        auto& cur = world[at.to_idx()];
        if(cur.visible == '.' || cur.visible == 'o')
        {
            cur.visible = 'f';
            cur.fresh_visit = true;
            placed_flags++;
        }
        else if(cur.visible == 'f')
        {
            cur.visible = 'o';
            cur.fresh_visit = true;
            placed_flags--;
        }
    }
}

MineServer::MineServer(int map_width, int map_height, int bombs_percent, int player_amount)
:
is_all_set(false),
width(map_width), height(map_height), had_first(false),
bombs(map_width * map_height * bombs_percent / 100.0f),
world(map_width * map_height), clients(player_amount),
data_to_send(sizeof(ServerWorldPacket) + (sizeof(ServerPlayerPacket) * clients.size()) + world.size() + (MAX_CHAT_LINE_LEN + 1)),
start_time(0), generated(false)
{
    /* Bind the server to the default localhost.     */
    /* A specific host address can be specified by   */
    /* enet_address_set_host(&address, "x.x.x.x"); */
    address.host = ENET_HOST_ANY;
    address.port = COMMS_PORT;
    auto h = enet_host_create(&address /* the address to bind the server host to */, 
                               player_amount,
                               2 /* allow up to 2 channels to be used, 0 and 1 */,
                               0 /* assume any amount of incoming bandwidth */,
                               0 /* assume any amount of outgoing bandwidth */);
    host.reset(h);
    h = nullptr;

    init.players = clients.size();
    init.width = width;
    init.height = height;
    init.bombs = ENET_HOST_TO_NET_16(bombs);
}

void MineServer::update(const float deltatime)
{
    for(auto& c : clients)
    {
        if(!c.connected) continue;

        const enet_uint16 yaw_bytes = ENET_NET_TO_HOST_16(c.doing.yaw);
        int16_t yaw_int = 0;
        memcpy(&yaw_int, &yaw_bytes, sizeof(int16_t));
        c.data.yaw = yaw_int;

        const enet_uint16 pitch_bytes = ENET_NET_TO_HOST_16(c.doing.pitch);
        int16_t pitch_int = 0;
        memcpy(&pitch_int, &pitch_bytes, sizeof(int16_t));
        c.data.pitch = pitch_int;

        c.data.position[0] = ENET_NET_TO_HOST_32(c.doing.x) / POS_SCALE;
        c.data.position[2] = ENET_NET_TO_HOST_32(c.doing.y) / POS_SCALE;

        if(c.data.position[0] < 0.5f)
        {
            c.data.position[0] = 0.5f;
        }
        else if(c.data.position[0] > width - 0.5f)
        {
            c.data.position[0] = width - 0.5f;
        }

        if(c.data.position[2] < 0.5f)
        {
            c.data.position[2] = 0.5f;
        }
        else if(c.data.position[2] > height - 0.5f)
        {
            c.data.position[2] = height - 0.5f;
        }

        if(c.doing.looking_at_x < 0 || c.doing.looking_at_x >= width)
        {
            c.data.looking_at_x = -1;
        }
        else
        {
            c.data.looking_at_x = c.doing.looking_at_x;
        }

        if(c.doing.looking_at_y < 0 || c.doing.looking_at_y >= height)
        {
            c.data.looking_at_y = -1;
        }
        else
        {
            c.data.looking_at_y = c.doing.looking_at_y;
        }

        if(c.doing.action == 2)
        {
            if(c.data.looking_at_x != -1 && c.data.looking_at_y != -1)
            {
                if(start_time) toggle_flag(world, cur_state.placed_flags, Coord(c.data.looking_at_x, c.data.looking_at_y, width, height));
            }
        }
        else if(c.doing.action == 1)
        {
            if(c.data.looking_at_x != -1 && c.data.looking_at_y != -1)
            {
                MineInfo info{
                    world,
                    width, height,
                    bombs,
                    cur_state.placed_flags
                };
                cur_state.result = reveal(info, generated, start_time, Coord(c.data.looking_at_x, c.data.looking_at_y, width, height));
                if(cur_state.result) return;
            }
        }

        c.doing.action = 0;
    }
}
void MineServer::send_update()
{
    if(start_time)
    {
        const auto delta = time(nullptr);
        const auto d1 = lldiv(delta - start_time, 60);
        cur_state.seconds = d1.rem;
        cur_state.minutes = d1.quot;
    }
    else
    {
        cur_state.seconds = 0;
        cur_state.minutes = 0;
        cur_state.result = 0;
        cur_state.placed_flags = 0;
    }

    auto sc = cur_state;
    sc.placed_flags = ENET_HOST_TO_NET_16(sc.placed_flags);
    memcpy(&data_to_send[0], &sc, sizeof(sc));

    ServerPlayerPacket curdata;
    size_t idx = sizeof(sc);
    for(const auto& c : clients)
    {
        curdata = c.data.fill_info();
        memcpy(&data_to_send[idx], &curdata, sizeof(curdata));
        idx += sizeof(curdata);
    }

    for(auto& t : world)
    {
        data_to_send[idx] = t.prepare();
        t.fresh_visit = false;
        idx += 1;
    }

    if(chatted.size())
    {
        std::copy(chatted.begin(), chatted.end(), data_to_send.begin() + idx);
        idx += chatted.size();
        chatted.clear();
    }

    auto upd_packet(enet_packet_create(data_to_send.data(), idx, ENET_PACKET_FLAG_RELIABLE));
    enet_host_broadcast(host.get(), 1, upd_packet);
    enet_host_flush(host.get());
}

void MineServer::receive()
{
    ENetEvent event;
    while(enet_host_service(host.get(), &event, is_all_set ? 0 : 60000) > 0)
    {
        if(is_all_set)
        {
            switch (event.type)
            {
            case ENET_EVENT_TYPE_CONNECT: {
                // impossible
            } break;
            case ENET_EVENT_TYPE_RECEIVE: {
                auto& c = clients[*(char*)(event.peer->data)];

                ClientPlayerPacket cpp;
                memcpy(&cpp, event.packet->data, sizeof(cpp));

                const auto old_action = c.doing.action;
                const auto old_x = c.doing.looking_at_x;
                const auto old_y = c.doing.looking_at_y;
                c.doing = cpp;
                if(old_action > c.doing.action)
                {
                    c.doing.action = old_action;
                    c.doing.looking_at_x = old_x;
                    c.doing.looking_at_y = old_y;
                }
                if(event.packet->dataLength > sizeof(c.doing))
                {
                    chatted.resize(1 + (event.packet->dataLength - sizeof(c.doing)));
                    chatted[0] = c.idx;
                    memcpy(chatted.data() + 1, event.packet->data + sizeof(c.doing), chatted.size() - 1);
                }

                // Clean up the packet now that we're done using it.
                enet_packet_destroy(event.packet);
            } break;
            case ENET_EVENT_TYPE_DISCONNECT: {
                auto& c = clients[*(char*)(event.peer->data)];
                fprintf(stderr, "Player %d disconnected.\n", c.idx);
                c.connected = false;
                /* Reset the peer's client information. */
                event.peer->data = nullptr;
            } break;
            }
        }
        else
        {
            switch (event.type)
            {
            case ENET_EVENT_TYPE_CONNECT: {
                const auto current = find_not_connected();
                if(current == -1) {
                    fprintf(stderr, "Impossible to connect\n");
                    return;
                }

                auto& c = clients[current];
                c.connected = true;
                c.idx = current;
                fprintf(stderr, "Player %d connected.\n", c.idx);

                init.your_id = c.idx;
                
                auto init_packet(enet_packet_create(&init, sizeof(init), ENET_PACKET_FLAG_RELIABLE));
                enet_peer_send(event.peer, 0, init_packet);

                had_first = true;
                // Store any relevant client information here.
                event.peer->data = &c.idx;
            } break;
            case ENET_EVENT_TYPE_RECEIVE: {
                {
                    auto& c = clients[*(char*)(event.peer->data)];
                    PlayerMetaPacket in;
                    memcpy(&in, event.packet->data, sizeof(in));
                    c.data.fill(in);
                    c.set = true;
                }

                if((is_all_set = all_set()))
                {
                    auto arr = std::make_unique<StartDataPacket[]>(clients.size());
                    size_t idx = 0;
                    for(auto& cli : clients)
                    {
                        fill_pos_and_angle_start(cli.data, idx, clients.size(), width, height);
                        memcpy(&cli.doing.pitch, &cli.data.pitch, 2);
                        memcpy(&cli.doing.yaw, &cli.data.yaw, 2);
                        cli.doing.pitch = ENET_HOST_TO_NET_16(cli.doing.pitch);
                        cli.doing.yaw = ENET_HOST_TO_NET_16(cli.doing.yaw);
                        cli.doing.x = ENET_HOST_TO_NET_32(enet_uint32(cli.data.position[0] * POS_SCALE));
                        cli.doing.y = ENET_HOST_TO_NET_32(enet_uint32(cli.data.position[2] * POS_SCALE));
                        cli.doing.action = 0;

                        arr[idx].info = cli.data.fill_info();
                        arr[idx].meta = cli.data.fill_meta();
                        idx += 1;
                    }

                    auto first_packet = enet_packet_create(arr.get(), sizeof(StartDataPacket) * clients.size(), ENET_PACKET_FLAG_RELIABLE);
                    enet_host_broadcast(host.get(), 0, first_packet);
                }

                // Clean up the packet now that we're done using it.
                enet_packet_destroy(event.packet);
            } break;
            case ENET_EVENT_TYPE_DISCONNECT: { // impossible?
                auto& c = clients[*(char*)(event.peer->data)];
                fprintf(stderr, "Player %d disconnected before start of the game.\n", c.idx);
                c.connected = false;
                c.set = false;
                // Reset the peer's client information.
                event.peer->data = nullptr;
            } break;
            }
        }
    }
}

bool MineServer::all_set() const
{
    return std::all_of(clients.begin(), clients.end(), [](const ServClient& cli) {
        return cli.set; 
    });
}
bool MineServer::should_shutdown() const
{
    return had_first && std::none_of(clients.begin(), clients.end(), [](const ServClient& cli) {
        return cli.connected; 
    });
}
int MineServer::find_not_connected()
{
    int idx = 0;
    for(const auto& c : clients)
    {
        if(!c.connected) return idx;
        idx += 1;
    }
    return -1;
}
