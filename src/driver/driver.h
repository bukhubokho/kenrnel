#pragma once
#include <cstdint>
#include <cstddef>

// RTkernel IOCTL codes (reversed from 5.10.sh driver binary)
#define IOCTL_SET_PID   0x803   // find process, store mm — bidirectional 24-byte struct
#define IOCTL_READ_MEM  0x801   // read cross-process memory
#define IOCTL_WRITE_MEM 0x802   // write cross-process memory

// cmd=0x801 / 0x802: both use same 32-byte layout
struct rt_rw_args {
    uint32_t pid;
    uint32_t _pad;
    uint64_t addr;      // virtual address in target process
    uint64_t buf;       // userspace buffer ptr (output for read, input for write)
    uint64_t size;      // byte count
};

// cmd=0x803: in: pid+name_ptr, out: base written back at offset 16
struct rt_pid_args {
    uint32_t pid;
    uint32_t _pad;
    uint64_t name_ptr;  // userspace ptr to process name string
    uint64_t base;      // [out] first VMA start (module base)
};

// Scan /dev/ for RTkernel device (BY* prefix from driver source)
// Returns fd >= 0 or -1 on failure
int rt_open();
void rt_close(int fd);

// Set target process — call once after finding FF pid
// name = "com.dts.freefireth", returns base address or 0 on fail
uint64_t rt_set_pid(int fd, uint32_t pid, const char* name);

// Read size bytes from target process at addr into buf
bool rt_read(int fd, uint32_t pid, uint64_t addr, void* buf, size_t size);

// Write size bytes from buf to target process at addr
bool rt_write(int fd, uint32_t pid, uint64_t addr, const void* buf, size_t size);

// Convenience template
template<typename T>
inline T rt_read_t(int fd, uint32_t pid, uint64_t addr) {
    T v{};
    rt_read(fd, pid, addr, &v, sizeof(T));
    return v;
}
