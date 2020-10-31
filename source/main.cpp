#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS

#ifdef __SWITCH__
#include <switch.h>
#endif

#include <enet/enet.h>
#include <glm/gtc/matrix_transform.hpp>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <tuple>
#include <array>
#include <algorithm>
extern "C" {
    #include <sys/stat.h>
}
#ifdef __MINGW32__
#include "mingw.thread.h"
#else
#include <thread>
#endif
#include <chrono>

#include "graphics_includes.h"

#include "globjects.h"
#include "focus.h"
#include "game_limits.h"
#include "server.h"
#include "client.h"

#include "globjects.h"
#include "icon.png.h"
#include "base_skin.png.h"
#include "spritesheet.png.h"
#include "shader.h"
#include "shader_fsh.glsl.h"
#include "flat_shader_vsh.glsl.h"
#include "world_shader_vsh.glsl.h"

inline constexpr std::string_view CONFIG_VERSION = "v02";

void glCheckError_(const char *file, int line)
{
    int errorCode;
    while ((errorCode = glGetError()) != GL_NO_ERROR)
    {
        switch (errorCode)
        {
            case GL_INVALID_ENUM:                  fprintf(stderr, "%s", "INVALID_ENUM"); break;
            case GL_INVALID_VALUE:                 fprintf(stderr, "%s", "INVALID_VALUE"); break;
            case GL_INVALID_OPERATION:             fprintf(stderr, "%s", "INVALID_OPERATION"); break;
            case GL_STACK_OVERFLOW:                fprintf(stderr, "%s", "STACK_OVERFLOW"); break;
            case GL_STACK_UNDERFLOW:               fprintf(stderr, "%s", "STACK_UNDERFLOW"); break;
            case GL_OUT_OF_MEMORY:                 fprintf(stderr, "%s", "OUT_OF_MEMORY"); break;
            case GL_INVALID_FRAMEBUFFER_OPERATION: fprintf(stderr, "%s", "INVALID_FRAMEBUFFER_OPERATION"); break;
        }
        fprintf(stderr,  " | %s (%d)\n", file, line);
    }
}

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

