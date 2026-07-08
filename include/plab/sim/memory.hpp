#ifndef PLAB_SIM_MEMORY_HPP
#define PLAB_SIM_MEMORY_HPP

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace plab::sim {

/// Thrown on an unaligned or out-of-bounds data-memory access.
class MemoryError : public std::runtime_error {
 public:
  explicit MemoryError(const std::string& what) : std::runtime_error(what) {}
};

/// Word-granular, byte-addressed data memory. Accesses must be 4-byte aligned
/// and within bounds. Single-cycle ("magic") memory: no latency modelling.
class DataMemory {
 public:
  explicit DataMemory(std::uint32_t sizeBytes = 4096)
      : words_(sizeBytes / 4, 0), sizeBytes_(sizeBytes - (sizeBytes % 4)) {}

  std::int32_t loadWord(std::uint32_t addr) const {
    checkAccess(addr);
    return words_[addr / 4];
  }

  void storeWord(std::uint32_t addr, std::int32_t value) {
    checkAccess(addr);
    words_[addr / 4] = value;
  }

  std::uint32_t sizeBytes() const noexcept { return sizeBytes_; }
  const std::vector<std::int32_t>& raw() const noexcept { return words_; }

  bool operator==(const DataMemory& other) const { return words_ == other.words_; }

 private:
  void checkAccess(std::uint32_t addr) const {
    if (addr % 4 != 0) {
      throw MemoryError("unaligned data access at address " + std::to_string(addr));
    }
    if (addr + 4 > sizeBytes_) {
      throw MemoryError("data access out of bounds at address " + std::to_string(addr));
    }
  }

  std::vector<std::int32_t> words_;
  std::uint32_t sizeBytes_;
};

}  // namespace plab::sim

#endif  // PLAB_SIM_MEMORY_HPP
