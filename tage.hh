#ifndef __TAGE_HH__
#define __TAGE_HH__

#include <algorithm>
#include <array>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <random>
#include <ratio>

#include "bimodal.hh"
#include "bp.hh"
#include "util.hh"

enum class AllocCond
{
    // consider the provider's prediction is correct and altpred's is wrong but we use the altpred, there is no
    // need to allocate a new entry since we will update the use_alt_on_na
    ALL_MISPRED,
    // misprediction by the longest matching entry
    LONGEST_MISPRED,
    FINAL_MISPRED
};

template <typename T>
concept ratioSpec = std::same_as<T, std::ratio<T::num, T::den>>;
template <size_t COMPONENT_NUM,                          // number of predictor components
          size_t CTR_WIDTH,                              // width of prediction counters
          size_t USEFUL_WIDTH,                           // width of useful counters
          size_t USEALT_WIDTH,                           // width of use_alt_on_na counters
          size_t BASE_WIDTH,                             // width of the base preditor
          size_t PATH_HIST_LEN,                          // length of the path history info
          size_t MIN_HIST_LEN,                           // length of the shortest(first) global history info
          ratioSpec HIST_ALPHA,                          // growth rate of the length of geometric global
                                                         // history
          std::array<size_t, COMPONENT_NUM> INDEX_WIDTH, // a array of indexes' width
                                                         // of components
          std::array<size_t, COMPONENT_NUM> TAG_WIDTH,   // a array of tags' width of
                                                         // components
          bool RSHIFT_TO_DECRE_USE,                      // use right shift instead of decrement for useful
                                                         // counter
          bool USE_BASE_AS_ALT,                          // always use base predictor as alternated component
          size_t ALLOC_NUM,                              // max number of entries allocated at one time
          AllocCond ALLOC_COND,                          // allocate new entry if the eventual prediction is
                                                         // incorrect, rather than the longest matching entry.
          bool ALLOC_ATLEAST_ONE,                        // allocate at least one entry
          bool UPDATE_ALT_WHEN_USEFUL_NONE,              // update the altpred when provider's
                                                         // useful equals 0
          bool COMPLICATED_HASH,                         // use complicated hash algorithm
          std::pair<bool, size_t> RESET_STRATEGY         // true  - allocation success
                                                         // counter, the width of the counter
                                                         // false - branch counter, the width
                                                         // of the counter
          >
class Tage : public IDirectionPredictor
{
  private:
    static constexpr size_t HIST_LEN(const int n)
    {
        auto power = 1.0;
        for (auto i = 0; i <= n; i++)
            power *= double(HIST_ALPHA::num) / HIST_ALPHA::den;
        return size_t(MIN_HIST_LEN * power + 0.5);
    }
    static constexpr size_t MAX_HIST_LEN = HIST_LEN(COMPONENT_NUM - 1);
    // we use the formula according to the original paper
    // since manually optimized set of history lengths leads to a limited benefits
    // (<0.5%)

    static constexpr size_t MAX_INDEX_WIDTH = *std::max_element(INDEX_WIDTH.begin(), INDEX_WIDTH.end());
    static constexpr size_t MAX_TAG_WIDTH = *std::max_element(TAG_WIDTH.begin(), TAG_WIDTH.end());

    using Ctr = Counter<CTR_WIDTH>;
    using Index = uint16_t;
    using Tag = uint16_t;
    using Useful = Counter<USEFUL_WIDTH, RSHIFT_TO_DECRE_USE>;
    using UseAlt = Counter<USEALT_WIDTH>;

    struct TageEntry
    {
      public:
        Ctr pred;
        Tag tag;
        Useful useful;

      public:
        TageEntry()
        {
            pred = Ctr(exp2(CTR_WIDTH - 1)); // weakly taken
        }
    };

    using CompEntey = std::pair<int, TageEntry *>;
    static constexpr auto NULL_ENTRY = CompEntey(-1, nullptr);

  private:
    // use bool array instead std::bitset delivered a 4-fold increase in
    // performance
    bool global_history[MAX_HIST_LEN];
    bool path_history[PATH_HIST_LEN]; // the last bits of the last branch PCs

    Bimodal<2, BASE_WIDTH> base;
    TageEntry predict_table[COMPONENT_NUM][exp2(MAX_INDEX_WIDTH)];
    UseAlt use_alt_on_na;
    Counter<RESET_STRATEGY.second> success_alloc;
    Counter<RESET_STRATEGY.second> branch;