static bool check_username_char_valid(const char c)
{
    return (c == '_' || c== ' ' || (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'));
}
static int username_callback(ImGuiInputTextCallbackData* data)
{
    if(auto c = data->EventChar; !check_username_char_valid(c))
    {
        return 1;
    }
    return 0;
}

static void server_thread_func(std::unique_ptr<MineServer>&& srv_ptr)
{
    std::unique_ptr<MineServer> server = std::move(srv_ptr);
    const auto first_upd = std::chrono::steady_clock::now();
    auto last_upd = first_upd;
    bool first_tick = true;
    while(!server->should_shutdown())
    {
        if(server->is_all_set) {
            auto now = std::chrono::steady_clock::now();
            if(first_tick)
            {
                last_upd = now;
                first_tick = false;
            }

            if(const float deltaTime = std::chrono::duration<float>{now - last_upd}.count(); deltaTime >= TIME_PER_TICK * 2.0f)
            {
                server->update(deltaTime);
                server->send_update();
                last_upd = now;
            }
        }

        server->receive();
    }
}

struct WindowDeleter {
    void operator()(GLFWwindow* w)
    {
        glfwDestroyWindow(w);
    }
};
using WindowPtr = std::unique_ptr<GLFWwindow, WindowDeleter>;

static void do_graphical(std::string filepath, const char* skinpath)
{
    GLFWmonitor* primary = glfwGetPrimaryMonitor();
    int total_resolutions;
    const GLFWvidmode* m = glfwGetVideoModes(primary, &total_resolutions);

    std::vector<std::pair<int, int>> resolution_results;
    for(auto mp = m; mp != m + total_resolutions; ++mp)
    {
        const auto& m = *mp;
        auto p = std::make_pair(m.width, m.height);
        if(std::find(resolution_results.cbegin(), resolution_results.cend(), p) == resolution_results.cend())
        {
            resolution_results.push_back(std::move(p));
        }
    }
    total_resolutions = resolution_results.size();

    std::sort(resolution_results.begin(), resolution_results.end());
    const std::string possible_resolutions_str = [&]() {
        std::string s;
        for(const auto& [w, h] : resolution_results)
        {
            s += std::to_string(w) + "x" + std::to_string(h) + '\0';
        }
        return s;
    }();
    const std::vector<const char*> possible_resolutions = [&]() {
        std::vector<const char*> v;
        std::string_view sv{possible_resolutions_str};
        for(int i = 0; i < total_resolutions; ++i)
        {
            v.push_back(sv.data());
            sv.remove_prefix(sv.find('\0', 3) + 1);
        }
        return v;
    }();
    int current_resolution = 0;

    // Decide GL+GLSL versions
    // GL 3.0 + GLSL 330
    #ifndef __SWITCH__
    const char* glsl_version = "#version 330 core";
    #else
    const char* glsl_version = "#version 300 es";
    #endif
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    const auto [initial_w, initial_h] = resolution_results[current_resolution];
    // Create window with graphics context
    WindowPtr window_holder(glfwCreateWindow(initial_w, initial_h, "MinesweeperFPS v1.2", nullptr, nullptr));
    if(!window_holder)
        return;

    auto window = window_holder.get();
    glfwMakeContextCurrent(window);

    // Initialize OpenGL loader
    bool err = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress) == 0;
    if(err)
    {
        fprintf(stderr, "Failed to initialize OpenGL loader!\n");
        return;
    }

    glfwSwapInterval(1); // Enable vsync

    GLFWimage icon_image;
    const auto& [icon_image_width, icon_image_height, icon_image_pixels] = Images::icon;
    auto icon_pixels = std::make_unique<unsigned char[]>(icon_image_pixels.size());
    memcpy(icon_pixels.get(), icon_image_pixels.data(), icon_image_pixels.size());
    std::tie(icon_image.width, icon_image.height, icon_image.pixels) = std::tuple(icon_image_width, icon_image_height, icon_pixels.get());
    glfwSetWindowIcon(window, 1, &icon_image);

    glfwSetWindowFocusCallback(window, Focus::callback);
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GLFW_TRUE);
    glfwSetInputMode(window, GLFW_STICKY_MOUSE_BUTTONS, GLFW_TRUE);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer bindings
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Our state
    {
    constexpr std::string_view world_vsh_shader(Shaders::world_shader_vsh.data(), Shaders::world_shader_vsh.size());
    constexpr std::string_view flat_vsh_shader(Shaders::flat_shader_vsh.data(), Shaders::flat_shader_vsh.size());
    constexpr std::string_view fsh_shader(Shaders::shader_fsh.data(), Shaders::shader_fsh.size());
    Shader worldShader(world_vsh_shader, fsh_shader);
    Shader flatShader(flat_vsh_shader, fsh_shader);

    const auto& [tex_width, tex_height, tex_data] = Images::spritesheet;
    glActiveTexture(GL_TEXTURE0);
    Texture spritesheet(tex_width, tex_height, tex_data.data());
    const auto& [skin_width, skin_height, skin_data] = Images::base_skin;
    Texture default_skin(skin_width, skin_height, skin_data.data());

    flatShader.use();
    flatShader.setInt("texture1", 0);
    worldShader.use();
    worldShader.setInt("texture1", 0);

    char username[MAX_NAME_LEN + 1] = {0};
    const auto set_username = [](char* usr) {
        #ifdef __SWITCH__
        Result rc = accountInitialize(AccountServiceType_Application);
        if(R_FAILED(rc)) return;

        AccountUid userID={0};
        AccountProfile profile;
        AccountUserData userdata;
        AccountProfileBase profilebase;

        rc = accountGetPreselectedUser(&userID);
        if(R_SUCCEEDED(rc))
        {
            rc = accountGetProfile(&profile, userID);
            if(R_SUCCEEDED(rc))
            {
                rc = accountProfileGet(&profile, &userdata, &profilebase);//userdata is otional, see libnx acc.h.
                if(R_SUCCEEDED(rc))
                {
                    for(size_t i = 0, o = 0; i < MAX_NAME_LEN; ++i)
                    {
                        const char c = profilebase.nickname[i];
                        if(c & 0x80) // remove all utf-8 characters, only ascii here
                        {
                            if(c & 0x40)
                            {
                                ++i;
                                if(c & 0x20)
                                {
                                    ++i;
                                    if(c & 0x10)
                                    {
                                        ++i;
                                    }
                                }
                            }
                        }
                        else if(check_username_char_valid(c)) // and even then, only a subset of ascii (alphanumeric + _ + space)
                        {
                            usr[o] = c;
                            ++o;
                        }
                    }
                }

                accountProfileClose(&profile);
            }
        }
        accountExit();
        #endif

        if(usr[0] == 0) strncpy(usr, "Player", MAX_NAME_LEN);
    };
    set_username(username);
    char server_address[2048] = {0};

    std::array<float, 4> crosshair_color{{
        0.25f, 0.25f, 0.25f, 0.75f
    }};

    std::thread server_thread;
    std::unique_ptr<MineClient> client;

    enum class MenuScreen {
        Main,
        Settings,
        Server,
        CoOpClient,
        SPClient,
        StartingServer,
        StartingLocalServer,
        StartingClient,
        InGame,
        AfterGame,
    };

    MenuScreen screen = MenuScreen::Main;
    const auto main_window_flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse;
    const auto extra_window_flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse;
    bool closed_extra_window = true;
    bool in_esc_menu = false;
    bool is_typing = false;
    bool released_esc = false;
    bool first_frame = false;
    bool start_client = false, start_server = false;
    bool fullscreen = false;

   std::vector<std::unique_ptr<char[]>> out_chat;

    float mouse_sensitivity = 3.5f;
    int fov = 60;
    int crosshair_distance = 5;
    int crosshair_width = 8;
    int crosshair_length = 20;
    int minimap_scale = 10;

    int overlay_w = 25;
    int overlay_h = 50;

    int map_width = 15, map_height = 15, bombs_percent = 10, player_amount = 2;
    int window_x = 0, window_y = 0;

    float client_start_time = 0.0f;

    bool modified_config = false;
    const auto load_config = [&]() {
        fprintf(stderr, "Attempting to load config from: '%s'\n", filepath.c_str());
        struct stat s;
        if(stat(filepath.c_str(), &s) < 0)
        {
            modified_config = true;
        }
        else if(FILE* fh = fopen(filepath.c_str(), "r"); fh != nullptr)
        {
            auto buf = std::make_unique<char[]>(s.st_size);
            size_t rsize = fread(buf.get(), 1, s.st_size, fh);
            fclose(fh);
            std::string_view sv(buf.get(), rsize);
            if(sv.compare(0, CONFIG_VERSION.size(), CONFIG_VERSION) == 0)
            {
                sv = sv.substr(CONFIG_VERSION.size() + 1); // + 1 for ';'
                while(sv.size() > 0)
                {
                    const char front = sv.front();
                    sv = sv.substr(2);
                    const size_t len = sv.find_first_of(';');
                    const auto val = sv.substr(0, len);

                    constexpr auto convert_to_float = [](auto val) { return std::stof(std::string(val)); };
                    constexpr auto convert_to_int = [](auto val) { return std::stoi(std::string(val)); };
                    switch(front)
                    {
                        case 'n':
                            memset(username, 0, MAX_NAME_LEN);
                            strncpy(username, val.data(), std::min(MAX_NAME_LEN, len));
                            break;
                        #define MAP_TO(constant, out, conv) case constant: out = conv(val); break;
                        MAP_TO('r', crosshair_color[0], convert_to_float)
                        MAP_TO('g', crosshair_color[1], convert_to_float)
                        MAP_TO('b', crosshair_color[2], convert_to_float)
                        MAP_TO('a', crosshair_color[3], convert_to_float)
                        MAP_TO('S', mouse_sensitivity, convert_to_float)
                        MAP_TO('d', crosshair_distance, convert_to_int)
                        MAP_TO('w', crosshair_width, convert_to_int)
                        MAP_TO('l', crosshair_length, convert_to_int)
                        MAP_TO('W', overlay_w, convert_to_int)
                        MAP_TO('H', overlay_h, convert_to_int)
                        MAP_TO('Z', minimap_scale, convert_to_int)
                        MAP_TO('F', fov, convert_to_int)
                        #undef MAP_TO
                        default:
                            break;
                    }
                    sv = sv.substr(len + 1);
                }
            }
        }
        else
        {
            modified_config = true;
        }
    };
    load_config();

    std::string typed_text;
    auto last_int_upd = std::chrono::steady_clock::now();
    auto last_ext_upd = last_int_upd;

    // Main loop
    while(!glfwWindowShouldClose(window))
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        glfwPollEvents();

        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);

        // Start the Dear ImGui frame
        auto start_imgui_frame = [](){
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            ImVec2 center(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
            ImVec2 sz(ImGui::GetIO().DisplaySize.x * 0.75f, ImGui::GetIO().DisplaySize.y * 0.75f);
            ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
            ImGui::SetNextWindowSize(center);
        };
        const MineClient::State st = client ? client->get_state() : MineClient::State::NotConnected;

        const auto now = std::chrono::steady_clock::now();
        const auto lastComm = std::chrono::duration<float>{now - last_ext_upd}.count();
        const auto deltaTime = std::chrono::duration<float>{now - last_int_upd}.count();

        const auto prev_screen = screen;
        if(screen == MenuScreen::InGame)
        {
            if(st != MineClient::State::Playing || lastComm >= TIME_PER_TICK)
            {
                ENetEvent event;
                if(client->host && enet_host_service(client->host.get(), &event, 1))
                {
                    switch(event.type)
                    {
                    case ENET_EVENT_TYPE_CONNECT:
                        // connection succeeded
                        break;
                    case ENET_EVENT_TYPE_RECEIVE:
                        client->receive_packet(event.packet->data, event.packet->dataLength, out_chat);
                        // Clean up the packet now that we're done using it.
                        enet_packet_destroy(event.packet);
                        break;
                    case ENET_EVENT_TYPE_DISCONNECT:
                        client->disconnect(true);
                        break;
                    }
                }
            }

            if(st == MineClient::State::NotConnected)
            {
                start_imgui_frame();

                int timeout = 5 - int(glfwGetTime() - client_start_time);
                if(timeout == 0) client->cancel();

                ImGui::Begin("Waiting for server", nullptr, extra_window_flags);

                ImGui::Text("Please wait for the connection to the server to complete.");
                ImGui::Text("Timeout in %d second%s.", timeout, timeout == 1 ? "" : "s");

                ImGui::End();
            }
            else if(st == MineClient::State::Waiting)
            {
                start_imgui_frame();
                ImGui::Begin("Waiting for players", nullptr, extra_window_flags);

                ImGui::Text("Please wait for the other players to join.");

                ImGui::End();
            }
            else if(st == MineClient::State::Playing)
            {
                auto set_controls_for_game = [&]() -> void {
                    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                };

                if(first_frame)
                {
                    first_frame = false;
                    set_controls_for_game();
                }

                if(in_esc_menu)
                {
                    start_imgui_frame();
                    ImGui::Begin("Pause menu", &closed_extra_window, extra_window_flags);

                    if(ImGui::SliderInt("HUD width", &overlay_w, 5, 45)) modified_config = true;
                    if(ImGui::SliderInt("HUD height", &overlay_h, 10, 90)) modified_config = true;
                    if(ImGui::SliderInt("Minimap zoom", &minimap_scale, 0, 20)) modified_config = true;

                    ImGui::Spacing();
                    if(ImGui::SliderInt("Field of View", &fov, 30, 90)) modified_config = true;
                    ImGui::Spacing();
                    if(ImGui::SliderFloat("Mouse sensitivity", &mouse_sensitivity, 0.5f, 5.0f)) modified_config = true;
                    if(ImGui::SliderInt("Crosshair thickness", &crosshair_width, 4, 16)) modified_config = true;
                    if(ImGui::SliderInt("Crosshair size", &crosshair_length, 8, 35)) modified_config = true;
                    if(ImGui::SliderInt("Crosshair space", &crosshair_distance, 4, 12)) modified_config = true;

                    ImGui::Separator();
                    if(ImGui::Button("Back to game")) in_esc_menu = false;
                    ImGui::SameLine();
                    if(ImGui::Button("Back to main menu")) client->disconnect(true);

                    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

                    ImGui::End();

                    if(!closed_extra_window)
                    {
                        closed_extra_window = true;
                        in_esc_menu = false;
                    }

                    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
                    {
                        if(released_esc)
                        {
                            in_esc_menu = false;
                            released_esc = false;
                        }
                    }
                    else
                    {
                        released_esc = true;
                    }
                    if(!in_esc_menu) set_controls_for_game();
                }

                if(deltaTime >= 1.0f/60.0f)
                {
                    client->handle_events(window, mouse_sensitivity/20.0f, display_w, display_h, in_esc_menu, released_esc, is_typing, deltaTime);
                    if(in_esc_menu)
                    {
                        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                    }
                    last_int_upd = now;
                }
            }
            else
            {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                if(client->host) client->disconnect(false);
                screen = MenuScreen::AfterGame;
            }
        }
        else if(screen == MenuScreen::AfterGame)
        {
            auto exit_lambda = [&]() {
                screen = MenuScreen::Main;
                client = nullptr;
                if(server_thread.joinable()) server_thread.join();
            };

            start_imgui_frame();

            if(st == MineClient::State::Cancelled)
            {
                ImGui::Begin("Your connection was cancelled.", &closed_extra_window, extra_window_flags);

                if(ImGui::Button("Back to main menu")) exit_lambda();
                ImGui::SameLine();
                if(ImGui::Button("Quit game")) glfwSetWindowShouldClose(window, true);

                ImGui::End();
            }
            if(st == MineClient::State::Lost)
            {
                ImGui::Begin("You lost to the mines!", &closed_extra_window, extra_window_flags);

                ImGui::Text("What a shame, you didn't win this time! Better luck on the next one.");
                ImGui::Text("Loss in %02d min%s %02d second%s",
                            client->sc_packet.minutes, client->sc_packet.minutes == 1 ? "" : "s",
                            client->sc_packet.seconds, client->sc_packet.seconds == 1 ? "" : "s");

                ImGui::Separator();
                if(ImGui::Button("Back to main menu")) exit_lambda();
                ImGui::SameLine();
                if(ImGui::Button("Quit game")) glfwSetWindowShouldClose(window, true);

                ImGui::End();
            }
            else if(st == MineClient::State::Won)
            {
                ImGui::Begin("You won!", &closed_extra_window, extra_window_flags);

                ImGui::Text("Congratulations! Maybe challenge yourself with a bigger field?");
                ImGui::Text("Victory in %02d min%s %02d second%s",
                            client->sc_packet.minutes, client->sc_packet.minutes == 1 ? "" : "s",
                            client->sc_packet.seconds, client->sc_packet.seconds == 1 ? "" : "s");

                ImGui::Separator();
                if(ImGui::Button("Back to main menu")) exit_lambda();
                ImGui::SameLine();
                if(ImGui::Button("Quit game")) glfwSetWindowShouldClose(window, true);

                ImGui::End();
            }
            else if(st == MineClient::State::Disconnected)
            {
                ImGui::Begin("You were disconnected.", &closed_extra_window, extra_window_flags);

                ImGui::Text("You were disconnected from the server.");

                ImGui::Separator();
                if(ImGui::Button("Back to main menu")) exit_lambda();
                ImGui::SameLine();
                if(ImGui::Button("Quit game")) glfwSetWindowShouldClose(window, true);

                ImGui::End();
            }

            if(!closed_extra_window)
            {
                closed_extra_window = true;
                client = nullptr;
                exit_lambda();
            }
        }
        else
        {
            if(start_client)
            {
                size_t l = strnlen(username, MAX_NAME_LEN);
                if(l < MAX_NAME_LEN) memset(username + l, 0, MAX_NAME_LEN - l);
                client = std::make_unique<MineClient>(server_address, skinpath, default_skin, crosshair_color, username);
                client_start_time = glfwGetTime();
                start_client = false;
                in_esc_menu = false;
                first_frame = true;
                screen = MenuScreen::InGame;
            }

            if(start_server)
            {
                server_thread = std::thread(server_thread_func, std::make_unique<MineServer>(map_width, map_height, bombs_percent, player_amount));
                std::this_thread::sleep_for(std::chrono::milliseconds(250));

                std::fill(std::begin(server_address), std::end(server_address), '\0');
                player_amount = player_amount == 1 ? 2 : player_amount;
                start_server = false;
                screen = MenuScreen::StartingClient;
            }

            if(screen != MenuScreen::InGame) start_imgui_frame();

            if(screen == MenuScreen::Main)
            {
                ImGui::Begin("Welcome to MinesweeperFPS v1.1!", nullptr, main_window_flags);

                ImGui::Text("This is a clone of Minesweeper, where you play in a first person view!");
                ImGui::Text("It also supports playing with friends, to make large maps easier.");

                ImGui::Separator();

                if(ImGui::Button("Edit settings")) screen = MenuScreen::Settings;
                if(ImGui::Button("Play singleplayer")) screen = MenuScreen::SPClient;
                if(ImGui::Button("Host coop server")) screen = MenuScreen::Server;
                if(ImGui::Button("Join coop server")) screen = MenuScreen::CoOpClient;

                ImGui::Separator();
                if(ImGui::Button("Quit game")) glfwSetWindowShouldClose(window, true);

                ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

                ImGui::End();
            }
            else if(screen == MenuScreen::Settings)
            {
                ImGui::Begin("Settings", &closed_extra_window, extra_window_flags);

                ImGui::Text("Player settings");
                if(ImGui::InputText("Player name", username, MAX_NAME_LEN, ImGuiInputTextFlags_CallbackCharFilter, username_callback))
                {
                    modified_config = true;
                }
                if(ImGui::ColorEdit4("Crosshair color", crosshair_color.data(), ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreview))
                {
                    modified_config = true;
                }

                if(total_resolutions > 1)
                {
                    ImGui::Spacing();
                    ImGui::Text("Graphics settings");
                    if(ImGui::Checkbox("Fullscreen", &fullscreen))
                    {
                        if(fullscreen)
                        {
                            GLFWmonitor* monitor = glfwGetPrimaryMonitor();
                            const GLFWvidmode* mode = glfwGetVideoMode(monitor);
                            glfwGetWindowPos(window, &window_x, &window_y);
                            glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, 60);
                        }
                        else
                        {
                            const auto& [resolution_x, resolution_y] = resolution_results[current_resolution];
                            glfwSetWindowMonitor(window, nullptr, window_x, window_y, resolution_x, resolution_y, 60);
                        }
                    }

                    if(!fullscreen)
                    {
                        if(ImGui::Combo("Resolution", &current_resolution, possible_resolutions.data(), total_resolutions))
                        {
                            const auto& [resolution_x, resolution_y] = resolution_results[current_resolution];
                            glfwSetWindowSize(window, resolution_x, resolution_y);
                        }
                    }
                }

                ImGui::Separator();
                if(ImGui::Button("Back")) screen = MenuScreen::Main;

                ImGui::End();
            }
            else if(screen == MenuScreen::Server)
            {
                ImGui::Begin("Host", &closed_extra_window, extra_window_flags);

                ImGui::SliderInt("Map width", &map_width, Limits::Min::Width, Limits::Max::Width);
                ImGui::SliderInt("Map height", &map_height, Limits::Min::Height, Limits::Max::Height);
                ImGui::SliderInt("Bomb %", &bombs_percent, Limits::Min::BombPercent, Limits::Max::BombPercent);

                ImGui::Spacing();
                ImGui::SliderInt("Players", &player_amount, Limits::Min::Players, Limits::Max::Players);

                ImGui::Separator();
                if(ImGui::Button("Start")) screen = MenuScreen::StartingServer;
                ImGui::SameLine();
                if(ImGui::Button("Back")) screen = MenuScreen::Main;

                ImGui::End();
            }
            else if(screen == MenuScreen::CoOpClient)
            {
                ImGui::Begin("Join", &closed_extra_window, extra_window_flags);

                ImGui::InputText("Server address", server_address, IM_ARRAYSIZE(server_address));

                ImGui::Separator();
                if(ImGui::Button("Join")) screen = MenuScreen::StartingClient;
                ImGui::SameLine();
                if(ImGui::Button("Back")) screen = MenuScreen::Main;

                ImGui::End();
            }
            else if(screen == MenuScreen::SPClient)
            {
                ImGui::Begin("Singleplayer", &closed_extra_window, extra_window_flags);

                ImGui::SliderInt("Map width", &map_width, Limits::Min::Width, Limits::Max::Width);
                ImGui::SliderInt("Map height", &map_height, Limits::Min::Height, Limits::Max::Height);
                ImGui::SliderInt("Bomb %", &bombs_percent, Limits::Min::BombPercent, Limits::Max::BombPercent);

                ImGui::Separator();
                if(ImGui::Button("Start")) screen = MenuScreen::StartingLocalServer;
                ImGui::SameLine();
                if(ImGui::Button("Back")) screen = MenuScreen::Main;

                ImGui::End();
            }
            else if(screen == MenuScreen::StartingServer)
            {
                ImGui::Begin("Starting co-op server", nullptr, main_window_flags);
                ImGui::Text("Please wait...");
                ImGui::End();

                start_server = true;
            }
            else if(screen == MenuScreen::StartingLocalServer)
            {
                ImGui::Begin("Starting local server", nullptr, main_window_flags);
                ImGui::Text("Please wait...");
                ImGui::End();

                player_amount = 1;
                start_server = true;
            }
            else if(screen == MenuScreen::StartingClient)
            {
                ImGui::Begin("Starting client", nullptr, main_window_flags);
                ImGui::Text("Please wait...");
                ImGui::End();

                start_client = true;
            }

            if(!closed_extra_window)
            {
                screen = MenuScreen::Main;
                closed_extra_window = true;
            }
        }

        glViewport(0, 0, display_w, display_h);
        glClearColor(0.0f, 148.0f/255.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if(client && st == MineClient::State::Playing)
        {
            MineClient::RenderInfo info{
                worldShader, flatShader, spritesheet, display_w, display_h,
                crosshair_distance, crosshair_width, crosshair_length,
                minimap_scale,
                overlay_w, overlay_h,
                fov
            };
            client->render(info);
            if(lastComm >= (TIME_PER_TICK * 2.0f))
            {
                client->send();
                last_ext_upd = now;
            }
        }

        if((screen == MenuScreen::AfterGame && prev_screen == screen) || (screen != MenuScreen::InGame && screen != MenuScreen::AfterGame) || st != MineClient::State::Playing || in_esc_menu)
        {
            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        }

        glfwSwapBuffers(window);
    }

    if(server_thread.joinable()) server_thread.join();

    if(modified_config)
    {
        if(FILE* fh = fopen(filepath.c_str(), "w"); fh != nullptr)
        {
            std::string to_write(CONFIG_VERSION.data(), CONFIG_VERSION.size());
            to_write += ';';

            #define ADD(constant, in) { \
                to_write += constant; \
                to_write += ':'; \
                to_write += in; \
                to_write += ';'; \
            }
            #define MAP_TO(constant, in) ADD(constant, std::to_string(in))

            ADD('n', username)
            MAP_TO('r', crosshair_color[0])
            MAP_TO('g', crosshair_color[1])
            MAP_TO('b', crosshair_color[2])
            MAP_TO('a', crosshair_color[3])
            MAP_TO('S', mouse_sensitivity)
            MAP_TO('d', crosshair_distance)
            MAP_TO('w', crosshair_width)
            MAP_TO('l', crosshair_length)
            MAP_TO('W', overlay_w)
            MAP_TO('H', overlay_h)
            MAP_TO('Z', minimap_scale)
            MAP_TO('F', fov)

            #undef MAP_TO
            #undef ADD

            fwrite(to_write.c_str(), 1, to_write.size(), fh);
            fclose(fh);
        }
    }
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

