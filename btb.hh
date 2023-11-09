#ifndef __BTB_HH__
#define __BTB_HH__

#include <cstddef>
#include <cstdint>

#include "bp.hh"
#include "util.hh"

enum class TagAlgo
{
    TRUNC,
    XOR
};

template <size_t BTB_WIDTH, size_t TAG_WIDTH, TagAlgo TAG_ALGO = TagAlgo::TRUNC, size_t PC_SHIFT_AMT = 2>
    requires(BTB_WIDTH + TAG_WIDTH <= 64)
class Btb : public ITargetPredictor
{
  private:
    struct BtbEntry
    {
        bool valid;
        uint64_t tag;
        BranchTarget target;
    };

    BtbEntry btb[exp2(BTB_WIDTH)] = {};

    uint64_t getIndex(uint64_t ip)
    {
        return ip & bitmask(BTB_WIDTH);
    }

    template <TagAlgo ALGO>
    uint64_t getTag(uint64_t ip)
        requires(ALGO == TagAlgo::TRUNC)
    {
        return (ip >> BTB_WIDTH) & bitmask(TAG_WIDTH);
    }

    template <TagAlgo ALGO>
    uint64_t getTag(uint64_t ip)
        requires(ALGO == TagAlgo::XOR)
    {
        return fold<64 - BTB_WIDTH, TAG_WIDTH>(ip >> BTB_WIDTH);
    }

    uint64_t getTag(uint64_t ip)
    {
        return getTag<TAG_ALGO>(ip);
    }

  public:
    const std::string &getName() override
    {
        static const std::string name = fmt::format("BTB<{}, {}>", BTB_WIDTH, TAG_WIDTH);
        return name;
    };

    BranchTarget *predict(uint64_t ip) override
    {
        ip >>= PC_SHIFT_AMT;
        BtbEntry *entry = &btb[getIndex(ip)];
        if (entry->valid && entry->tag == getTag(ip))
            return &(entry->target);
        return nullptr;
    }

    void update(uint64_t ip, BranchTarget target) override
    {
        ip >>= PC_SHIFT_AMT;
        BtbEntry *entry = &btb[getIndex(ip)];
        entry->tag = getTag(ip);
        entry->target = target;
        entry->valid = true;
    }
};

#endif