    // last prediction
    bool used_alt;
    bool used_base;
    bool prediction;
    CompEntey provider;
    CompEntey alter;

  private:
    uint64_t foldGlobalHistory(const size_t original_size, const size_t folded_size)
    {
        uint64_t folded_hist = 0, temp_hist = 0;
        for (unsigned i = 0; i < original_size; i++)
        {
            if (i % folded_size == 0)
            {
                folded_hist ^= temp_hist;
                temp_hist = 0;
            }
            temp_hist = (temp_hist << 1) | global_history[i];
        }
        folded_hist ^= temp_hist;
        return folded_hist;
    }

    template <bool COMPLICATED>
    uint64_t foldPathHistory(const int component)
        requires(COMPLICATED)
    {
        const int original_size = std::min(HIST_LEN(component), PATH_HIST_LEN);
        const int folded_size = INDEX_WIDTH[component];
        const uint64_t bitmask_folded = bitmask(folded_size);
        uint64_t path = 0;
        for (auto i = original_size - 1; i >= 0; i--)
        {
            path = (path << 1) | path_history[i];
        }
        path = path & bitmask(original_size);

        auto F = [component, bitmask_folded](uint64_t x) {
            return ((x << component) & bitmask_folded) + (x >> std::abs(int(INDEX_WIDTH[component] - component)));
        };
        return F((path & bitmask_folded) ^ F((path >> INDEX_WIDTH[component]) & bitmask_folded));
    }

    template <bool COMPLICATED>
    uint64_t foldPathHistory(const int component)
        requires(!COMPLICATED)
    {
        const int original_size = std::min(HIST_LEN(component), PATH_HIST_LEN);
        const auto folded_size = INDEX_WIDTH[component];
        uint64_t folded_hist = 0, path = 0;
        for (auto i = 0; i < original_size; i++)
        {
            if (i % folded_size == 0)
            {
                folded_hist ^= path;
                path = 0;
            }
            path = (path << 1) | path_history[i];
        }
        folded_hist ^= path;
        return folded_hist;
    }

    uint64_t foldPathHistory(const int component)
    {
        return foldPathHistory<COMPLICATED_HASH>(component);
    }

    Tag getTag(uint64_t ip, const int component)
    {
        const uint64_t ghist_hash = foldGlobalHistory(HIST_LEN(component), TAG_WIDTH[component])
                                    ^ foldGlobalHistory(HIST_LEN(component), TAG_WIDTH[component] - 1);
        return (ghist_hash ^ ip) & bitmask(TAG_WIDTH[component]);
    }

    Index getIndex(uint64_t ip, const int component)
    {
        const uint64_t ghist_hash = foldGlobalHistory(HIST_LEN(component), TAG_WIDTH[component]);
        const uint64_t phist_hash = foldPathHistory(component);
        return (ghist_hash ^ phist_hash ^ ip) & bitmask(INDEX_WIDTH[component]);
    }

    CompEntey matchComp(uint64_t ip, const int below = COMPONENT_NUM)
    {
        for (auto i = below - 1; i >= 0; i--)
        {
            const auto index = getIndex(ip, i);
            const auto tag = getTag(ip, i);
            if (predict_table[i][index].tag == tag)
                return CompEntey(i, &predict_table[i][index]);
        }
        return NULL_ENTRY; // base predictor
    }

    CompEntey allocEntry(uint64_t ip, const int start)
    {
        for (auto i = start; i < COMPONENT_NUM; i++)
        {
            TageEntry *const entry = &predict_table[i][getIndex(ip, i)];
            const auto alloc = entry->useful.none() && (USEFUL_WIDTH > 1 || !entry->pred.isStrong());
            // when u is single-bit, only entries with u = 0 and pred is not strong
            // can be replaced
            if (alloc)
            {
                entry->pred = Ctr(exp2(CTR_WIDTH - 1));
                entry->useful = Useful();
                entry->tag = getTag(ip, i);
                return CompEntey(i, entry);
            }
            success_alloc.update(alloc);
        }
        return NULL_ENTRY;
    }

    void updateHistory(uint64_t ip, bool taken)
    {
        for (auto i = MAX_HIST_LEN - 1; i > 0; i--)
            global_history[i] = global_history[i - 1];
        global_history[0] = taken;

        for (auto i = PATH_HIST_LEN - 1; i > 0; i--)
            path_history[i] = path_history[i - 1];
        path_history[0] = ip & 1;
    }

