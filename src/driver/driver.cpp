#include "driver.h"
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <cstring>
#include <cstdio>
#include <cerrno>
#include <sys/stat.h>

// Probe a single fd: send IOCTL_SET_PID with pid=0 and see if driver responds
// RTkernel returns 0 or -ESRCH (no such process) — anything but ENOTTY/ENOIOCTLCMD
static bool probe_fd(int fd) {
    rt_pid_args args{};
    args.pid = 0;
    char name[] = "";
    args.name_ptr = (uint64_t)(uintptr_t)name;
    int r = ioctl(fd, IOCTL_SET_PID, &args);
    int e = errno;
    // ENOTTY / EINVAL on wrong device; driver returns 0 or ESRCH/EINVAL but not ENOTTY
    if (r == 0) return true;
    if (e == ENOTTY) return false;   // not our driver
    if (e == ENXIO)  return false;
    // ESRCH, EFAULT, EINVAL — driver handled the ioctl (pid 0 = no process found)
    return true;
}

int rt_open() {
    DIR* d = opendir("/dev");
    if (!d) return -1;

    char path[128];
    struct dirent* ent;
    int found = -1;

    while ((ent = readdir(d)) && found < 0) {
        // skip . and ..
        if (ent->d_name[0] == '.') continue;
        // only check char devices (DT_CHR) or unknown type (DT_UNKNOWN)
        if (ent->d_type != DT_CHR && ent->d_type != DT_UNKNOWN) continue;
        // skip well-known system devices to avoid side effects
        const char* skip[] = {
            "null","zero","full","random","urandom","kmsg","ptmx","tty",
            "mem","kmem","port","console","binder","hwbinder","vndbinder",
            "ashmem","ion","fuse","input","snd","video","block",nullptr
        };
        bool skip_it = false;
        for (int i = 0; skip[i]; i++) {
            if (strncmp(ent->d_name, skip[i], strlen(skip[i])) == 0) {
                skip_it = true; break;
            }
        }
        if (skip_it) continue;

        snprintf(path, sizeof(path), "/dev/%s", ent->d_name);
        int fd = open(path, O_RDWR | O_NONBLOCK);
        if (fd < 0) continue;

        struct stat st{};
        fstat(fd, &st);
        if (!S_ISCHR(st.st_mode)) { close(fd); continue; }

        if (probe_fd(fd)) {
            fprintf(stderr, "[DRV] found RTkernel device: %s\n", path);
            found = fd;
        } else {
            close(fd);
        }
    }
    closedir(d);
    return found;
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

bool rt_set_pid_ok(int fd, uint32_t pid, const char* name) {
    rt_pid_args args{};
    args.pid      = pid;
    args.name_ptr = (uint64_t)(uintptr_t)name;
    return ioctl(fd, IOCTL_SET_PID, &args) >= 0;
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
