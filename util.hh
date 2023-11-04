#ifndef __UTIL_HH__
#define __UTIL_HH__

#include <cstddef>
#include <cstdint>
#include <cstdio>

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
    if (begin - end >= 64)
        return ~0ul << end;
    return ((1ull << (begin - end)) - 1) << end;
}

constexpr uint64_t spliceBits(uint64_t upper, uint64_t lower, std::size_t bits)
{
    return (upper & ~bitmask(bits)) | (lower & bitmask(bits));
}

enum class IndexAlgo
{
    CONCAT,
    XOR,
    HASH
};

template <size_t ORIGINAL_SIZE, size_t FOLDED_SIZE>
uint64_t foldHistory(auto &history)
{
    uint64_t folded_hist = 0, temp_hist = 0;
    for (auto i = 0; i < ORIGINAL_SIZE; i++)
    {
        if (i % FOLDED_SIZE == 0)
        {
            folded_hist ^= temp_hist;
            temp_hist = 0;
        }
        temp_hist = (temp_hist << 1) | history[i];
    }
    folded_hist ^= temp_hist;
    return folded_hist;
}

template <size_t ORIGINAL_SIZE, size_t FOLDED_SIZE>
uint64_t fold(uint64_t val)
{
    val &= bitmask(ORIGINAL_SIZE);
    uint64_t folded = 0, temp = 0;
    for (auto i = 0; i < ORIGINAL_SIZE / FOLDED_SIZE; i++)
    {
        temp = val & bitmask(FOLDED_SIZE);
        val >>= FOLDED_SIZE;
        folded ^= temp;
    }
    folded ^= val;
    return folded;
}

template <size_t PC_WIDTH, size_t HISTORY_WIDTH, size_t RESULT_WIDTH>
uint64_t getConcatedIndex(uint64_t pc, uint64_t history)
    requires(PC_WIDTH + HISTORY_WIDTH == RESULT_WIDTH)
{
    return ((pc & bitmask(PC_WIDTH)) << HISTORY_WIDTH) + (history & bitmask(HISTORY_WIDTH));
}

template <size_t PC_WIDTH, size_t HISTORY_WIDTH, size_t RESULT_WIDTH>
uint64_t getXoredIndex(uint64_t pc, uint64_t history)
{
    uint64_t p = fold<PC_WIDTH, RESULT_WIDTH>(pc);
    uint64_t h = fold<HISTORY_WIDTH, RESULT_WIDTH>(history);
    return fold<PC_WIDTH, RESULT_WIDTH>(pc) ^ fold<HISTORY_WIDTH, RESULT_WIDTH>(history);
}

#endif
