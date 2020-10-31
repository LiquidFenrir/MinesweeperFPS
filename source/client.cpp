#include "client.h"
#include "fillers.h"
#include "focus.h"

#define LODEPNG_NO_COMPILE_ENCODER
#define LODEPNG_NO_COMPILE_DISK
#include "lodepng.h"

#include <cmath>
#include <cstring>
#include <cstdio>
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>

namespace {
    constexpr float min_pitch_to_look = -30.0f;
    constexpr float Overlay_TL_x = -1.0f;
    constexpr float Overlay_TL_y = 1.0f;
    constexpr float MyEpsilon = 0.00006103515625f;
    using UVArr = std::tuple<float, float, float, float>;
    inline const PDD2 plain_color_uv{
        {0.0f, 1.0f},
        {0.0625f, 0.0f},
        {0.0f, 0.125f},
    };
    inline constexpr UVArr transparent_uvs{0.25f, 0.25f, 0.125f, 0.25f};

    inline const glm::vec4 solidWhite{1.0f, 1.0f, 1.0f, 1.0f};
    inline const glm::vec4 solidBlack{0.0f, 0.0f, 0.0f, 1.0f};
    inline const glm::vec4 slightlyBlack{0.0f, 0.0f, 0.0f, 0.75f};
    inline const glm::vec3 outerScaleVec{1.125f, 1.125f, 1.125f};
    inline std::string typed_str;
    inline constexpr size_t MAX_CHAT_LINES = 8;

    void character_callback(GLFWwindow* window, unsigned int codepoint)
    {
        bool lc = (codepoint >= 'a' && codepoint <= 'z');
        bool uc = (codepoint >= 'A' && codepoint <= 'Z');
        bool di = (codepoint >= '0' && codepoint <= '9');
        bool sp = codepoint == ' ';
        if(lc || uc || di || sp)
        {
            if(typed_str.size() < MAX_CHAT_LINE_LEN_TXT)
            {
                typed_str += char(codepoint);
            }
        }
    }

    template<size_t N>
    constexpr std::array<UVArr, N> generate_char_uvs(const std::array<std::pair<int, int>, N>& coords)
    {
        std::array<UVArr, N> out;

        constexpr float delta_u = 0.0625f;
        constexpr float delta_v = 0.125f;
        constexpr float extra_u = 8.0f / 1024.0f;
        constexpr float extra_v = 8.0f / 512.0f;

        for(size_t i = 0; i < N; ++i)
        {
            auto& num = out[i];
            const auto [col, row] = coords[i];
            auto l_u = 0.5f + delta_u * col;
            auto b_v = delta_v * row;
            const auto r_u = l_u + delta_u - extra_u;
            const auto t_v = b_v + delta_v - extra_v;
            l_u += extra_u;
            b_v += extra_v;

            std::get<0>(num) = l_u;
            std::get<1>(num) = t_v;
            std::get<2>(num) = r_u - l_u;
            std::get<3>(num) = t_v - b_v;
        }

        return out;
    }

    template<size_t N>
    constexpr std::array<std::pair<int, int>, N> generate_char_pos(const int start_x, const int start_y)
    {
        int cur_x = start_x;
        int cur_y = start_y;
        std::array<std::pair<int, int>, N> pos;
        for(auto& [x, y] : pos)
        {
            x = cur_x;
            y = cur_y;
            cur_x++;
            if(cur_x == 8)
            {
                cur_y--;
                cur_x = 0;
            }
        }
        return pos;
    }

    constexpr std::array<UVArr, 10> generate_number_uvs()
    {
        return generate_char_uvs<10>(generate_char_pos<10>(4, 1));
    }
    constexpr std::array<UVArr, 26> generate_lowerc_uvs()
    {
        return generate_char_uvs<26>(generate_char_pos<26>(0, 7));
    }
    constexpr std::array<UVArr, 26> generate_upperc_uvs()
    {
        return generate_char_uvs<26>(generate_char_pos<26>(2, 4));
    }

    inline constexpr auto number_uvs_arr = generate_number_uvs();
    inline constexpr auto underscore_uvs_arr = generate_char_uvs<1>({{
        {6, 0},
    }});
    inline constexpr auto colon_uvs_arr = generate_char_uvs<1>({{
        {7, 0},
    }});
    inline constexpr std::array<UVArr, 1> transparent_uvs_arr{{transparent_uvs}};

    inline constexpr auto lowerc_uvs_arr = generate_lowerc_uvs();
    inline constexpr auto upperc_uvs_arr = generate_upperc_uvs();

    void fill_minimap(VertexPtr verts)
    {
        Fillers::fill_quad_generic(verts, 0,
            PDD3{
                {0.0f, 0.0f, 0.0f},
                {1, 0, 0},
                {0.0f, 128.0f/384.0f, 0.0f}
            },
            PDD2{
                {0.0f, 1.0f},
                {1.0f, 0.0f},
                {0.0f, 1.0f},
            },
        solidWhite);
    }
    void fill_chat_visible(VertexPtr verts)
    {
        Fillers::fill_quad_generic(verts, 0,
            PDD3{
                {0.0f, 0.0f, 0.0f},
                {1, 0, 0},
                {0, 1, 0}
            },
            PDD2{
                {0.0f, 1.0f},
                {1.0f, 0.0f},
                {0.0f, 1.0f},
            },
        solidWhite);
    }
    void fill_minimap_behind(VertexPtr verts)
    {
        Fillers::fill_quad_generic(verts, 0, 
            PDD3{
                {-1.0f, 1.0f, 0.0f},
                {2.0f, 0.0f, 0.0f},
                {0.0f, 2.0f, 0.0f}
            },
            PDD2{
                {0.25f, 0.5f},
                {0.125f, 0.0f},
                {0.0f, 0.25f},
            },
            solidWhite
        );
    }
    void fill_overlay(VertexPtr verts)
    {
        Fillers::fill_quad_generic(verts, 0, 
            PDD3{
                {0.0f, 0.0f, 0.0f},
                {1, 0, 0},
                {0, 1, 0}
            },
            PDD2{
                {0.0f, 0.75f},
                {0.125f, 0.0f},
                {0.0f, 0.75f},
            },
            solidWhite
        );
    }
    void fill_cursor(VertexPtr verts)
    {
        Fillers::fill_quad_generic(verts, 0,
            PDD3{
                {0.0f, -1.0f + MyEpsilon * 2.0f, 0.0f},
                {1, 0, 0},
                {0.0f, 0.0f, 1.0f},
            },
            PDD2{
                {0.375f, 1.0f},
                {0.125f, 0.0f},
                {0.0f, 0.25f},
            },
        solidWhite);
    }

    void fill_crosshair(VertexPtr verts)
    {
        Fillers::fill_quad_generic(verts, 0, 
            PDD3{
                {-0.5f, 0.5f, 0.0f},
                {1, 0, 0},
                {0, 1, 0}
            },
            plain_color_uv,
            solidWhite
        );
    }

