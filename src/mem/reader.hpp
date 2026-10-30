#pragma once
// language: C++17, target: Android arm64
// Unified memory-read abstraction.
// Build with -DUSE_PROC_MEM  → external backend (/proc/pid/mem + pread64, no driver)
// Build without             → kernel  backend (RTkernel IOCTL)
//
// Interface is identical for both — fd meaning differs:
//   external : fd = open("/proc/<pid>/mem", O_RDONLY)
//   kernel   : fd = RTkernel device fd from rt_open()

#include <cstdint>
#include <cstddef>

namespace mem {

// Open the read channel.
//   external: opens /proc/<pid>/mem — pid required.
//   kernel  : scans /dev/ for RTkernel device — pid ignored (pass 0).
// Returns fd >= 0 on success, -1 on failure.
int  open(uint32_t pid = 0);

// Close the fd returned by open().
void close(int fd);

// Inform the backend about the target process.
//   kernel  : sends IOCTL_SET_PID to the driver.
//   external: no-op (pread works without notification), returns true.
bool set_pid(int fd, uint32_t pid, const char* pkg);

// Read `size` bytes from virtual address `addr` in the target process into `buf`.
bool read(int fd, uint32_t pid, uint64_t addr, void* buf, size_t size);

// Typed convenience wrapper.
template<typename T>
inline T read_t(int fd, uint32_t pid, uint64_t addr) {
    T v{};
    read(fd, pid, addr, &v, sizeof(T));
    return v;
}

} // namespace mem
