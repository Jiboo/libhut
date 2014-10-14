#include <gtest/gtest.h>

#include "hut.hpp"

TEST(math, vector) {
    hut::vec2 v1 {1, 1};
    hut::vec2 v2 {2, 2};
    hut::vec2 v3 {4, 4};

    hut::vec2 vcopy = v1;
    EXPECT_EQ(v1, vcopy);

    EXPECT_EQ(v1 + 1, v2);
    EXPECT_EQ(v1 + v1, v2);

    EXPECT_EQ(v1 += 1, v2);
    EXPECT_EQ(v1 += v2, v3);

    hut::vec3 c1 {2, 3, 4};
    hut::vec3 c2 {5, 6, 7};
    hut::vec3 c3 {-3, 6, -3};
    EXPECT_EQ(hut::cross(c1, c2), c3);
}

TEST(math, matrix) {
    hut::mat2 m1 {1, 1, 1, 1};
    hut::mat2 m2 {2, 2, 2, 2};
    hut::mat2 m3 {4, 4, 4, 4};

    hut::mat2 mcopy = m1;
    EXPECT_EQ(m1, mcopy);

    EXPECT_EQ(m1 + 1, m2);
    EXPECT_EQ(m1 + m1, m2);

    EXPECT_EQ(m1 += 1, m2);
    EXPECT_EQ(m1 += m2, m3);

    hut::vec2 v1 {1, 1};
    hut::vec2 v2 {2, 2};
    hut::mat2 mvref {1, 1, 2, 2};

    hut::mat2 mv {v1, v2};
    EXPECT_EQ(mv, mvref);

    hut::mat<float, 3, 1> mm11 = {3, 4, 2};
    hut::mat<float, 4, 3> mm12 = {
            13, 8, 6,
            9, 7, 4,
            7, 4, 0,
            15, 6, 3
    };
    hut::mat<float, 4, 1> mm1ref = {83, 63, 37, 75};
    EXPECT_EQ(mm11 * mm12, mm1ref);

    hut::mat<float, 2, 1> mm21 = {1, 2};
    hut::mat<float, 1, 2> mm22 = {3, 4};
    hut::mat<float, 1, 1> mm2ref = {11};

    EXPECT_EQ(mm21 * mm22, mm2ref);
}
