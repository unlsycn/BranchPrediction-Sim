#ifndef __BIMODAL_H__
#define __BIMODAL_H__

#include <cstddef>
#include <cstdint>

#include "util.h"
template <size_t BIMODAL_WIDTH>
class Bimodal
{
  private:
    using Ctr = Counter<2>;
    Ctr bimodal_table[exp2(BIMODAL_WIDTH)];

  public:
    Bimodal()
    {
        for (auto& ctr : bimodal_table)
            ctr = Ctr(exp2(1));
    }

    bool predict(uint64_t ip)
    {
        uint64_t index = ip & bitmask(BIMODAL_WIDTH);
        return bimodal_table[index].get();
    }

    void update(uint64_t ip, bool taken)
    {
        uint64_t index = ip & bitmask(BIMODAL_WIDTH);
        bimodal_table[index].update(taken);
    }
};

#endif
