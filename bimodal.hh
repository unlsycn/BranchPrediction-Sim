#ifndef __BIMODAL_HH__
#define __BIMODAL_HH__

#include <cstddef>
#include <cstdint>

#include "bp.hh"
template <size_t CTR_WIDTH, size_t BIMODAL_WIDTH, size_t PC_SHIFT_AMT = 3>
class Bimodal : public IDirectionPredictor
{
  private:
    using Ctr = Counter<CTR_WIDTH>;
    Ctr bimodal_table[exp2(BIMODAL_WIDTH)];

  public:
    Bimodal()
    {
        for (auto &ctr : bimodal_table)
            ctr = Ctr(exp2(CTR_WIDTH - 1));
    }

    const std::string &getName() override
    {
        static const std::string name = fmt::format("Bimodal<{}>", BIMODAL_WIDTH);
        return name;
    }

    bool predict(uint64_t ip) override
    {
        auto pc_used = ip >> PC_SHIFT_AMT;
        uint64_t index = pc_used & bitmask(BIMODAL_WIDTH);
        return bimodal_table[index].get();
    }

    void update(uint64_t ip, bool taken) override
    {
        auto pc_used = ip >> PC_SHIFT_AMT;
        uint64_t index = pc_used & bitmask(BIMODAL_WIDTH);
        bimodal_table[index].update(taken);
    }
};

#endif
