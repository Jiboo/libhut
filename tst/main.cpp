#include <ext/codecvt_specializations.h>

#include <gtest/gtest.h>

#include "hut.hpp"

class unit_tests : public hut::application {
public:
    virtual int entry(int argc, char** argv, hut::window& main) override {

        main.geometry = {{250, 250}};
        main.title = "Hello world";
        main.clear_color = {{0, 0, 0, 0.5}};

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

        float local[] = {
                0, 0,   1, 1, 1, 1,     0, 0,
                1, 0,   1, 1, 1, 1,     0, 0,
                1, 1,   1, 1, 1, 1,     0, 0,
        };

        hut::buffer vbo {(uint8_t*)local, sizeof(local), hut::BSTREAM};

        hut::mat4 trans1 = hut::mat4::init();
        hut::mat4 trans2 = hut::mat4::init();
        hut::vec4 tint1 = {1, 0, 0, 0.5};
        hut::vec4 tint2 = {0, 1, 0, 0.5};
        float opacity = 0.5f;
        hut::drawable shader_simple = hut::drawable::factory()
                .pos(vbo, 0, 6).transform(trans1).transform(trans2)
                .col(vbo, 2, 4).col(tint1).col(tint2).opacity(opacity)
                .compile();

        main.on_draw.connect([&shader_simple]() -> bool {
            shader_simple.bind();

            glDrawArrays(GL_TRIANGLES, 0, 3);

            shader_simple.unbind();
            return false;
        });

        ::testing::InitGoogleTest(&argc, argv);
        int result = RUN_ALL_TESTS();

        result |= application::entry(argc, argv, main);
        return result;
    }
};

HUT_MAIN(unit_tests);