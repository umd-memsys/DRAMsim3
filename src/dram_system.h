#ifndef __DRAM_SYSTEM_H
#define __DRAM_SYSTEM_H

#include <fstream>
#include <string>
#include <vector>

#include "common.h"
#include "configuration.h"
#include "controller.h"
#include "timing.h"

#ifdef THERMAL
#include "thermal.h"
#endif  // THERMAL

namespace dramsim3 {

class BaseDRAMSystem {
   public:
    BaseDRAMSystem(Config &config, const std::string &output_dir,
                   std::function<void(uint64_t)> read_callback,
                   std::function<void(uint64_t)> write_callback);
    virtual ~BaseDRAMSystem() {}
    void RegisterCallbacks(std::function<void(uint64_t)> read_callback,
                           std::function<void(uint64_t)> write_callback);
    void PrintEpochStats();
    void PrintStats();
    void ResetStats();

    virtual bool WillAcceptTransaction(uint64_t hex_addr,
                                       bool is_write) const = 0;
    virtual bool WillAcceptTransaction(uint64_t hex_addr,
                                       bool is_write, bool is_MRS) const = 0;                                       
    virtual bool AddTransaction(uint64_t hex_addr, bool is_write, bool is_MRS) = 0;
    virtual bool AddTransaction(uint64_t hex_addr, bool is_write, bool is_MRS, std::vector<u_int64_t> &payload) = 0;
    virtual void ClockTick() = 0;
    int GetChannel(uint64_t hex_addr) const;
    virtual std::vector<uint64_t> GetRespData(uint64_t hex_addr) = 0;

    std::function<void(uint64_t req_id)> read_callback_, write_callback_;
    static int total_channels_;

   protected:
    uint64_t id_;
    uint64_t last_req_clk_;
    Config &config_;
    Timing timing_;
    uint64_t parallel_cycles_;
    uint64_t serial_cycles_;

#ifdef THERMAL
    ThermalCalculator thermal_calc_;
#endif  // THERMAL

    uint64_t clk_;
    std::vector<Controller*> ctrls_;

#ifdef ADDR_TRACE
    std::ofstream address_trace_;
#endif  // ADDR_TRACE
};

// hmmm not sure this is the best naming...
class JedecDRAMSystem : public BaseDRAMSystem {
   public:
    JedecDRAMSystem(Config &config, const std::string &output_dir,
                    std::function<void(uint64_t)> read_callback,
                    std::function<void(uint64_t)> write_callback);
    ~JedecDRAMSystem();
    bool WillAcceptTransaction(uint64_t hex_addr, bool is_write) const override;
    bool WillAcceptTransaction(uint64_t hex_addr, bool is_write, bool is_MRS) const override;
    bool AddTransaction(uint64_t hex_addr, bool is_write, bool is_MRS) override;
    bool AddTransaction(uint64_t hex_addr, bool is_write, bool is_MRS, std::vector<u_int64_t> &payload) override;
    void ClockTick() override;
    std::vector<uint64_t> GetRespData(uint64_t hex_addr) override;
};

// Model a memorysystem with an infinite bandwidth and a fixed latency (possibly
// zero) To establish a baseline for what a 'good' memory standard can and
// cannot do for a given application
class IdealDRAMSystem : public BaseDRAMSystem {
   public:
    IdealDRAMSystem(Config &config, const std::string &output_dir,
                    std::function<void(uint64_t)> read_callback,
                    std::function<void(uint64_t)> write_callback);
    ~IdealDRAMSystem();
    bool WillAcceptTransaction(uint64_t hex_addr,
                               bool is_write) const override {
        return true;
    };
    bool WillAcceptTransaction(uint64_t hex_addr,
                               bool is_write, bool is_MRS) const override {
        return true;
    };

    bool AddTransaction(uint64_t hex_addr, bool is_write, bool is_MRS) override;
    bool AddTransaction(uint64_t hex_addr, bool is_write, bool is_MRS, std::vector<u_int64_t> &payload) override;
    void ClockTick() override;
    std::vector<uint64_t> GetRespData(uint64_t hex_addr) override;

   private:
    int latency_;
    std::vector<Transaction> infinite_buffer_q_;
};

}  // namespace dramsim3
#endif  // __DRAM_SYSTEM_H
