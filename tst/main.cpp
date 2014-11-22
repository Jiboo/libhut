#include <ext/codecvt_specializations.h>

#include <gtest/gtest.h>
#include <sys/time.h>

#include "hut.hpp"

class unit_tests : public hut::application {
public:
    virtual int entry(int argc, char** argv, hut::window& main) override {

        auto proj = std::make_shared<hut::mat4>(hut::mat4::ortho(0, main.geometry.get()[0], main.geometry.get()[1], 0, INT_MAX, INT_MIN));
        auto model = std::make_shared<hut::mat4>(hut::mat4::init());
        auto bg_color = std::make_shared<hut::vec4>(hut::vec4{1, 1, 1, 1});
        auto circle_color = std::make_shared<hut::vec4>(hut::vec4{0, 1, 1, 0.5});
        auto circle_params = std::make_shared<hut::vec4>(hut::vec4{250, 250, 100, 0.5});
        auto shadow_color = std::make_shared<hut::vec4>(hut::vec4{0, 0, 0, 0.5});
        auto shadow_params = std::make_shared<hut::vec4>(hut::vec4{250, 250, 100, 50});

        struct vertex_info {
            hut::vec2 pos;
            hut::vec2 texcoords;
        };

        vertex_info vertices[] {
                {{0, 0},     {0, 0}},
                {{500, 0},   {1, 0}},
                {{500, 500}, {1, 1}},
                {{0, 0},     {0, 0}},
                {{500, 500}, {1, 1}},
                {{0, 500},   {0, 1}},
        };
        std::shared_ptr<hut::buffer> vbo = std::make_shared<hut::buffer>((uint8_t*)vertices, sizeof(vertices));

        auto tex1 = hut::load_png("tex1.png");
        auto bg = hut::drawable::factory(vbo, 0, 16).transform(model).transform(proj)
                .tex(tex1, vbo, 8, 16, hut::BLEND_OVER)
                .compile(hut::PRIMITIVE_TRIANGLES, 6);

        auto shadow = hut::drawable::factory(vbo, 0, 16).transform(model).transform(proj)
                .col(shadow_color, hut::BLEND_OVER)
                .circle(shadow_params)
                .compile(hut::PRIMITIVE_TRIANGLES, 6);

        auto circle = hut::drawable::factory(vbo, 0, 16).transform(model).transform(proj)
                .col(circle_color, hut::BLEND_OVER)
                .circle(circle_params)
                .compile(hut::PRIMITIVE_TRIANGLES, 6);

        float angle = 0, frames = 0;
        main.on_draw.connect([&]() -> bool {
            const float scale = frames / 60.f;
            //*model = hut::mat4::trans({{100, 100, 0}});
            //*model = hut::mat4::scale({1.5, 1.5, 1});
            //*model = hut::mat4::scale({2, 2, 1}) * hut::mat4::trans({{-250, -250, 0}});
            //*model = hut::mat4::trans({{-250, -250, 0}}) * hut::mat4::scale({2, 2, 1}) * hut::mat4::trans({{300, 300, 0}});
            //*model = hut::mat4::trans({{-250, -250, 0}}) * hut::mat4::rotate_x(frames/360.f) * hut::mat4::trans({{250, 250, 0}});
            *model = hut::mat4::trans({{-250, -250, 0}}) * hut::mat4::rotate_y(frames/360.f) * hut::mat4::scale({2, 2, 1}) * hut::mat4::trans({{300, 300, 0}});
            frames++;

            main.draw(bg);
            //main.draw(shadow);
            main.draw(circle);
            return false;
        });

        ::testing::InitGoogleTest(&argc, argv);
        int result = RUN_ALL_TESTS();

        return result || application::entry(argc, argv, main);
    }
};

HUT_MAIN(unit_tests);
