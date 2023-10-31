#ifndef __UTIL_HH__
#define __UTIL_HH__

#include <cstddef>
#include <cstdint>

constexpr unsigned lg2(uint64_t n)
{
    return n < 2 ? 0 : 1 + lg2(n / 2);
}

constexpr uint64_t exp2(std::size_t width)
{
    return 1ull << width;
}
constexpr uint64_t bitmask(std::size_t begin, std::size_t end = 0)
{
    return ((1ull << (begin - end)) - 1) << end;
}

constexpr uint64_t splice_bits(uint64_t upper, uint64_t lower, std::size_t bits)
{
    return (upper & ~bitmask(bits)) | (lower & bitmask(bits));
}

#endif
