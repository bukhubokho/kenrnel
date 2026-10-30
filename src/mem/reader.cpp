// language: C++17, target: Android arm64
#include "reader.hpp"

#ifdef USE_PROC_MEM
// ════════════════════════════════════════════════════════════════════════════
// External backend — /proc/<pid>/mem  +  pread64
// No kernel driver needed.  Requires root (KernelSU / Magisk).
// ════════════════════════════════════════════════════════════════════════════
#include <fcntl.h>
#include <unistd.h>
#include <cstdio>

namespace mem {

int open(uint32_t pid) {
    if (pid == 0) return -1;
    char path[64];
    snprintf(path, sizeof(path), "/proc/%u/mem", pid);
    // O_RDONLY — write not needed for ESP read path
    int fd = ::open(path, O_RDONLY);
    return fd; // -1 on fail, caller checks
}

void close(int fd) {
    if (fd >= 0) ::close(fd);
}

bool set_pid(int /*fd*/, uint32_t /*pid*/, const char* /*pkg*/) {
    return true; // pread uses the fd path — no driver notification needed
}

bool read(int fd, uint32_t /*pid*/, uint64_t addr, void* buf, size_t size) {
    if (fd < 0 || !buf || size == 0) return false;
    ssize_t n = pread64(fd, buf, size, (off64_t)addr);
    return n == (ssize_t)size;
}

} // namespace mem

#else
// ════════════════════════════════════════════════════════════════════════════
// Kernel backend — RTkernel IOCTL
// ════════════════════════════════════════════════════════════════════════════
#include "../driver/driver.h"

namespace mem {

int open(uint32_t /*pid*/) {
    return rt_open(); // scans /dev/ for BY* device
}

void close(int fd) {
    rt_close(fd);
}

bool set_pid(int fd, uint32_t pid, const char* pkg) {
    return rt_set_pid_ok(fd, pid, pkg);
}

bool read(int fd, uint32_t pid, uint64_t addr, void* buf, size_t size) {
    return rt_read(fd, pid, addr, buf, size);
}

} // namespace mem
#endif
