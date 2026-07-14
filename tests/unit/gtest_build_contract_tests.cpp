#include <cstdint>
#include <cstring>

#include <gtest/gtest.h>

namespace
{

bool IsLittleEndian()
{
    const std::uint32_t value = 0x01020304u;
    unsigned char firstByte = 0;
    std::memcpy(&firstByte, &value, sizeof(firstByte));
    return firstByte == 0x04u;
}

} // namespace

TEST(BuildContract, PointerSizeIsSupported)
{
    EXPECT_TRUE(sizeof(void*) == 4u || sizeof(void*) == 8u);
}

TEST(BuildContract, EngineAssumesLittleEndianTargets)
{
    EXPECT_TRUE(IsLittleEndian());
}
