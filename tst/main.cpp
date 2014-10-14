#include <gtest/gtest.h>

#include "hut.hpp"

class unit_tests : public hut::application {
public:
    virtual int entry(int argc, char** argv) override {
        ::testing::InitGoogleTest(&argc, argv);
        int result = RUN_ALL_TESTS();

        auto w = create_window();

        result |= application::entry(argc, argv);
        return result;
    }
};

HUT_MAIN(unit_tests);