#ifndef __SWITCH__
static void do_server_alone(char** args)
{
    const char* width_a = args[0];
    const int width = atoi(width_a);
    if(Limits::Max::Width < width || width < Limits::Min::Width) return;

    const char* height_a = args[1];
    const int height = atoi(height_a);
    if(Limits::Max::Height < height || height < Limits::Min::Height) return;

    const char* bombs_a = args[2];
    const int bombs = atoi(bombs_a);
    if(Limits::Max::BombPercent < bombs || bombs < Limits::Min::BombPercent) return;
    
    const char* players_a = args[3];
    const int players = atoi(players_a);
    if(Limits::Max::Players < players || players < Limits::Min::Players) return;

    printf("Starting server\n - width: %d\n - height: %d\n - bombs %%: %d\n - players: %d\n", width, height, bombs, players);
    server_thread_func(std::make_unique<MineServer>(width, height, bombs, players));
    printf("Server stopped.\n");
}
#endif

int main(int argc, char** argv)
{
    if(enet_initialize () != 0)
    {
        fprintf(stderr, "An error occurred while initializing ENet.\n");
        return 1;
    }

    #ifndef __SWITCH__
    if(argc == 6)
    {
        const char* server_indicator = argv[1];
        if(strcmp(server_indicator, "srv") == 0) do_server_alone(argv + 2);
    }
    else if(argc <= 2)
    #endif
    {
        glfwSetErrorCallback(glfw_error_callback);
        if(glfwInit())
        {
            std::string confpath = argv[0];
            do_graphical(confpath + ".cfg", argv[1]);
            glfwTerminate();
        }
    }

    enet_deinitialize();
}
