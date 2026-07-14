#include <cstdint>

extern "C" int idtech4_emscripten_smoke()
{
    constexpr std::uint32_t magic = 0x49443457u;
    return magic == 0x49443457u ? 0 : 1;
}

int main()
{
    return idtech4_emscripten_smoke();
}
