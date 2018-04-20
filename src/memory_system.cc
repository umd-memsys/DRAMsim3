#include "memory_system.h"

namespace dramcore {
MemorySystem::MemorySystem(const std::string &config_file,
                           const std::string &output_dir,
                           std::function<void(uint64_t)> read_callback,
                           std::function<void(uint64_t)> write_callback) {
    Config *config = new Config(config_file, output_dir);
    // TODO: ideal memory type?
    if (config->IsHMC()) {
        dram_system_ = new HMCMemorySystem(config_file, output_dir,
                                           read_callback, write_callback);
    } else {
        dram_system_ = new JedecDRAMSystem(config_file, output_dir,
                                           read_callback, write_callback);
    }

    delete (config);
}

BaseDRAMSystem *MemorySystem::GetDRAMSystem() { return dram_system_; }

}  // namespace dramcore

// This function can be used by autoconf AC_CHECK_LIB since
// apparently it can't detect C++ functions.
// Basically just an entry in the symbol table
extern "C" {
void libdramcore_is_present(void) { ; }
}
