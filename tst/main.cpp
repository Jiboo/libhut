#include <ext/codecvt_specializations.h>

#include <gtest/gtest.h>
#include <sys/time.h>

#include "hut.hpp"

class unit_tests : public hut::application {
public:
    virtual int entry(int argc, char** argv, hut::window& main) override {

        /*main.title = "Hello world";
        main.clear_color = {{0, 0, 0, 0}};
        main.geometry = {{250, 250}};

        auto_display().post([]{
            std::cout << "posted job run once!" << std::endl;
        });

        auto job1 = auto_display().post([]{
            std::cout << "posted job that will get removed!" << std::endl;
        });
        auto_display().unpost(job1);

        auto_display().post_delayed([&main]{
            std::cout << "posted job with 5s delay!" << std::endl;
        }, std::chrono::milliseconds {5000});

        auto_display().schedule([]{
            std::cout << "scheduled job with 5s delay!" << std::endl;
        }, std::chrono::milliseconds {5000});

        main.on_ctrl.connect([](hut::keyboard_control_type ctrl, bool down) {
            std::cout << "ctrl ";
            switch(ctrl) {
                case hut::KALT_LEFT: std::cout << "KALT_LEFT"; break;
                case hut::KALT_RIGHT: std::cout << "KALT_RIGHT"; break;
                case hut::KCTRL_LEFT: std::cout << "KCTRL_LEFT"; break;
                case hut::KCTRL_RIGHT: std::cout << "KCTRL_RIGHT"; break;
                case hut::KSHIFT_LEFT: std::cout << "KSHIFT_LEFT"; break;
                case hut::KSHIFT_RIGHT: std::cout << "KSHIFT_RIGHT"; break;
                case hut::KPAGE_UP: std::cout << "KPAGE_UP"; break;
                case hut::KPAGE_DOWN: std::cout << "KPAGE_DOWN"; break;
                case hut::KVOLUME_UP: std::cout << "KVOLUME_UP"; break;
                case hut::KVOLUME_DOWN: std::cout << "KVOLUME_DOWN"; break;
                case hut::KUP: std::cout << "KUP"; break;
                case hut::KRIGHT: std::cout << "KRIGHT"; break;
                case hut::KDOWN: std::cout << "KDOWN"; break;
                case hut::KLEFT: std::cout << "KLEFT"; break;
                case hut::KBACKSPACE: std::cout << "KBACKSPACE"; break;
                case hut::KDELETE: std::cout << "KDELETE"; break;
                case hut::KINSER: std::cout << "KINSER"; break;
                case hut::KESCAPE: std::cout << "KESCAPE"; break;
                case hut::KF1: std::cout << "KF1"; break;
                case hut::KF2: std::cout << "KF2"; break;
                case hut::KF3: std::cout << "KF3"; break;
                case hut::KF4: std::cout << "KF4"; break;
                case hut::KF5: std::cout << "KF5"; break;
                case hut::KF6: std::cout << "KF6"; break;
                case hut::KF7: std::cout << "KF7"; break;
                case hut::KF8: std::cout << "KF8"; break;
                case hut::KF9: std::cout << "KF9"; break;
            }
            std::cout << " is " << (down ? "down" : "up") << std::endl;

            return true;
        });

        main.on_char.connect([](char32_t ch, bool down) {
            std::string utf8 = hut::to_utf8(ch);
            std::cout << "char " << utf8 << " is " << (down ? "down" : "up") << std::endl;

            return true;
        });

        main.on_mouse.connect([](uint8_t button, hut::mouse_event_type type, hut::ivec2 pos) {
            std::cout << "mouse event ";
            switch(type) {
                case hut::MDOWN: std::cout << "MDOWN"; break;
                case hut::MUP: std::cout << "MUP"; break;
                case hut::MMOVE: std::cout << "MMOVE"; break;
                case hut::MENTER: std::cout << "MENTER"; break;
                case hut::MLEAVE: std::cout << "MLEAVE"; break;
                case hut::MWHEEL_UP: std::cout << "MWHEEL_UP"; break;
                case hut::MWHEEL_DOWN: std::cout << "MWHEEL_DOWN"; break;
            }
            std::cout << " at " << pos << " with button " << (int)button << std::endl;
            return true;
        });

        #define OPACITY 0.5f

        float local[] = {
            0, 0,       0, 0, 1, OPACITY,   0, 0,
            50, 0,      0, 0, 1, OPACITY,   1, 0,
            50, 50,     0, 0, 1, OPACITY,   1, 1,
            0, 0,       0, 0, 0, 0,         0, 0,
            50, 50,     0, 0, 0, 0,         1, 1,
            0, 50,      0, 0, 0, 0,         0, 1,
            0, 0,       1, 1, 0, OPACITY,   0, 0,
            50, 0,      1, 1, 0, OPACITY,   0, 0,
            0, 50,      1, 1, 0, OPACITY,   0, 0,
            50, 0,      0, 0, 0, 0,         0, 0,
            50, 50,     0, 0, 0, 0,         0, 0,
            0, 50,      0, 0, 0, 0,         0, 0,
        };
        hut::blend_mode blend_modes[] = {
                hut::BLEND_CLEAR, hut::BLEND_SRC, hut::BLEND_DST, hut::BLEND_OVER,
                hut::BLEND_DST_OVER, hut::BLEND_IN, hut::BLEND_DST_IN, hut::BLEND_OUT,
                hut::BLEND_DST_OUT, hut::BLEND_ATOP, hut::BLEND_DST_ATOP, hut::BLEND_XOR,
                hut::BLEND_NONE,
        };

        hut::buffer vbo {(uint8_t*)local, sizeof(local)};
        hut::mat4 proj = hut::mat4::ortho(0, main.geometry.get()[0], main.geometry.get()[1], 0, 100, -100);
        hut::mat4 model = hut::mat4::init();
        size_t modes_count = sizeof(blend_modes) / sizeof(blend_modes[0]);

        hut::drawable left = hut::drawable::factory()
            .pos(vbo, 0 * sizeof(float), 8 * sizeof(float)).transform(model).transform(proj)
            .col(vbo, 2 * sizeof(float), 8 * sizeof(float))
            .compile(hut::PRIMITIVE_TRIANGLES, 6);

        hut::drawable right[modes_count];
        for(size_t i = 0; i < modes_count; i++) {
            right[i] = hut::drawable::factory()
                .pos(vbo, 6 * 8 * sizeof(float), 8 * sizeof(float)).transform(model).transform(proj)
                .col(vbo, 6 * 8 * sizeof(float) + 2 * sizeof(float), 8 * sizeof(float), blend_modes[i])
                .compile(hut::PRIMITIVE_TRIANGLES, 6);
        }

        hut::texture tex1 = hut::load_png("tex1.png");
        hut::texture tex2 = hut::load_png("tex2.png");

        float local2[] = {
                0, 0,       0, 0, 1, OPACITY,   0, 0,
                50, 0,      0, 0, 1, OPACITY,   1, 0,
                50, 50,     0, 0, 1, OPACITY,   1, 1,
                0, 0,       0, 0, 1, OPACITY,   0, 0,
                50, 50,     0, 0, 1, OPACITY,   1, 1,
                0, 50,      0, 0, 1, OPACITY,   0, 1,
        };
        hut::buffer vbo2 {(uint8_t*)local2, sizeof(local2)};

        hut::drawable tex1rect = hut::drawable::factory()
                .pos(vbo2, 0 * sizeof(float), 8 * sizeof(float)).transform(model).transform(proj)
                .tex(tex1, vbo2, 6 * sizeof(float), 8 * sizeof(float), hut::BLEND_OVER)
                .tex(tex2, vbo2, 6 * sizeof(float), 8 * sizeof(float), hut::BLEND_ATOP)
                .compile(hut::PRIMITIVE_TRIANGLES, 6);

        int frames = 0;
        auto drawing_time = std::chrono::milliseconds(0);
        auto bench_start = std::chrono::steady_clock::now();

        main.on_draw.connect([&]() -> bool {
            std::chrono::steady_clock::time_point frame_start = std::chrono::steady_clock::now();

            for (size_t i = 0; i < modes_count; i++) {
                model = hut::mat4::trans({{(i % 4) * 50, (i / 4) * 50, 0}});
                main.draw(left);
                main.draw(right[i]);
            }

            const float scale = frames / 60.f * 10;
            model = hut::mat4::trans({{0, 0, 0}}) * hut::mat4::scale({{scale, scale, 0}});
            main.draw(tex1rect);

            std::chrono::steady_clock::time_point frame_end = std::chrono::steady_clock::now();
            frames++;
            drawing_time += std::chrono::duration_cast<std::chrono::milliseconds>(frame_end - frame_start);

            if(std::chrono::duration_cast<std::chrono::milliseconds>(frame_end - bench_start).count() > 1000) {
                std::cout << "Render stats: "
                        << frames << " fps, "
                        << (drawing_time.count() / frames) << "ms average drawing time"
                        << std::endl;
                bench_start = frame_end;
                drawing_time = std::chrono::milliseconds::zero();
                frames = 0;
            }

            return false;
        });*/

        struct vertex_info {
            hut::vec2 pos;
            hut::vec4 col;
            float tex;
            hut::vec2 texcoords;
        };

        vertex_info vertices[] {
            {{0, 0},     {0, 0, 1, 0.5}, 0, {0, 0}},
            {{100, 0},   {0, 0, 1, 0.5}, 0, {1, 0}},
            {{100, 100}, {0, 0, 1, 0.5}, 0, {1, 1}},
            {{0, 0},     {0, 0, 1, 0.5}, 0, {0, 0}},
            {{100, 100}, {0, 0, 1, 0.5}, 0, {1, 1}},
            {{0, 100},   {0, 0, 1, 0.5}, 0, {0, 1}},

            {{100, 100}, {0, 1, 0, 0.5}, 1, {0, 0}},
            {{200, 100}, {0, 1, 0, 0.5}, 1, {1, 0}},
            {{200, 200}, {0, 1, 0, 0.5}, 1, {1, 1}},
            {{100, 100}, {0, 1, 0, 0.5}, 1, {0, 0}},
            {{200, 200}, {0, 1, 0, 0.5}, 1, {1, 1}},
            {{100, 200}, {0, 1, 0, 0.5}, 1, {0, 1}},
        };
        std::shared_ptr<hut::buffer> vbo = std::make_shared<hut::buffer>((uint8_t*)vertices, sizeof(vertices));

        hut::mat4 proj = hut::mat4::ortho(0, main.geometry.get()[0], main.geometry.get()[1], 0, 100, -100);
        hut::mat4 model = hut::mat4::init();

        auto tex1 = hut::load_png("tex1.png");
        auto tex2 = hut::load_png("tex2.png");

        auto rects = hut::drawable::factory()
            .pos(vbo, offsetof(vertex_info, pos), sizeof(vertex_info)).transform(model).transform(proj)
            .multitex({tex1, tex2}, vbo, offsetof(vertex_info, tex), offsetof(vertex_info, texcoords), sizeof(vertex_info), hut::BLEND_OVER)
            .col(vbo, offsetof(vertex_info, col), sizeof(vertex_info), hut::BLEND_ATOP)
            .compile(hut::PRIMITIVE_TRIANGLES, 12);

        main.on_draw.connect([&]() -> bool {
            main.draw(rects);
            return false;
        });

        ::testing::InitGoogleTest(&argc, argv);
        int result = RUN_ALL_TESTS();

        return result || application::entry(argc, argv, main);
    }
};

HUT_MAIN(unit_tests);
