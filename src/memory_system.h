#ifndef __MEMORY_SYSTEM__H
#define __MEMORY_SYSTEM__H

#include <functional>
#include <string>
#include <vector>

#include "configuration.h"
#include "dram_system.h"
#include "hmc.h"

namespace dramcore {

// This should be the interface class that deals with CPU
class MemorySystem {
   public:
    MemorySystem(const std::string &config_file, const std::string &output_dir,
                 std::function<void(uint64_t)> read_callback,
                 std::function<void(uint64_t)> write_callback);
    ~MemorySystem() { delete (dram_system_); }
    BaseDRAMSystem *GetDRAMSystem();

   private:
    BaseDRAMSystem *dram_system_;
    int num_mems_;
};

}  // namespace dramcore

#endif