    void fill_walls(VertexPtr verts, const int width, const int height)
    {
        const float maxX = width;
        const float maxY = height;
        const float minX = 0;
        const float minY = 0;

        const float wall_height = 3.0f;
        const float dZs[2] = {minY, maxY};
        const float dXs[2] = {minX, maxX};
        const PDD2 wall_uvs{
            {0.25f, 0.75f},
            {0.125f, 0.0f},
            {0.0f, 0.25f}
        };

        int idx = 0;
        for(int d = 0; d < 2; d++)
        {
            for(int i = 0; i < width; i++)
            {
                const float fi = i;
                const float x = minX + fi;
                const float z = dZs[d];

                Fillers::fill_quad_generic(verts, idx,
                    PDD3{
                        {x + ((1 - d) * 1) , -1.0f + wall_height, z},
                        {1.0f * ((d * 2) - 1), 0.0f, 0.0f},
                        {0.0f, wall_height, 0.0f},
                    },
                    wall_uvs,
                    solidWhite
                );
                idx++;
            }

            for(int i = 0; i < height; i++)
            {
                const float fi = i;
                const float z = minY + fi;
                const float x = dXs[d];

                Fillers::fill_quad_generic(verts, idx,
                    PDD3{
                        {x, -1.0f + wall_height, z + (d * -1) + (d * 2) },
                        {0.0f, 0.0f, -1.0f * ((d * 2) - 1)},
                        {0.0f, wall_height, 0.0f},
                    },
                    wall_uvs,
                    solidWhite
                );
                idx++;
            }
        }
    }

    void fill_player_part(VertexPtr verts, float u_l_all, float v_t_all, float h, float wz, float wx)
    {
        const float center_u = u_l_all + (wz + wx);
        PDD2 pz_uv{
            {u_l_all + wx, v_t_all - wx},
            {wz, 0.0f},
            {0.0f, h},
        };
        PDD2 mz_uv{
            {center_u + wx, v_t_all - wx},
            {wz, 0.0f},
            {0.0f, h},
        };

        PDD2 px_uv{
            {u_l_all + wx, v_t_all - wx},
            {-wx, 0.0f},
            {0.0f, h},
        };
        PDD2 mx_uv{
            {center_u + wx, v_t_all - wx},
            {-wx, 0.0f},
            {0.0f, h},
        };

        PDD2 py_uv{
            {center_u - wz, v_t_all},
            {wz, 0.0f},
            {0.0f, wx},
        };
        PDD2 my_uv{
            {center_u, v_t_all},
            {wz, 0.0f},
            {0.0f, wx},
        };

        Fillers::fill_quad_generic(verts, 0, // +Z
            PDD3{
                {+0.5f, +0.5f, +0.5f},
                {-1.0f, 0.0f, 0.0f},
                {0.0f, +1.0f, 0.0f}
            },
        mz_uv, solidWhite);
        Fillers::fill_quad_generic(verts, 1, // -X
            PDD3{
                {-0.5f, +0.5f, +0.5f},
                {0.0f, 0.0f, -1.0f},
                {0.0f, +1.0f, 0.0f}
            },
        mx_uv, solidWhite);
        Fillers::fill_quad_generic(verts, 2, // +Y
            PDD3{
                {-0.5f, +0.5f, +0.5f},
                {+1, 0, 0},
                {0.0f, 0.0f, +1.0f}
            },
        py_uv, solidWhite);

        Fillers::fill_quad_generic(verts, 3, // -Z
            PDD3{
                {-0.5f,+0.5f,-0.5f},
                {+1.0f,0.0f,0.0f},
                {0.0f,+1.0f,0.0f}
            },
        pz_uv, solidWhite);
        Fillers::fill_quad_generic(verts, 4, // +X
            PDD3{
                {+0.5f, +0.5f, -0.5f},
                {0.0f,0.0f,+1.0f},
                {0.0f,+1.0f,0.0f}
            },
        px_uv, solidWhite);
        Fillers::fill_quad_generic(verts, 5, // -Y
            PDD3{
                {+0.5f, -0.5f, -0.5f},
                {0.0f,0.0f,+1.0f},
                {+1.0f,0.0f,0.0f}
            },
        my_uv, solidWhite);
    }
    void fill_player(Buffer& head_buf, Buffer& body_buf, Buffer& arm_l_buf, Buffer& arm_r_buf, Buffer& leg_l_buf, Buffer& leg_r_buf)
    {
        fill_player_part(head_buf.getAllVerts(), 0.0f, 1.0f, 1.0f/8.0f, 1.0f/8.0f, 1.0f/8.0f);
        fill_player_part(body_buf.getAllVerts(), 0.25f, 0.75f, 3.0f/16.0f, 2.0f/16.0f, 1.0f/16.0f);

        fill_player_part(arm_l_buf.getAllVerts(), 0.5f, 0.25f, 3.0f/16.0f, 1.0f/16.0f, 1.0f/16.0f);
        fill_player_part(arm_r_buf.getAllVerts(), 0.625f, 0.75f, 3.0f/16.0f, 1.0f/16.0f, 1.0f/16.0f);
        
        fill_player_part(leg_l_buf.getAllVerts(), 0.25f, 0.25f, 3.0f/16.0f, 1.0f/16.0f, 1.0f/16.0f);
        fill_player_part(leg_r_buf.getAllVerts(), 0.0f, 0.75f, 3.0f/16.0f, 1.0f/16.0f, 1.0f/16.0f);
    }
    void fill_player_extra(Buffer& head_buf, Buffer& body_buf, Buffer& arm_l_buf, Buffer& arm_r_buf, Buffer& leg_l_buf, Buffer& leg_r_buf)
    {
        fill_player_part(head_buf.getAllVerts(), 0.5f, 1.0f, 1.0f/8.0f, 1.0f/8.0f, 1.0f/8.0f);
        fill_player_part(body_buf.getAllVerts(), 0.25f, 0.5f, 3.0f/16.0f, 2.0f/16.0f, 1.0f/16.0f);

        fill_player_part(arm_l_buf.getAllVerts(), 0.75f, 0.25f, 3.0f/16.0f, 1.0f/16.0f, 1.0f/16.0f);
        fill_player_part(arm_r_buf.getAllVerts(), 0.625f, 0.5f, 3.0f/16.0f, 1.0f/16.0f, 1.0f/16.0f);
        
        fill_player_part(leg_l_buf.getAllVerts(), 0.0f, 0.25f, 3.0f/16.0f, 1.0f/16.0f, 1.0f/16.0f);
        fill_player_part(leg_r_buf.getAllVerts(), 0.0f, 0.5f, 3.0f/16.0f, 1.0f/16.0f, 1.0f/16.0f);
    }

