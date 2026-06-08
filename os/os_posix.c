#if !defined(_POSIX_C_SOURCE) && (defined(__unix__) || defined(__APPLE__))
#define _POSIX_C_SOURCE 200809L
#endif

#include "os.h"
#include <stdio.h>
#include <errno.h>


#ifndef _POSIX_C_SOURCE
#error Not a POSIX target
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>


file_hnd_fd os_creat_file(const char* pathname)
{
    return open(pathname, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
}

void os_file_close(file_hnd_fd hnd_fd)
{
    close(hnd_fd);
}

ssize_t os_write(file_hnd_fd hnd_fd, const void *buffer, size_t len)
{
    size_t total_written = 0;
    const char *ptr = (const char *)buffer;

    while (total_written < len) {
        ssize_t written = write(hnd_fd, ptr + total_written, len - total_written);
        if (written < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        if (written == 0) {
            break;
        }
        total_written += written;
    }

    return (ssize_t)total_written;
}

#include <stdint.h>

void* os_map(const char *pathname, size_t* size)
{
    void* ret = nullptr;
    struct stat stat;
    os_file_defer(fd);

    fd = open(pathname, O_RDONLY);
    if (fd == -1)
        return nullptr;

    if (fstat(fd, &stat))
        return nullptr;

    if (stat.st_size <= 0)
        return nullptr;

    if ((uintmax_t)stat.st_size > (uintmax_t)SIZE_MAX)
        return nullptr;

    ret = mmap(nullptr, (size_t)stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (ret == MAP_FAILED)
        return nullptr;

    *size = (size_t)stat.st_size;

    return ret;
}

void os_unmap(void* addr, size_t size)
{
    munmap(addr, size);
}

ssize_t os_write_to_terminal(const void *buffer, size_t len)
{
    size_t total_written = 0;
    const char *ptr = (const char *)buffer;

    while (total_written < len) {
        ssize_t written = write(STDOUT_FILENO, ptr + total_written, len - total_written);
        if (written < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        if (written == 0) {
            break;
        }
        total_written += written;
    }

    return (ssize_t)total_written;
}

void os_file_cleanup(void *fd) {
    int *fd_int = (int *) fd;
    if (fd_int) {
        close(*fd_int);
        *fd_int = -1;
    }
}
