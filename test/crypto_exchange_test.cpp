
#include "../crypto_exchange.cpp"
#include "gtest/gtest.h"

TEST(MathTest, Foobar){
    EXPECT_EQ(4, 2+2);
}

int main(int argc, char **argv){
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
    
