#ifndef __TAGE_CONFIG_HH__
#define __TAGE_CONFIG_HH__
#include <array>
#include <cstddef>
#include <ratio>

#include "tage.hh"

template <size_t n>
using WidthArray = std::array<size_t, n>;
using Strategy = std::pair<bool, size_t>;

constexpr Strategy success_reset_strategy(true, 8);
constexpr Strategy smooth_reset_strategy(false, 19);

constexpr WidthArray<12> Huge_index_width = {10, 10, 11, 11, 11, 11, 10, 10, 10, 10, 9, 9};
constexpr WidthArray<12> Huge_tag_width = {7, 7, 8, 8, 9, 10, 11, 12, 12, 13, 14, 15};

using HugeTage = Tage<12, 3, 2, 4, 14, 32, 4, std::ratio<8, 5>, Huge_index_width, Huge_tag_width, false, false, 1,
                      AllocCond::FINAL_MISPRED, false, true, false, success_reset_strategy>;

using HugeWithCondATage = Tage<12, 3, 2, 4, 14, 32, 4, std::ratio<8, 5>, Huge_index_width, Huge_tag_width, false, false, 1,
                               AllocCond::ALL_MISPRED, false, true, false, success_reset_strategy>;

using HugeWithCondLTage = Tage<12, 3, 2, 4, 14, 32, 4, std::ratio<8, 5>, Huge_index_width, Huge_tag_width, false, false, 1,
                               AllocCond::LONGEST_MISPRED, false, true, false, success_reset_strategy>;

using HugeWith1UTage = Tage<12, 3, 1, 4, 14, 32, 4, std::ratio<8, 5>, Huge_index_width, Huge_tag_width, false, false, 1,
                            AllocCond::FINAL_MISPRED, false, true, false, success_reset_strategy>;

using HugeWith1UCondATage = Tage<12, 3, 1, 4, 14, 32, 4, std::ratio<8, 5>, Huge_index_width, Huge_tag_width, false, false, 1,
                                 AllocCond::ALL_MISPRED, false, true, false, success_reset_strategy>;

using HugeWith1UCondLTage = Tage<12, 3, 1, 4, 14, 32, 4, std::ratio<8, 5>, Huge_index_width, Huge_tag_width, false, false, 1,
                                 AllocCond::LONGEST_MISPRED, false, true, false, success_reset_strategy>;

using HugeWithRightShiftTage = Tage<12, 3, 2, 4, 14, 32, 4, std::ratio<8, 5>, Huge_index_width, Huge_tag_width, true, false, 1,
                                    AllocCond::FINAL_MISPRED, false, true, false, success_reset_strategy>;

constexpr WidthArray<10> L32_index_width = {8, 8, 8, 8, 7, 8, 7, 6, 6, 6};
constexpr WidthArray<10> L32_tag_width = {7, 7, 7, 8, 9, 10, 10, 11, 13, 13};
using L32Tage = Tage<10, 3, 2, 5, 12, 27, 4, std::ratio<19, 10>, L32_index_width, L32_tag_width, true, false, 1,
                     AllocCond::FINAL_MISPRED, false, true, false, success_reset_strategy>;

#endif
