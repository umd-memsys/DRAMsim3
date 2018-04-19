#ifndef __MEMORY_SYSTEM__H
#define __MEMORY_SYSTEM__H

#include <functional>
#include <string>
#include <vector>


namespace dramcore {

// This should be the interface class that deals with CPU
class MemorySystem {
 public:
    MemorySystem(const std::string &config_file,
                 const std::string &output_dir,
                 std::function<void(uint64_t)> read_callback,
                 std::function<void(uint64_t)> write_callback);
    ~MemorySystem();
    virtual bool IsReqInsertable(uint64_t hex_addr, bool is_write) = 0;
    virtual bool InsertReq(uint64_t hex_addr, bool is_write) = 0;
    virtual void ClockTick() = 0;
    virtual void PrintIntermediateStats();
    virtual void PrintStats();
    std::function<void(uint64_t req_id)> read_callback_, write_callback_;   
 private:
    int num_mems_;
};

}  // namespace dramcore

#endif