    void fill_name(VertexPtr verts, std::string_view name)
    {
        constexpr float start_y = 1.25f - 0.0625f;
        constexpr float height = 0.675f/4.0f;
        const float delta_x = 1.0f / 4.0f;
        const float width = name.size() * delta_x;
        const float start_x = (width / 2.0f);

        Fillers::fill_quad_generic(verts, 0, 
            PDD3{
                {start_x - width, start_y, 0.0f},
                {width, 0.0f, 0.0f},
                {0.0f, height, 0.0f}
            },
            plain_color_uv, slightlyBlack);

        size_t idx = 1;
        float x = start_x;

        auto write_char = [&](const int c, const auto& arr) {
            const auto [l_u, t_v, del_u, del_v] = arr[c];
            Fillers::fill_quad_generic(verts, idx, 
                PDD3{
                    {x, start_y, -MyEpsilon},
                    {delta_x, 0.0f, 0.0f},
                    {0.0f, height, 0.0f}
                },
                PDD2{
                    {l_u + del_u, t_v},
                    {-del_u, 0.0f},
                    {0.0f, del_v}
                },
            solidWhite);
        };

        for(char c : name)
        {
            x -= delta_x;
            if(c == '_')
            {
                write_char(0, underscore_uvs_arr);
            }
            else if(c == ' ')
            {
                write_char(0, transparent_uvs_arr);
            }
            else if(c >= '0' && c <= '9')
            {
                write_char(c - '0', number_uvs_arr);
            }
            else if(c >= 'a' && c <= 'z')
            {
                write_char(c - 'a', lowerc_uvs_arr);
            }
            else if(c >= 'A' && c <= 'Z')
            {
                write_char(c - 'A', upperc_uvs_arr);
            }
            idx += 1;
        }
    }
    void fill_counters(VertexPtr verts, const ServerWorldPacket& info, const int bombs, const bool first_time)
    {
        constexpr float timer_y = 128.0f / 384.0f;
        constexpr float delta_y = 50.0f / 384.0f;
        constexpr float start_x = 44.0f / 128.0f;
        constexpr float delta_x = 15.0f / 128.0f;

        constexpr float height = 32.0f / 384.0f;
        constexpr float bombs_y = timer_y + delta_y;
        constexpr float flags_y = bombs_y + delta_y;

        auto write_char = [&](size_t idx, const int* arr_indices, const auto arr_uvs, const int* arr_xs, const size_t cnt, const float y) {
            for(size_t i = 0; i < cnt; ++i)
            {
                const auto [l_u, t_v, del_u, del_v] = arr_uvs[arr_indices[i]];
                const auto x = arr_xs[i] * delta_x;
                Fillers::fill_quad_generic(verts, idx, 
                    PDD3{
                        {start_x + x, -(y + ((delta_y - height)/4.0f)), -MyEpsilon},
                        {delta_x, 0.0f, 0.0f},
                        {0.0f, height, 0.0f}
                    },
                    PDD2{
                        {l_u, t_v},
                        {del_u, 0.0f},
                        {0.0f, del_v}
                    },
                solidBlack);
                idx += 1;
            }
        };

        if(first_time)
        {
            {
                int arr_indices[] = {0};
                int arr_xs[] = {2};
                write_char(0, arr_indices, colon_uvs_arr, arr_xs, 1, timer_y);
            }
            {
                const auto thousands = div(bombs, 1000);
                const auto hundreds = div(thousands.rem, 100);
                const auto tens = div(hundreds.rem, 10);
                int arr_indices[] = {thousands.quot, hundreds.quot, tens.quot, tens.rem};
                int arr_xs[] = {1, 2, 3 ,4};
                write_char(1, arr_indices, number_uvs_arr, arr_xs, 4, bombs_y);
            }
        }

        {
            const auto thousands = div(ENET_NET_TO_HOST_16(info.placed_flags), 1000);
            const auto hundreds = div(thousands.rem, 100);
            const auto tens = div(hundreds.rem, 10);
            int arr_indices[] = {thousands.quot, hundreds.quot, tens.quot, tens.rem};
            int arr_xs[] = {1, 2, 3, 4};
            write_char(5, arr_indices, number_uvs_arr, arr_xs, 4, flags_y);
        }
        {
            const auto mins_tens = div(info.minutes, 10);
            const auto secs_tens = div(info.seconds, 10);
            int arr_indices[] = {mins_tens.quot % 10, mins_tens.rem, secs_tens.quot, secs_tens.rem};
            int arr_xs[] = {0, 1, 3, 4};
            write_char(9, arr_indices, number_uvs_arr, arr_xs, 4, timer_y);
        }
    }
    void fill_indicator(VertexPtr verts)
    {
        Fillers::fill_quad_generic(verts, 0, 
            PDD3{
                {-0.5f, 0.5f, 0.0f},
                {1, 0, 0},
                {0, 1, 0}
            },
            PDD2{
                {0.0625f + (MyEpsilon*2.0f), 1.0f - 0.125f - (MyEpsilon*2.0f)},
                {0.0625f - (MyEpsilon*4.0f), 0.0f},
                {0.0f, 0.125f - (MyEpsilon*4.0f)}
            },
            solidWhite
        );
    }
    
    void fill_chat(VertexPtr verts, const std::vector<std::unique_ptr<char[]>>& lines, const std::vector<PlayerData>& players)
    {
        constexpr float start_y = 1.0f;
        constexpr float height = 2.0f/(MAX_CHAT_LINES + 2);
        const float delta_x = 2.0f/MAX_CHAT_LINE_LEN;
        const float start_x = -1.0f;

        size_t idx = MAX_CHAT_LINE_LEN;
        float x = start_x;
        float y = start_y;

        auto write_char = [&](const int c, const auto& arr, const auto& col) {
            const auto [l_u, t_v, del_u, del_v] = arr[c];
            Fillers::fill_quad_generic(verts, idx, 
                PDD3{
                    {x, y, -MyEpsilon},
                    {delta_x, 0.0f, 0.0f},
                    {0.0f, height, 0.0f}
                },
                PDD2{
                    {l_u, t_v},
                    {del_u, 0.0f},
                    {0.0f, del_v}
                },
            col);
        };

        for(size_t i = MAX_CHAT_LINE_LEN, j = 0; i < verts.count / 6; ++i)
        {
            write_char(0, transparent_uvs_arr, solidWhite);
            idx += 1;

            j++;
            if(j == MAX_CHAT_LINE_LEN)
            {
                x = start_x;
                y -= height;
                j = 0;
            }
            else
            {
                x += delta_x;
            }
        }
        
        x = start_x;
        y = start_y;
        idx = MAX_CHAT_LINE_LEN;
        for(auto& ln : lines)
        {
            const char usernum = ln[0];
            const auto col = players[usernum].color;
            for(size_t i = 0; i < MAX_NAME_LEN; ++i)
            {
                const auto c = players[usernum].username[i];
                if(c == '_')
                {
                    write_char(0, underscore_uvs_arr, col);
                }
                else if(c == ' ')
                {
                    write_char(0, transparent_uvs_arr, col);
                }
                else if(c >= '0' && c <= '9')
                {
                    write_char(c - '0', number_uvs_arr, col);
                }
                else if(c >= 'a' && c <= 'z')
                {
                    write_char(c - 'a', lowerc_uvs_arr, col);
                }
                else if(c >= 'A' && c <= 'Z')
                {
                    write_char(c - 'A', upperc_uvs_arr, col);
                }
                x += delta_x;
                idx += 1;
            }
            
            x = start_x;
            y -= height;
            for(size_t i = 1; i <= MAX_CHAT_LINE_LEN_TXT; ++i)
            {
                const auto c = ln[i];
                if(c == ' ')
                {
                    write_char(0, transparent_uvs_arr, solidWhite);
                }
                else if(c >= '0' && c <= '9')
                {
                    write_char(c - '0', number_uvs_arr, solidWhite);
                }
                else if(c >= 'a' && c <= 'z')
                {
                    write_char(c - 'a', lowerc_uvs_arr, solidWhite);
                }
                else if(c >= 'A' && c <= 'Z')
                {
                    write_char(c - 'A', upperc_uvs_arr, solidWhite);
                }
                x += delta_x;
                idx += 1;
            }

            x = start_x;
            y -= height;
        }

    }

