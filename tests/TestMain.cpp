#include <gtest/gtest.h>

#include <cstdlib>
#include <ctime>
#include <filesystem>

class GlobalTestEnvironment : public ::testing::Environment
{
  public:
    void SetUp() override
    {
        std::srand(static_cast<unsigned>(std::time(nullptr)));
        std::filesystem::create_directories("outdir");
    }

    void TearDown() override
    {
        std::filesystem::remove_all("outdir");
    }
};

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    testing::AddGlobalTestEnvironment(new GlobalTestEnvironment);
    return RUN_ALL_TESTS();
}
