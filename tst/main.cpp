#include <gtest/gtest.h>

#include "hut.hpp"

class unit_tests : public hut::application {
public:
    virtual int entry(int argc, char** argv) override {
        ::testing::InitGoogleTest(&argc, argv);
        int result = RUN_ALL_TESTS();

        hut::window win {auto_display(), "Hello world"};

        result |= application::entry(argc, argv);
        return result;
    }
};

HUT_MAIN(unit_tests);