    void fill_typed(VertexPtr verts)
    {
        constexpr float height = 2.0f/(MAX_CHAT_LINES + 2);
        const float delta_x = 2.0f/MAX_CHAT_LINE_LEN;
        const float start_x = -1.0f;

        size_t idx = 0;
        float x = start_x;
        const float y = -1.0f + height * 2.0f;

        auto write_char = [&](const int c, const auto& arr, const auto& col) {
            const auto [l_u, t_v, del_u, del_v] = arr[c];
            Fillers::fill_quad_generic(verts, idx, 
                PDD3{
                    {x, y, -MyEpsilon},
                    {delta_x, 0.0f, 0.0f},
                    {0.0f, height, 0.0f}
                },
                PDD2{
                    {l_u, t_v},
                    {del_u, 0.0f},
                    {0.0f, del_v}
                },
            col);
        };

        for(size_t i = 0; i < MAX_CHAT_LINE_LEN; ++i)
        {
            write_char(0, transparent_uvs_arr, solidWhite);
            x += delta_x;
            idx += 1;
        }

        idx = 0;
        x = start_x;
        for(const auto c : typed_str)
        {
            if(c == ' ')
            {
                write_char(0, transparent_uvs_arr, solidWhite);
            }
            else if(c >= '0' && c <= '9')
            {
                write_char(c - '0', number_uvs_arr, solidWhite);
            }
            else if(c >= 'a' && c <= 'z')
            {
                write_char(c - 'a', lowerc_uvs_arr, solidWhite);
            }
            else if(c >= 'A' && c <= 'Z')
            {
                write_char(c - 'A', upperc_uvs_arr, solidWhite);
            }
            x += delta_x;
            idx += 1;
        }
    }
}

MineClient::MineClient(const char * server_addr, const char* skinpath, Texture& default_skin, const std::array<float, 4>& c_c, const char* un)
:
host(enet_host_create(nullptr, 1, 2, 0, 0)),
default_skin_tex(default_skin),
minimap_frame(256, 256),
chat_frame(MAX_CHAT_LINE_LEN * 32, (MAX_CHAT_LINES + 1) * 32),
minimap_behind_buf(Buffer::Quads(1)),
indicator_buf(Buffer::Quads(1)),
chat_buf(Buffer::Quads((MAX_CHAT_LINE_LEN * MAX_CHAT_LINES) + MAX_CHAT_LINE_LEN)),
chat_visible_buf(Buffer::Quads(1)),
minimap_buf(Buffer::Quads(1)),
overlay_buf(Buffer::Quads(1)),
crosshair_buf(Buffer::Quads(1)),
counters_buf(Buffer::Quads(5 + 4 + 4)),
cursor_buf(Buffer::Quads(1)),
player_head_buf(Buffer::Quads(6)),
player_body_buf(Buffer::Quads(6)),
player_arm_left_buf(Buffer::Quads(6)),
player_arm_right_buf(Buffer::Quads(6)),
player_leg_left_buf(Buffer::Quads(6)),
player_leg_right_buf(Buffer::Quads(6)),
player_helm_buf(Buffer::Quads(6)),
player_coat_buf(Buffer::Quads(6)),
player_sleeve_left_buf(Buffer::Quads(6)),
player_sleeve_right_buf(Buffer::Quads(6)),
player_pant_left_buf(Buffer::Quads(6)),
player_pant_right_buf(Buffer::Quads(6)),
current_state(MineClient::State::NotConnected),
pressed_m1(false),
pressed_m2(false),
first_mouse(true),
send_str(false),
my_crosshair_color(c_c),
username(un)
{
    fill_crosshair(crosshair_buf.getAllVerts());
    fill_cursor(cursor_buf.getAllVerts());
    fill_indicator(indicator_buf.getAllVerts());
    fill_minimap(minimap_buf.getAllVerts());
    fill_minimap_behind(minimap_behind_buf.getAllVerts());
    fill_overlay(overlay_buf.getAllVerts());
    fill_player(player_head_buf, player_body_buf, player_arm_left_buf, player_arm_right_buf, player_leg_left_buf, player_leg_right_buf);
    fill_player_extra(player_helm_buf, player_coat_buf, player_sleeve_left_buf, player_sleeve_right_buf, player_pant_left_buf, player_pant_right_buf);
    fill_chat_visible(chat_visible_buf.getAllVerts());
    fill_typed(chat_buf.getAllVerts());

    if(skinpath == nullptr)
    {
        skin_bytes.resize(sizeof(PlayerMetaPacket));
    }
    else
    {
        FILE* fh = fopen(skinpath, "rb");
        fseek(fh, 0, SEEK_END);
        long bytes_sz = ftell(fh);
        rewind(fh);
        skin_bytes.resize(bytes_sz + sizeof(PlayerMetaPacket));
        fread(skin_bytes.data() + sizeof(PlayerMetaPacket), 1, bytes_sz, fh);
        fclose(fh);
    }

    if(server_addr[0] == '\0')
    {
        enet_address_set_host(&address, "127.0.0.1");
    }
    else
    {
        enet_address_set_host(&address, server_addr);
    }
    address.port = COMMS_PORT;
    peer = enet_host_connect(host.get(), &address, 2, 0);

    cs_packet.action = 0;
}

