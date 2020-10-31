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

    MineClient(const char * server_addr, const char* skinpath, Texture& default_skin, const std::array<float, 4>& c_c, const char* un);

    void set_server_peer(ENetPeer* p);
    void receive_packet(unsigned char* data, size_t length, std::vector<std::unique_ptr<char[]>>& out_chat);
    void disconnect(bool change_state);
    void cancel();

    void handle_events(GLFWwindow* window, float mouse_sensitivity, int display_w, int display_h, bool& in_esc_menu, bool& released_esc, bool& is_typing, const float deltaTime);
    void render(RenderInfo& info);
    void send();

    State get_state() const;

    ENetHostPtr host;
    // to receive every frame
    ServerWorldPacket sc_packet;

private:
    void update_counters(enet_uint16 new_bombs, enet_uint16 new_flags, unsigned char new_seconds, unsigned char new_minutes);
    glm::mat4 get_view_matrix();
    glm::mat4 get_top_view_matrix();
    void render_world();

    Texture& default_skin_tex;
    Framebuffer minimap_frame, chat_frame;
    Buffer minimap_behind_buf, indicator_buf;
    Buffer chat_buf, chat_visible_buf;
    Buffer minimap_buf, overlay_buf, crosshair_buf, counters_buf, cursor_buf;
    Buffer player_head_buf, player_body_buf, player_arm_left_buf, player_arm_right_buf, player_leg_left_buf, player_leg_right_buf;
    Buffer player_helm_buf, player_coat_buf, player_sleeve_left_buf, player_sleeve_right_buf, player_pant_left_buf, player_pant_right_buf;
    ENetPeer* peer;
    ENetAddress address;

    // state
    State current_state;
    bool pressed_m1;
    bool pressed_m2;
    bool first_mouse;
    float prevx, prevy;
    bool send_str;
    std::vector<unsigned char> skin_bytes;

    // to send at connection
    std::array<float, 4> my_crosshair_color;
    const char* username;

    // to receive at connection
    unsigned char width, height;
    enet_uint16 total_bombs;
    std::vector<unsigned char> world;
    std::unique_ptr<Buffer> wall_buf, lower_world_buf, upper_world_buf;
    std::vector<PlayerData> players;
    std::vector<std::unique_ptr<Texture>> skins;
    std::vector<Buffer> player_names_buf;
    unsigned char my_player_id;

    // to send every frame
    ClientPlayerPacket cs_packet;
};