    template <bool STRATEGY>
    void clearUseful()
        requires(STRATEGY)
    {
        if (success_alloc.none())
        {
            success_alloc.set();
            for (auto &comp : predict_table)
                for (auto &entry : comp)
                    entry.useful--;
        }
    }

    template <bool STRATEGY>
    void clearUseful()
        requires(!STRATEGY)
    {
        if (branch.all())
        {
            branch.reset();
            for (auto &comp : predict_table)
                for (auto &entry : comp)
                    entry.useful--;
        }
    }

    void clearUseful()
    {
        clearUseful<RESET_STRATEGY.first>();
    }

  public:
    Tage()
    {
        use_alt_on_na = UseAlt(exp2(USEALT_WIDTH - 1));
    }

    const std::string &getName() override
    {
        static const std::string name = fmt::format("TAGE<>");
        return name;
    }

    bool predict(uint64_t ip) override
    {
        used_alt = false;
        used_base = false;
        branch++;

        provider = matchComp(ip);
        alter = USE_BASE_AS_ALT ? NULL_ENTRY : matchComp(ip, provider.first);
        auto provider_entry = provider.second;
        auto alter_entry = alter.second;
        if (provider_entry)
        {
            if (!use_alt_on_na.get() || provider_entry->pred.isStrong()) // use provider
            {
                prediction = provider_entry->pred.get();
            }
            else
            {
                used_alt = true;
                if (alter_entry) // use altpred
                {
                    prediction = alter_entry->pred.get();
                }
                else
                {
                    used_base = true;
                    prediction = base.predict(ip);
                }
            }
        }
        else
        {
            used_base = true;
            prediction = base.predict(ip);
        }
        return prediction;
    }

    void update(uint64_t ip, bool taken) override
    {
        auto provider_entry = provider.second;
        auto alter_entry = alter.second;

        auto mispredict = [taken](TageEntry *entry) -> bool {
            return !entry || entry->pred.get() != taken;
        };
        bool need_alloc;
        if (ALLOC_COND == AllocCond::ALL_MISPRED)
            need_alloc = mispredict(provider_entry) && mispredict(alter_entry);
        else if (ALLOC_COND == AllocCond::LONGEST_MISPRED)
            need_alloc = mispredict(provider_entry);
        else if (ALLOC_COND == AllocCond::FINAL_MISPRED)
            need_alloc = prediction != taken;

        // The judgment must be made before update, even though it can improve
        // accuracy otherwise

        if (provider_entry)
        {
            assert(!used_alt || (bool(alter_entry) ^ used_base));
            const auto provider_pred = provider_entry->pred.get();
            const auto altpred = alter_entry ? alter_entry->pred.get() : prediction;
            const auto provider_correct = provider_pred == taken;
            const auto update_alt = used_alt || (UPDATE_ALT_WHEN_USEFUL_NONE && provider_entry->useful.none());
            const auto pred_distinct = provider_pred != altpred;

            if (pred_distinct)
            {
                use_alt_on_na.update(!provider_correct);
                provider_entry->useful.update(provider_correct,
                                              !used_alt); // only decrese provider's useful when it is used
            }

            if (update_alt)
            {
                if (alter_entry)
                    alter_entry->pred.update(taken);
                else
                    base.update(ip, taken);
            }
            provider_entry->pred.update(taken);
        }
        else
            base.update(ip, taken);

        // allocate new entry
        if (need_alloc)
        {
            const auto random = rand();
            const auto start = provider.first + 1 + (random & 1) + (random & 2);
            assert(start >= 0);

            const auto first_alloc = allocEntry(ip, start);
            auto last_alloc = first_alloc;
            for (auto i = 1; i < ALLOC_NUM; i++)
                if (last_alloc.second)
                    last_alloc = allocEntry(ip, last_alloc.first);

            // allocate at least one entry
            if (ALLOC_ATLEAST_ONE && !first_alloc.second)
            {
                const auto alloc_comp = std::min(start + 1, int(COMPONENT_NUM - 1));
                predict_table[alloc_comp][getIndex(ip, alloc_comp)].useful.reset();
                allocEntry(ip, start);
            }
        }

        clearUseful();
        updateHistory(ip, taken);
    }
};

#endif