void MineClient::receive_packet(unsigned char* data, size_t length, std::vector<std::unique_ptr<char[]>>& out_chat)
{
    if(current_state == MineClient::State::NotConnected)
    {
        ServerWorldPacketInit in;
        memcpy(&in, data, sizeof(in));

        players.resize(in.players);
        skins.resize(in.players);
        my_player_id = in.your_id;

        width = in.width;
        height = in.height;
        total_bombs = ENET_NET_TO_HOST_16(in.bombs);

        PlayerMetaPacket out;
        out.cross_r = 255 * my_crosshair_color[0];
        out.cross_g = 255 * my_crosshair_color[1];
        out.cross_b = 255 * my_crosshair_color[2];
        out.cross_a = 255 * my_crosshair_color[3];
        memcpy(out.username, username, sizeof(out.username));
        out.skinbytes = ENET_HOST_TO_NET_32(skin_bytes.size() - sizeof(PlayerMetaPacket));

        memcpy(skin_bytes.data(), &out, sizeof(out));

        auto send_packet(enet_packet_create(skin_bytes.data(), skin_bytes.size(), ENET_PACKET_FLAG_RELIABLE));
        enet_peer_send(peer, 0, send_packet);
        enet_host_flush(host.get());

        current_state = MineClient::State::Waiting;
    }
    else if(current_state == MineClient::State::Waiting)
    {
        size_t offset = 0, player_idx = 0;
        StartDataPacket in;
        std::vector<unsigned char> skin_pixels;
        for(auto& player : players)
        {
            memcpy(&in, data + offset, sizeof(in));
            player.fill(in.info);
            player.fill(in.meta);
            const char* name = in.meta.username;
            const size_t namelen = strnlen(name, MAX_NAME_LEN);
            player_names_buf.push_back(Buffer::Quads(namelen + 1));
            fill_name(player_names_buf.back().getAllVerts(), std::string_view{name, namelen});
            
            offset += sizeof(in);
            enet_uint32 skin_sz = ENET_NET_TO_HOST_32(in.meta.skinbytes);
            if(skin_sz)
            {
                skin_pixels.clear();
                unsigned skin_w = 0, skin_h = 0;
                lodepng::decode(skin_pixels, skin_w, skin_h, data + offset, skin_sz);
                const unsigned w_b = skin_w * 4;
                for(unsigned from_s = 0, from_e = ((skin_h - 1) * w_b); from_s < from_e; from_s += w_b, from_e -= w_b)
                {
                    std::swap_ranges(skin_pixels.begin() + from_e, skin_pixels.begin() + from_e + w_b, skin_pixels.begin() + from_s);
                }
                skins[player_idx] = std::make_unique<Texture>(skin_w, skin_h, skin_pixels.data());
                offset += skin_sz;
            }
            player_idx++;
        }

        wall_buf = std::make_unique<Buffer>(Buffer::Quads((width + height) * 2));
        fill_walls(wall_buf->getAllVerts(), width, height);

        world.resize(width * height, '.' | 0x80);
        lower_world_buf = std::make_unique<Buffer>(Buffer::Quads(width * height));
        upper_world_buf = std::make_unique<Buffer>(Buffer::Quads(width * height));
        render_world();
        fill_counters(counters_buf.getAllVerts(), ServerWorldPacket{0,0,0,0}, total_bombs, true);

        current_state = MineClient::State::Playing;
    }
    else if(current_state == MineClient::State::Playing)
    {
        memcpy(&sc_packet, data, sizeof(sc_packet));
        if(sc_packet.result)
        {
            if(sc_packet.result > 0)
            {
                current_state = MineClient::State::Won;
            }
            else
            {
                current_state = MineClient::State::Lost;
            }
            return;
        }

        size_t offset = sizeof(sc_packet);
        ServerPlayerPacket in;
        unsigned char idx = 0;
        for(auto& player : players)
        {
            memcpy(&in, data + offset, sizeof(in));
            if(idx != my_player_id)
            {
                player.fill(in);
            }
            offset += sizeof(in);
            idx += 1;
        }

        memcpy(world.data(), data + offset, world.size());
        offset += world.size();
        if(offset < length)
        {
            if(out_chat.size() == (MAX_CHAT_LINES / 2))
            {
                std::rotate(out_chat.begin(), out_chat.begin() + 1, out_chat.end());
            }
            else
            {
                out_chat.push_back(std::make_unique<char[]>(MAX_CHAT_LINE_LEN + 2));
            }
            char* write_to = out_chat.back().get();
            memset(write_to, 0, MAX_CHAT_LINE_LEN + 2);
            memcpy(write_to, data + offset, length - offset);
            fill_chat(chat_buf.getAllVerts(), out_chat, players);
        }

        render_world();
        fill_counters(counters_buf.getAllVerts(), sc_packet, 0, false);
    }
}
void MineClient::disconnect(bool change_state)
{
    if(host)
    {
        ENetEvent event;
        bool done = false;

        enet_peer_disconnect(peer, 0);
        while(enet_host_service(host.get(), &event, 1000) > 0)
        {
            switch (event.type)
            {
            case ENET_EVENT_TYPE_RECEIVE:
                enet_packet_destroy(event.packet);
                break;
            case ENET_EVENT_TYPE_DISCONNECT:
                done = true;
                break;
            default:
                break;
            }
        }

        if(!done) enet_peer_reset(peer);
        host.reset();
    }
    if(change_state) current_state = MineClient::State::Disconnected;
}
void MineClient::cancel()
{
    if(host)
    {
        ENetEvent event;
        bool done = false;

        enet_peer_disconnect(peer, 0);
        while(enet_host_service(host.get(), &event, 1000) > 0)
        {
            switch (event.type)
            {
            case ENET_EVENT_TYPE_RECEIVE:
                enet_packet_destroy(event.packet);
                break;
            case ENET_EVENT_TYPE_DISCONNECT:
                done = true;
                break;
            default:
                break;
            }
        }

        if(!done) enet_peer_reset(peer);
        host.reset();
    }
    current_state = MineClient::State::Cancelled;
}

