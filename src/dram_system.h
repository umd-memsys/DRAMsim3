#ifndef __DRAM_SYSTEM_H
#define __DRAM_SYSTEM_H

#include <string>
#include <vector>

#include "common.h"
#include "configuration.h"
#include "controller.h"
#include "memory_system.h"
#include "statistics.h"
#include "timing.h"

#ifdef THERMAL
#include "thermal.h"
#endif  // THERMAL

namespace dramcore{

class BaseDRAMSystem {
   public:
    BaseDRAMSystem(const std::string &config_file,
                     const std::string &output_dir,
                     std::function<void(uint64_t)> read_callback,
                     std::function<void(uint64_t)> write_callback);
    virtual ~BaseDRAMSystem();
    virtual bool IsReqInsertable(uint64_t hex_addr, bool is_write) = 0;
    virtual bool InsertReq(uint64_t hex_addr, bool is_write) = 0;
    virtual void ClockTick() = 0;
    virtual void PrintIntermediateStats();
    virtual void PrintStats();
    std::function<void(uint64_t req_id)> read_callback_, write_callback_;
    std::vector<Controller *> ctrls_;
    Config *ptr_config_;
    static int num_mems_;  // a lot of CPU sims create a JedecDRAMSystem for each
                           // channel, oh well..

   protected:
    uint64_t clk_;
    uint64_t id_;
    uint64_t last_req_clk_;
    Timing *ptr_timing_;
    Statistics *ptr_stats_;
#ifdef THERMAL
    ThermalCalculator *ptr_thermCal_;
#endif  // THERMAL
    int mem_sys_id_;

    // Stats output files
    std::ofstream stats_file_;
    std::ofstream epoch_stats_file_;
    std::ofstream stats_file_csv_;
    std::ofstream epoch_stats_file_csv_;
    std::ofstream histo_stats_file_csv_;
#ifdef GENERATE_TRACE
    std::ofstream address_trace_;
#endif  // GENERATE_TRACE
};

// hmmm not sure this is the best naming...
class JedecDRAMSystem : public BaseDRAMSystem {
   public:
    JedecDRAMSystem(const std::string &config_file, const std::string &output_dir,
                 std::function<void(uint64_t)> read_callback,
                 std::function<void(uint64_t)> write_callback);
    ~JedecDRAMSystem();
    bool IsReqInsertable(uint64_t hex_addr, bool is_write) override;
    bool InsertReq(uint64_t hex_addr, bool is_write) override;
    void ClockTick() override;

#ifdef NO_BACKPRESSURE
   private:
    std::list<Request *> buffer_q_;
#endif
};

// Model a memorysystem with an infinite bandwidth and a fixed latency (possibly
// zero) To establish a baseline for what a 'good' memory standard can and
// cannot do for a given application
class IdealDRAMSystem : public BaseDRAMSystem {
   public:
    IdealDRAMSystem(const std::string &config_file,
                      const std::string &output_dir,
                      std::function<void(uint64_t)> read_callback,
                      std::function<void(uint64_t)> write_callback);
    ~IdealDRAMSystem();
    bool IsReqInsertable(uint64_t hex_addr, bool is_write) override {
        return true;
    };
    bool InsertReq(uint64_t hex_addr, bool is_write) override;
    void ClockTick() override;

   private:
    int latency_;
    std::list<Request *> infinite_buffer_q_;
};

}  // namespace dramcore
#endif  // __DRAM_SYSTEM_H

