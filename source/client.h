#pragma once

#include "comms.h"
#include "shader.h"
#include "globjects.h"

#include <GLFW/glfw3.h>
#include <vector>
#include <array>
#include <memory>
#include <chrono>

struct MineClient {
    struct RenderInfo {
        Shader& worldShader;
        Shader& flatShader;
        Texture& spritesheet;
        const int display_w, display_h;
        const int crosshair_distance, crosshair_width, crosshair_length;
        const int minimap_scale;
        const int overlay_w, overlay_h;
        const float fov;
    };

    enum class State : unsigned char {
        NotConnected,
        Waiting,
        Playing,

        // After game
        Cancelled,
        Lost,
        Won,
        Disconnected,
    };

    MineClient(const char * server_addr, const std::array<float, 4>& c_c, const char* un);

    void set_server_peer(ENetPeer* p);
    void receive_packet(unsigned char* data, size_t length, std::vector<std::unique_ptr<char[]>>& out_chat);
    void disconnect(bool change_state);
    void cancel();

    void handle_events(GLFWwindow* window, float mouse_sensitivity, int display_w, int display_h, bool& in_esc_menu, bool& released_esc, bool& is_typing, const float deltaTime);
    void render(RenderInfo& info);
    void send();

    State get_state() const;

    EnetHostPtr host;
    // to receive every frame
    SCPacketData sc_packet;

private:
    void update_counters(enet_uint16 new_bombs, enet_uint16 new_flags, unsigned char new_seconds, unsigned char new_minutes);
    glm::mat4 get_view_matrix();
    glm::mat4 get_top_view_matrix();
    void render_world();

    Framebuffer minimap_frame, chat_frame;
    Buffer minimap_behind_buf, indicator_buf;
    Buffer chat_buf, chat_visible_buf;
    Buffer minimap_buf, overlay_buf, crosshair_buf, counters_buf, cursor_buf, player_buf;
    ENetPeer* peer;
    ENetAddress address;

    // state
    State current_state;
    bool pressed_m1;
    bool pressed_m2;
    bool first_mouse;
    float prevx, prevy;
    bool send_str;

    // to send at connection
    std::array<float, 4> my_crosshair_color;
    const char* username;

    // to receive at connection
    unsigned char width, height;
    enet_uint16 total_bombs;
    std::vector<unsigned char> world;
    std::unique_ptr<Buffer> wall_buf, lower_world_buf, upper_world_buf;
    std::vector<PlayerData> players;
    std::vector<Buffer> player_names_buf;
    unsigned char my_player_id;

    // to send every frame
    CSPacketData cs_packet;
};