void MineClient::handle_events(GLFWwindow* window, float mouse_sensitivity, int display_w, int display_h, bool& in_esc_menu, bool& released_esc, bool& is_typing, const float deltaTime)
{
    {
    size_t idx = 0;
    for(auto& p : players)
    {
        auto i = idx++;
        if(i == my_player_id) continue;

        if((roundf(p.movedDistance * 100.0f) / 100.0f) == 0.0f)
        {
            if(std::fabs(p.movementSwing) > ((SwingSpeed * 0.25f) * deltaTime))
            {
                p.currentSwingDirection = std::copysign(SwingSpeed, p.movementSwing);
                p.movementSwing -= p.currentSwingDirection * deltaTime;
            }
            if(std::fabs(p.movementSwing) <= ((SwingSpeed * 0.25f) * deltaTime))
            {
                p.currentSwingDirection = SwingSpeed;
                p.movementSwing = 0.0f;
            }
        }
        else
        {
            p.movementSwing += p.currentSwingDirection * deltaTime;
            if(std::fabs(p.movementSwing) >= MaxSwingAmplitude)
            {
                p.currentSwingDirection = -p.currentSwingDirection;
                p.movementSwing = std::copysign(MaxSwingAmplitude, p.movementSwing);
            }
        }
    }
    }

    if(in_esc_menu || !Focus::is_focused) return;

    const float velocity = MovementSpeed * deltaTime;

    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        if(released_esc)
        {
            released_esc = false;
            if(is_typing)
            {
            
                glfwSetCharCallback(window, nullptr);
                is_typing = false;
            }
            else
            {
                in_esc_menu = true;
                first_mouse = true;
            }
            return;
        }
    }
    else
    {
        released_esc = true;
    }

    static bool released_enter = true;
    if(glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS)
    {
        if(released_enter)
        {
            released_enter = false;
            if(is_typing)
            {
                glfwSetCharCallback(window, nullptr);
                is_typing = false;
                send_str = !typed_str.empty();
            }
            else
            {
                is_typing = true;
                glfwSetCharCallback(window, character_callback);
            }
        }
    }
    else
    {
        released_enter = true;
    }
    
    static bool released_bp = true;
    if(glfwGetKey(window, GLFW_KEY_BACKSPACE) == GLFW_PRESS)
    {
        if(released_bp)
        {
            released_bp = false;
            if(typed_str.size())
            {
                typed_str.pop_back();
            }
        }
    }
    else
    {
        released_bp = true;
    }

    static size_t prev_typed_size = 0;
    if(typed_str.size() != prev_typed_size)
    {
        prev_typed_size = typed_str.size();
        fill_typed(chat_buf.getAllVerts());
    }

    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    if(first_mouse)
    {
        first_mouse = false;
        prevx = xpos;
        prevy = ypos;
    }

    if(glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
    {
        if(!pressed_m1)
        {
            pressed_m1 = true;
            cs_packet.action = 1;
        }
    }
    else
    {
        pressed_m1 = false;
    }

    if(glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
    {
        if(!pressed_m2)
        {
            pressed_m2 = true;
            cs_packet.action = 2;
        }
    }
    else
    {
        pressed_m2 = false;
    }

    float xoffset = xpos - prevx;
    float yoffset = prevy - ypos;

    prevx = xpos;
    prevy = ypos;

    float going_towards = 0;
    unsigned char going_mag = 0;

#if GLFW_VERSION_MAJOR >= 2 && GLFW_VERSION_MINOR >= 3
    if(GLFWgamepadstate state; glfwGetGamepadState(GLFW_JOYSTICK_1, &state) == GLFW_TRUE)
    {
        if(state.buttons[GLFW_GAMEPAD_BUTTON_START])
        {
            in_esc_menu = true;
            first_mouse = true;
            return;
        }

        if(state.buttons[GLFW_GAMEPAD_BUTTON_LEFT_BUMPER])
        {
            cs_packet.action = 1;
        }
        if(state.buttons[GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER])
        {
            cs_packet.action = 2;
        }

        #define DEADZONE(ax) ((state.axes[ax] <= 0.0625f/2.0f) ? 0 : state.axes[ax])

        float x_move = DEADZONE(GLFW_GAMEPAD_AXIS_LEFT_X);
        float y_move = DEADZONE(GLFW_GAMEPAD_AXIS_LEFT_Y);

        going_towards = atan2f(-y_move, x_move);
        going_mag = sqrtf((x_move * x_move) + (y_move * y_move)) * 255;

        xoffset = DEADZONE(GLFW_GAMEPAD_AXIS_RIGHT_X) * (display_w/2.0f);
        yoffset = DEADZONE(GLFW_GAMEPAD_AXIS_RIGHT_Y) * (display_h/2.0f);

        #undef DEADZONE
    }
#endif

    auto& playa = players[my_player_id];
    playa.yaw += xoffset * mouse_sensitivity;
    playa.yaw %= 360;
    int16_t yaw = playa.yaw;
    memcpy(&cs_packet.yaw, &yaw, sizeof(int16_t));

    playa.pitch += yoffset * mouse_sensitivity;
    if(fabs(playa.pitch) >= 89.0f)
    {
        playa.pitch = copysign(89.0f, playa.pitch);
    }
    int16_t pitch = playa.pitch;
    memcpy(&cs_packet.pitch, &pitch, sizeof(int16_t));

    if(going_mag == 0 && !is_typing)
    {
        going_mag = 4 | 1;
        if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            going_mag ^= 1;
        else if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            going_mag ^= (2 | 1);

        if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            going_mag ^= 4;
        else if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            going_mag ^= (8 | 4);
        
        if(going_mag != (4 | 1))
        {
            const int forward_movement = -(int(going_mag & 3) - 1);
            const int side_movement = int((going_mag >> 2) & 3) - 1;
            going_towards = atan2f(forward_movement, side_movement);
            going_mag = 255;
        }
        else
        {
            going_mag = 0;
        }
    }

    const auto yaw_rads = glm::radians(float(yaw));
    if(going_mag != 0)
    {
        glm::vec3 Forward{
            cos(yaw_rads),
            0.0f,
            sin(yaw_rads)
        };
        glm::vec3 Right = glm::normalize(glm::cross(Forward, {0, 1, 0}));

        playa.position += velocity * (going_mag / 255.0f) * ((Forward * sinf(going_towards)) + Right * cosf(going_towards));

        if(playa.position[0] < 0.5f)
        {
            playa.position[0] = 0.5f;
        }
        else if(playa.position[0] > width - 0.5f)
        {
            playa.position[0] = width - 0.5f;
        }

        if(playa.position[2] < 0.5f)
        {
            playa.position[2] = 0.5f;
        }
        else if(playa.position[2] > height - 0.5f)
        {
            playa.position[2] = height - 0.5f;
        }
    }

    playa.looking_at_x = -1;
    playa.looking_at_y = -1;

    if(playa.pitch <= min_pitch_to_look)
    {
        const auto pitch_rads = glm::radians(float(playa.pitch));
        const glm::vec3 Front{
            cosf(yaw_rads) * cosf(pitch_rads),
            sinf(pitch_rads),
            sinf(yaw_rads) * cosf(pitch_rads)
        };
        const auto floorNormal = glm::vec3(0, 1, 0);
        const auto floorPos = glm::vec3(0.0f, -1.0f, 0.0f);
        const float d = glm::dot(floorNormal, Front);
        if(d != 0.0f)
        {
            const float dist = glm::dot(floorNormal, floorPos - playa.position) / d;
            auto LookingAt = playa.position + (Front * dist);
            if(!(LookingAt[0] < 0 || LookingAt[0] >= width || LookingAt[2] < 0 || LookingAt[2] >= height))
            {
                playa.looking_at_x = LookingAt[0];
                playa.looking_at_y = LookingAt[2];
            }
        }
    }
}

void MineClient::render(MineClient::RenderInfo& info)
{
    const auto& self = players[my_player_id];

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    // bind textures on corresponding texture units
    info.spritesheet.bind();

    // activate shader
    info.worldShader.use();

    // pass projection matrix to shader (note that in this case it could change every frame)
    glm::mat4 projection = glm::perspective(glm::radians(info.fov), (float)info.display_w / (float)info.display_h, 0.1f, 100.0f);
    info.worldShader.setMat4("projection", projection);

    // camera/view transformation
    glm::mat4 view = get_view_matrix();
    info.worldShader.setMat4("view", view);

    glm::mat4 model = glm::mat4(1.0f);
    info.worldShader.setMat4("model", model);

    info.worldShader.setVec4("constColor", solidWhite);

    wall_buf->bind();
    wall_buf->draw();

    lower_world_buf->bind();
    lower_world_buf->draw();
    upper_world_buf->bind();
    upper_world_buf->draw();

    // player cursor
    cursor_buf.bind();
    for(unsigned char i = 0; i < players.size(); ++i)
    {
        const auto& playa = players[i];
        if(playa.looking_at_x != -1 && playa.looking_at_y != -1)
        {
            info.worldShader.setVec4("constColor", playa.color);
            model = glm::translate(glm::mat4(1.0f), glm::vec3{playa.looking_at_x, 0.0f, playa.looking_at_y + 1});
            info.worldShader.setMat4("model", model);
            cursor_buf.draw();
        }
    }

    for(unsigned char i = 0; i < players.size(); ++i)
    {
        if(i == my_player_id) continue;

        const auto& playa = players[i];
        if(auto& sk = skins[i]; sk)
        {
            info.worldShader.setVec4("constColor", solidWhite);
            sk->bind();
        }
        else
        {
            info.worldShader.setVec4("constColor", playa.color);
            default_skin_tex.bind();
        }

        const auto playerSpinMat = glm::rotate(glm::translate(glm::mat4(1.0f), playa.position), glm::radians(-(float(playa.yaw) + 90.0f)), glm::vec3{0, 1, 0});

        model = glm::translate(playerSpinMat, glm::vec3{0.0f, -0.35f, 0.0f});
        model = glm::scale(model, glm::vec3{0.25f, 0.45f, 0.125f});
        info.worldShader.setMat4("model", model);
        player_body_buf.bind();
        player_body_buf.draw();
        info.worldShader.setMat4("model", glm::scale(model, outerScaleVec));
        player_coat_buf.bind();
        player_coat_buf.draw();

        const auto swingRads = glm::radians(playa.movementSwing);

        model = playerSpinMat;
        auto armMat = model;

        model = armMat;
        model = glm::translate(model, glm::vec3{0, -(0.125 + (0.45f / 2.0f)), 0});
        model = glm::translate(model, glm::vec3{+(0.125f + 0.0625f), 0, 0});
        model = glm::translate(model, glm::vec3{0, sinf(fabs(swingRads)) * (0.0625f * 1.5f), -sinf(swingRads) * 0.25f});
        model = glm::rotate(model, swingRads, glm::vec3{1, 0, 0});
        model = glm::scale(model, glm::vec3{0.125f, 0.45f, 0.125f});
        info.worldShader.setMat4("model", model);
        player_arm_left_buf.bind();
        player_arm_left_buf.draw();
        info.worldShader.setMat4("model", glm::scale(model, outerScaleVec));
        player_sleeve_left_buf.bind();
        player_sleeve_left_buf.draw();
        
        model = armMat;
        model = glm::translate(model, glm::vec3{0, -(0.125 + (0.45f / 2.0f)), 0});
        model = glm::translate(model, glm::vec3{-(0.125f + 0.0625f), 0, 0});
        model = glm::translate(model, glm::vec3{0, sinf(fabs(swingRads)) * (0.0625f * 1.5f), sinf(swingRads) * 0.25f});
        model = glm::rotate(model, -swingRads, glm::vec3{1, 0, 0});
        model = glm::scale(model, glm::vec3{0.125f, 0.45f, 0.125f});
        info.worldShader.setMat4("model", model);
        player_arm_right_buf.bind();
        player_arm_right_buf.draw();
        info.worldShader.setMat4("model", glm::scale(model, outerScaleVec));
        player_sleeve_right_buf.bind();
        player_sleeve_right_buf.draw();

        model = playerSpinMat;
        auto legMat = model;

        model = legMat;
        model = glm::translate(model, glm::vec3{0, -(0.125 + 0.45f + (0.425f / 2.0f)), 0});
        model = glm::translate(model, glm::vec3{+0.0625f, 0, 0});
        model = glm::translate(model, glm::vec3{0, sinf(fabs(swingRads)) * (0.0625f * 1.5f), sinf(swingRads) * 0.25f});
        model = glm::rotate(model, -swingRads, glm::vec3{1, 0, 0});
        model = glm::scale(model, glm::vec3{0.125f, 0.425f, 0.125f});
        info.worldShader.setMat4("model", model);
        player_leg_left_buf.bind();
        player_leg_left_buf.draw();
        info.worldShader.setMat4("model", glm::scale(model, outerScaleVec));
        player_pant_left_buf.bind();
        player_pant_left_buf.draw();
        
        model = legMat;
        model = glm::translate(model, glm::vec3{0, -(0.125 + 0.45f + (0.425f / 2.0f)), 0});
        model = glm::translate(model, glm::vec3{-0.0625f, 0, 0});
        model = glm::translate(model, glm::vec3{0, sinf(fabs(swingRads)) * (0.0625f * 1.5f), -sinf(swingRads) * 0.25f});
        model = glm::rotate(model, swingRads, glm::vec3{1, 0, 0});
        model = glm::scale(model, glm::vec3{0.125f, 0.425f, 0.125f});
        info.worldShader.setMat4("model", model);
        player_leg_right_buf.bind();
        player_leg_right_buf.draw();
        info.worldShader.setMat4("model", glm::scale(model, outerScaleVec));
        player_pant_right_buf.bind();
        player_pant_right_buf.draw();

        model = glm::rotate(playerSpinMat, glm::radians(float(playa.pitch)), glm::vec3{1, 0, 0});
        model = glm::scale(model, glm::vec3{0.25f, 0.25f, 0.25f});
        info.worldShader.setMat4("model", model);
        player_head_buf.bind();
        player_head_buf.draw();
        info.worldShader.setMat4("model", glm::scale(model, outerScaleVec));
        player_helm_buf.bind();
        player_helm_buf.draw();
    }

    info.spritesheet.bind();

    info.worldShader.setVec4("constColor", solidWhite);

    for(unsigned char i = 0; i < players.size(); ++i)
    {
        if(i == my_player_id) continue;

        const auto& playa = players[i];
        auto& buf = player_names_buf[i];
        model = glm::inverse(glm::lookAt(playa.position, self.position, glm::vec3(0, 1, 0)));
        info.worldShader.setMat4("model", model);
        buf.bind();
        buf.draw();
    }

    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    info.flatShader.use();
    info.flatShader.setMat4("model", glm::mat4(1.0f));
    info.flatShader.setVec4("constColor", solidWhite);

    chat_frame.bind();
    glClearColor(0.0f, 0.0f, 0.0f, 0.5f);
    glClear(GL_COLOR_BUFFER_BIT);

    chat_buf.bind();
    chat_buf.draw();

    minimap_frame.bind();
    glClearColor(0, 1, 0, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    minimap_behind_buf.bind();
    minimap_behind_buf.draw();

    glm::mat4 top_view = get_top_view_matrix();

    const float minimap_scale = (info.minimap_scale * 2 + 30)/200.0f;
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3{
        self.position[0] - (self.position[0] * minimap_scale),
        0.0,
        self.position[2] - (self.position[2] * minimap_scale)
    });
    model = glm::scale(model, glm::vec3{minimap_scale, 1.0f, minimap_scale});
    info.flatShader.setMat4("model", top_view * model);

    lower_world_buf->bind();
    lower_world_buf->draw();
    upper_world_buf->bind();
    upper_world_buf->draw();

    indicator_buf.bind();
    for(unsigned char i = 0; i < players.size(); ++i)
    {
        const auto& playa = players[i];
        glm::vec4 col = playa.color;
        col[3] = 1.0f;
        info.flatShader.setVec4("constColor", col);
        model = glm::translate(glm::mat4(1.0f), glm::vec3{
            (self.position[0] - players[i].position[0]) * minimap_scale,
            (players[i].position[2] - self.position[2]) * minimap_scale,
            0.0f
        });
        model = glm::rotate(model, glm::radians(-(float(playa.yaw) + 180.0f)), glm::vec3{0.0f, 0.0f, 1.0f});
        model = glm::scale(model,  glm::vec3(0.25f + 0.0625f));
        info.flatShader.setMat4("model", model);
        indicator_buf.draw();
    }

    info.flatShader.setVec4("constColor", solidWhite);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, info.display_w, info.display_h);

    chat_frame.bindOutput();

    model = glm::translate(glm::mat4(1.0f), glm::vec3{1.0f - (info.overlay_w / 50.0f), -1.0f + ((info.overlay_h * 50.0f) / (50.0f * 50.0f)), 0.0f});
    model = glm::scale(model, glm::vec3{info.overlay_w / 50.0f, ((info.overlay_h * 50.0f) / (50.0f * 50.0f)), 1.0f});
    info.flatShader.setMat4("model", model);

    chat_visible_buf.bind();
    chat_visible_buf.draw();

    minimap_frame.bindOutput();

    model = glm::translate(glm::mat4(1.0f), glm::vec3{Overlay_TL_x, Overlay_TL_y, 0.0f});
    model = glm::scale(model, glm::vec3{info.overlay_w / 50.0f, (info.overlay_h * 75.0f) / (50.0f * 50.0f), 1.0f});
    info.flatShader.setMat4("model", model);

    minimap_buf.bind();
    minimap_buf.draw();

    info.spritesheet.bind();

    overlay_buf.bind();
    overlay_buf.draw();

    counters_buf.bind();
    counters_buf.draw();

    const float crosshair_size_y = info.crosshair_width / float(info.display_h);
    const float crosshair_size_x = info.crosshair_width / float(info.display_w);
    const float crosshair_length_y = info.crosshair_length * 2.0f / info.display_h;
    const float crosshair_length_x = info.crosshair_length * 2.0f / info.display_w;
    const float crosshair_distance_from_center_y = info.crosshair_distance * 2.0f / info.display_h;
    const float crosshair_distance_from_center_x = info.crosshair_distance * 2.0f / info.display_w;

    crosshair_buf.bind();

    model = glm::mat4(1.0f);
    const auto model_pos_vert_u = glm::translate(model, glm::vec3{0.0f, +(crosshair_distance_from_center_y + crosshair_length_y/2.0f), 0.0f});
    const auto model_pos_vert_d = glm::translate(model, glm::vec3{0.0f, -(crosshair_distance_from_center_y + crosshair_length_y/2.0f), 0.0f});
    const auto model_pos_hori_r = glm::translate(model, glm::vec3{+(crosshair_distance_from_center_x + crosshair_length_x/2.0f), 0.0f, 0.0f});
    const auto model_pos_hori_l = glm::translate(model, glm::vec3{-(crosshair_distance_from_center_x + crosshair_length_x/2.0f), 0.0f, 0.0f});

    info.flatShader.setVec4("constColor", self.color);

    model = glm::scale(model_pos_vert_u, glm::vec3{crosshair_size_x, crosshair_length_y, 1.0f});
    info.flatShader.setMat4("model", model);
    crosshair_buf.draw();

    model = glm::scale(model_pos_vert_d, glm::vec3{crosshair_size_x, crosshair_length_y, 1.0f});
    info.flatShader.setMat4("model", model);
    crosshair_buf.draw();

    model = glm::scale(model_pos_hori_l, glm::vec3{crosshair_length_x, crosshair_size_y, 1.0f});
    info.flatShader.setMat4("model", model);
    crosshair_buf.draw();

    model = glm::scale(model_pos_hori_r, glm::vec3{crosshair_length_x, crosshair_size_y, 1.0f});
    info.flatShader.setMat4("model", model);
    crosshair_buf.draw();
}

