#ifndef __BP_HH__
#define __BP_HH__

#include <bitset>
#include <cstddef>
#include <cstdint>
#include <fmt/core.h>
#include <iostream>
#include <stdint.h>
#include <string>

#include "util.hh"

class IPredictor
{
  public:
    virtual const std::string &getName() = 0;
    virtual void statistic() = 0;
};

class IDirectionPredictor : public IPredictor
{
  private:
    virtual bool predict(uint64_t ip) = 0;
    virtual void update(uint64_t ip, bool taken) = 0;

  private:
    uint64_t pred_cnt = 0;
    uint64_t correct_cnt = 0;

  public:
    virtual ~IDirectionPredictor() = default;

    void checkPred(uint64_t ip, bool taken)
    {
        pred_cnt++;
        correct_cnt += predict(ip) == taken;
        update(ip, taken);
    }

    void statistic() override
    {
        fmt::print("{} prediction accuracy = {} / {} = {}%\n", getName(), correct_cnt, pred_cnt,
                   (double)correct_cnt / pred_cnt * 100);
    }
};

struct BranchTarget
{
    uint64_t addr;
};
class ITargetPredictor : public IPredictor
{

  private:
    virtual BranchTarget *predict(uint64_t ip) = 0;
    virtual void update(uint64_t ip, BranchTarget target) = 0;

  private:
    uint64_t pred_cnt = 0;
    uint64_t ct_cnt = 0; // control transfer
    uint64_t mishit_cnt = 0;
    uint64_t correct_cnt = 0;

  public:
    virtual ~ITargetPredictor() = default;

    void checkPred(uint64_t ip, bool is_ct_inst, uint64_t addr)
    {
        pred_cnt++;
        auto pred = predict(ip);
        if (is_ct_inst)
        {
            ct_cnt++;
            correct_cnt += pred && pred->addr == addr;
            BranchTarget target = {addr};
            update(ip, target);
        }
        else if (pred)
            mishit_cnt++;
    }

    void statistic() override
    {
        auto nonct_inst = pred_cnt - ct_cnt;
        fmt::print("{}\n\t prediction accuracy = {} / {} = {}%\n", getName(), correct_cnt, ct_cnt,
                   (double)correct_cnt / ct_cnt * 100);
        fmt::print("\t mishit rate = {} / {} = {}%\n", mishit_cnt, nonct_inst, (double)mishit_cnt / nonct_inst * 100);
    }
};

class ICallReturnPredictor : public IPredictor
{
  private:
    virtual uint64_t pop() = 0;

  private:
    uint64_t pred_cnt = 0;
    uint64_t correct_cnt = 0;

  public:
    virtual ~ICallReturnPredictor() = default;

    virtual void push(uint64_t addr) = 0;

    void checkPred(uint64_t addr)
    {
        pred_cnt++;
        correct_cnt += pop() == addr;
    }

    void statistic() override
    {
        fmt::print("{} prediction accuracy = {} / {} = {}%\n", getName(), correct_cnt, pred_cnt,
                   (double)correct_cnt / pred_cnt * 100);
    }
};

template <size_t width, bool RSHIFT_TO_DECRE = false>
struct Counter : public std::bitset<width>
{
    size_t max = bitmask(width);

    Counter &operator++()
    {
        if (this->to_ulong() < max)
            *this = Counter(this->to_ulong() + 1);
        return *this;
    }
    Counter &operator--()
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