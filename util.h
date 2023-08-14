/*
 *    Copyright 2023 The ChampSim Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef UTIL_H
#define UTIL_H

#include <bitset>
#include <cstdint>

#define _CONCAT(x, y) x##y
#define CONCAT(x, y) _CONCAT(x, y)

#define Assert(cond, format, ...)                                          \
    do                                                                     \
    {                                                                      \
        if (!(cond))                                                       \
        {                                                                  \
            (fflush(stdout), fprintf(stderr, format "\n", ##__VA_ARGS__)); \
            assert(cond);                                                  \
        }                                                                  \
    } while (0)

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

template <size_t width, bool RSHIFT_TO_DECRE = false>
struct Counter : public std::bitset<width>
{
    size_t max = bitmask(width);

    Counter& operator++()
    {
        if (this->to_ulong() < max)
            *this = Counter(this->to_ulong() + 1);
        return *this;
    }
    Counter& operator--()
    {
        if constexpr (RSHIFT_TO_DECRE)
            // substracting 1 to a whole table is not that realistic
            *this >>= 1;
        else if (long(this->to_ulong()) > 0)
            *this = Counter(this->to_ulong() - 1);
        return *this;
    }
    Counter operator++(int)
    {
        Counter temp(*this);
        operator++();
        return temp;
    }
    Counter operator--(int)
    {
        Counter temp(*this);
        operator--();
        return temp;
    }

  public:
    void update(bool cond, bool decreCond = true)
    {
        if (cond)
            (*this)++;
        else if (decreCond) // !cond && decreCond
            (*this)--;
    }

    bool get()
    {
        return (*this)[this->size() - 1];
    }

    bool isStrong()
    {
        return this->all() || this->none();
    }
};

#endif