void MineClient::send()
{
    if(host)
    {
        const size_t s = sizeof(cs_packet) + (send_str ? typed_str.size() : 0);
        auto buf = std::make_unique<char[]>(s);
        const auto& playa = players[my_player_id];
        cs_packet.x = ENET_HOST_TO_NET_32(enet_uint32(playa.position[0] * POS_SCALE));
        cs_packet.y = ENET_HOST_TO_NET_32(enet_uint32(playa.position[2] * POS_SCALE));
        cs_packet.yaw = ENET_HOST_TO_NET_16(cs_packet.yaw);
        cs_packet.pitch = ENET_HOST_TO_NET_16(cs_packet.pitch);
        cs_packet.looking_at_x = playa.looking_at_x;
        cs_packet.looking_at_y = playa.looking_at_y;

        memcpy(buf.get(), &cs_packet, sizeof(cs_packet));
        if(send_str)
        {
            std::copy(typed_str.begin(), typed_str.end(), buf.get() + sizeof(cs_packet));
            typed_str.clear();
            send_str = false;
        }

        auto send_packet(enet_packet_create(buf.get(), s, 0));
        enet_peer_send(peer, 0, send_packet);
        enet_host_flush(host.get());

        cs_packet.action = 0;

        cs_packet.yaw = ENET_NET_TO_HOST_16(cs_packet.yaw);
        cs_packet.pitch = ENET_NET_TO_HOST_16(cs_packet.pitch);
    }
}

