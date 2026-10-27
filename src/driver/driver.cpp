#include "driver.h"
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <cstring>
#include <cstdio>
#include <cerrno>
#include <sys/stat.h>

// Driver device name uses BY prefix (from get_random_bytes + "BY" author sig)
static const char* RT_PREFIX = "BY";

int rt_open() {
    // Scan /dev/ for BY* device
    DIR* d = opendir("/dev");
    if (!d) return -1;

    char path[64];
    struct dirent* ent;
    int fd = -1;

    while ((ent = readdir(d))) {
        if (strncmp(ent->d_name, RT_PREFIX, 2) == 0 && strlen(ent->d_name) > 2) {
            snprintf(path, sizeof(path), "/dev/%s", ent->d_name);
            fd = open(path, O_RDWR);
            if (fd >= 0) break;
        }
    }
    closedir(d);

    if (fd < 0) {
        // Fallback: check known name if already open
        fd = open("/dev/BYBGBB", O_RDWR);
    }
    return fd;
}

void rt_close(int fd) {
    if (fd >= 0) close(fd);
}

uint64_t rt_set_pid(int fd, uint32_t pid, const char* name) {
    rt_pid_args args{};
    args.pid      = pid;
    args._pad     = 0;
    args.name_ptr = (uint64_t)(uintptr_t)name;
    args.base     = 0;

    int r = ioctl(fd, IOCTL_SET_PID, &args);
    if (r < 0) {
        return 0;
    }
    return args.base;
}

bool rt_read(int fd, uint32_t pid, uint64_t addr, void* buf, size_t size) {
    rt_rw_args args{};
    args.pid  = pid;
    args._pad = 0;
    args.addr = addr;
    args.buf  = (uint64_t)(uintptr_t)buf;
    args.size = (uint64_t)size;

    return ioctl(fd, IOCTL_READ_MEM, &args) == 0;
}

bool rt_write(int fd, uint32_t pid, uint64_t addr, const void* buf, size_t size) {
    rt_rw_args args{};
    args.pid  = pid;
    args._pad = 0;
    args.addr = addr;
    args.buf  = (uint64_t)(uintptr_t)buf;
    args.size = (uint64_t)size;

    return ioctl(fd, IOCTL_WRITE_MEM, &args) == 0;
}
