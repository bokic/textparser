#include "os.h"

#ifndef _WIN32
#error Not a Windows target
#endif

#include <Windows.h>
#include <Winbase.h>
#include <Fileapi.h>

static wchar_t* utf8_to_wchar(const char* utf8)
{
    if (!utf8) return NULL;
    int req_len = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);
    if (req_len <= 0) return NULL;
    wchar_t* wstr = (wchar_t*)HeapAlloc(GetProcessHeap(), 0, (size_t)req_len * sizeof(wchar_t));
    if (!wstr) return NULL;
    if (MultiByteToWideChar(CP_UTF8, 0, utf8, -1, wstr, req_len) <= 0) {
        HeapFree(GetProcessHeap(), 0, wstr);
        return NULL;
    }
    return wstr;
}

file_hnd_fd os_creat_file(const char* pathname)
{
    wchar_t* wpath = utf8_to_wchar(pathname);
    if (!wpath) return INVALID_HANDLE_VALUE;
    HANDLE hnd = CreateFileW(wpath, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, NULL);
    HeapFree(GetProcessHeap(), 0, wpath);
    return hnd;
}

void os_file_close(file_hnd_fd hnd_fd)
{
    CloseHandle(hnd_fd);
}

ssize_t os_write(file_hnd_fd hnd_fd, const void *buffer, size_t len)
{
    size_t total_written = 0;
    const char *ptr = (const char *)buffer;

    while (total_written < len) {
        DWORD chunk = 0;
        size_t remaining = len - total_written;
        DWORD to_write = (remaining > MAXDWORD) ? MAXDWORD : (DWORD)remaining;

        if (!WriteFile(hnd_fd, ptr + total_written, to_write, &chunk, NULL)) {
            return -1;
        }
        if (chunk == 0) {
            break;
        }
        total_written += chunk;
    }

    return (ssize_t)total_written;
}

void* os_map(const char *pathname, size_t* size)
{
    void* ret = NULL;
    LARGE_INTEGER fileSize;

    wchar_t* wpath = utf8_to_wchar(pathname);
    if (!wpath) return NULL;

    HANDLE hnd = CreateFileW(wpath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    HeapFree(GetProcessHeap(), 0, wpath);
    if (hnd == INVALID_HANDLE_VALUE)
        return NULL;

    if (!GetFileSizeEx(hnd, &fileSize)) {
        CloseHandle(hnd);
        return NULL;
    }

    if (fileSize.QuadPart <= 0) {
        CloseHandle(hnd);
        return NULL;
    }

    if ((ULONGLONG)fileSize.QuadPart > (ULONGLONG)(size_t)-1) {
        CloseHandle(hnd);
        return NULL;
    }

    *size = (size_t)fileSize.QuadPart;

    HANDLE map_hnd = CreateFileMapping(hnd, NULL, PAGE_READONLY, 0, 0, NULL);
    if (map_hnd == NULL) {
        CloseHandle(hnd);
        return NULL;
    }

    ret = MapViewOfFile(map_hnd, FILE_MAP_READ, 0, 0, 0);

    CloseHandle(map_hnd);
    CloseHandle(hnd);

    return ret;
}

void os_unmap(void* addr, size_t size)
{
    (void)size;
    UnmapViewOfFile(addr);
}

ssize_t os_write_to_terminal(const void *buffer, size_t len)
{
    HANDLE hnd = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hnd == INVALID_HANDLE_VALUE || hnd == NULL) {
        return -1;
    }

    DWORD mode;
    if (GetConsoleMode(hnd, &mode)) {
        DWORD written = 0;
        if (WriteConsole(hnd, buffer, (DWORD)len, &written, NULL)) {
            return (ssize_t)written;
        }
    }

    return os_write(hnd, buffer, len);
}

void os_file_cleanup(void *fd) {
    HANDLE *hnd = (HANDLE *) fd;
    if (hnd) {
        CloseHandle(*hnd);
        *hnd = 0;
    }
}