MineClient::State MineClient::get_state() const
{
    return current_state;
}

glm::mat4 MineClient::get_view_matrix()
{
    const auto& self = players[my_player_id];

    // calculate the new Front vector
    const auto yaw_rads = glm::radians(float(self.yaw));
    const auto pitch_rads = glm::radians(float(self.pitch));

    glm::vec3 front;
    front.x = cos(yaw_rads) * cos(pitch_rads);
    front.y = sin(pitch_rads);
    front.z = sin(yaw_rads) * cos(pitch_rads);
    const auto Front = glm::normalize(front);

    // also re-calculate the Right and Up vector
    const auto Right = glm::normalize(glm::cross(Front, glm::vec3(0, 1, 0)));  // normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
    const auto Up    = glm::normalize(glm::cross(Right, Front));

    return glm::lookAt(self.position, self.position + Front, Up);
}

glm::mat4 MineClient::get_top_view_matrix()
{
    const auto& self = players[my_player_id];
    return glm::lookAt(self.position, self.position + glm::vec3{0.0f, -1.0f, 0.0f}, glm::vec3{0.0f, 0.0f, 1.0f});
}

void MineClient::render_world()
{
    const float minX = 0.0f;
    const float minY = 0.0f;
    constexpr UVArr tl_flag_uv{0.25f + 0.125f, 1.0f, -0.125f, 0.25f};
    constexpr std::pair<float, float> tl_lower_uvs[2] = {
        {0.375f, 0.75f}, // undiscovered bg
        {0.375f, 0.5f}, // discovered bg
    };
    const glm::vec3 numbers_color[8] = {
        {0, 0, 1},
        {0, 1, 0},
        {1, 0, 0},

        {0.75f, 0, 0.75f},
        {185.0f/255.0f, 122.0f/255.0f, 87.0f/255.0f},
        {0, 1, 1},

        {0, 0, 0},
        {0.25f, 0.25f, 0.25f},
    };

    int xi = 0;
    int yi = 0;
    int idx = 0;
    auto lower_verts = lower_world_buf->getAllVerts();
    auto upper_verts = upper_world_buf->getAllVerts();

    for(auto& s : world)
    {
        if(s & 0x80)
        {
            s ^= 0x80;
            const float x_l = minX + xi;
            const float t_y = minY + yi + 1.0f;

            const bool digit = s >= '1' && s <= '9';

            const UVArr& arr = digit ? number_uvs_arr[s - '0'] : (s == 'f' ? tl_flag_uv : transparent_uvs);
            const glm::vec4 color = digit ? glm::vec4(numbers_color[s - '1'], 1.0f) : solidWhite;
            const auto [l_u, t_v, delta_u, delta_v] = arr;

            const auto [lower_l_u, lower_t_v] = tl_lower_uvs[int(digit || s == ' ')];

            const PDD3 pos_upper{
                {x_l, -1.0f + MyEpsilon, t_y},
                {1, 0, 0},
                {0.0f, 0.0f, 1.0f}
            };
            const PDD3 pos_lower{
                {x_l, -1.0f, t_y},
                {1, 0, 0},
                {0.0f, 0.0f, 1.0f}
            };
            const PDD2 upper_uv{
                {l_u + delta_u, t_v},
                {-delta_u, 0.0f},
                {0.0f, delta_v}
            };
            const PDD2 lower_uv{
                {lower_l_u, lower_t_v},
                {0.125f, 0.0f},
                {0.0f, 0.25f}
            };

            Fillers::fill_quad_generic(upper_verts, idx, pos_upper, upper_uv, color);
            Fillers::fill_quad_generic(lower_verts, idx, pos_lower, lower_uv, solidWhite);
        }

        idx += 1;
        xi += 1;
        if(xi == width)
        {
            xi = 0;
            yi += 1;
        }
    }
}
