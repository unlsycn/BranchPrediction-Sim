#ifndef __BIMODAL_HH__
#define __BIMODAL_HH__

#include <cstddef>
#include <cstdint>

#include "bp.hh"
template <size_t BIMODAL_WIDTH>
class Bimodal : public IDirectionPredictor
{
  private:
    using Ctr = Counter<2>;
    Ctr bimodal_table[exp2(BIMODAL_WIDTH)];

  public:
    Bimodal()
    {
        for (auto &ctr : bimodal_table)
            ctr = Ctr(exp2(1));
    }

    const std::string &getName() override
    {
        static const std::string name = fmt::format("Bimodal<{}>", BIMODAL_WIDTH);
        return name;
    }

    bool predict(uint64_t ip) override
    {
        uint64_t index = ip & bitmask(BIMODAL_WIDTH);
        return bimodal_table[index].get();
    }

    void update(uint64_t ip, bool taken) override
    {
        uint64_t index = ip & bitmask(BIMODAL_WIDTH);
        bimodal_table[index].update(taken);
    }
};

#endif
