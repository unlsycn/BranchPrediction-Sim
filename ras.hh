#ifndef __RAS_HH__
#define __RAS_HH__

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include "bp.hh"
template <size_t RAS_DEPTH, size_t CTR_WIDTH>
class Ras : public ICallReturnPredictor
{
  private:
    using Ctr = Counter<CTR_WIDTH>;
    using Addr = struct Addr
    {
        uint64_t addr;
        Ctr repetition;
    };

    Addr stack[RAS_DEPTH];
    Addr *top = &stack[0];

    inline void incrTop()
    {
        if (top++ == &stack[RAS_DEPTH - 1])
            top = &stack[0];
    }

    inline void decrTop()
    {
        if (top-- == &stack[0])
            top = &stack[RAS_DEPTH - 1];
    }

  public:
    const std::string &getName() override
    {
        static const std::string name = fmt::format("RAS<{}, {}>", RAS_DEPTH, CTR_WIDTH);
        return name;
    }

    uint64_t pop() override
    {
        auto ra = top->addr;
        if (top->repetition.any())
            top->repetition--;
        else
            decrTop();
        return ra;
    }

    void push(uint64_t addr) override
    {
        if (addr == top->addr && !top->repetition.all()) // all() always return true if the width is 0
            top->repetition++;
        else
        {
            incrTop();
            top->addr = addr;
        }
    }
};

#endif
