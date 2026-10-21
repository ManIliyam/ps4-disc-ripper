// PS4 Disc Ripper: phase-2 filesystem diagnostic. No extraction or device I/O.
#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <orbis/libkernel.h>
#include <orbis/Net.h>
#include <sys/socket.h>
#include <initializer_list>
#ifdef MEDIA_RIPPER_GUI
#include <sstream>
#include <thread>
#include <mutex>
#include <exception>
#include <atomic>
#include <chrono>
#include <orbis/Pad.h>
#include <orbis/UserService.h>
#include <orbis/NetCtl.h>
#include "graphics.h"
#include "GuiModel.h"
#include "FtpInspect.h"
#include "StreamCopy.h"
#include "StorageSpace.h"
std::stringstream debugLogStream;
#endif

namespace {
int logFd = -1;
unsigned entries = 0, samples = 0;
bool failed = false;
#ifdef MEDIA_RIPPER_GUI
void guiRecordLog(const char* line);
#endif
// Keep the process alive like the upstream sample, without its busy loop.
// This tests the main-return crash hypothesis; it is not a confirmed crash fix.
[[noreturn]] void park() {
    for (;;) sceKernelUsleep(1000000);
}
void logf(const char* format, ...) {
    char text[8192];
    va_list args;
    va_start(args, format);
    const int n = std::vsnprintf(text, sizeof(text), format, args);
    va_end(args);
    if (n < 0 || static_cast<size_t>(n) >= sizeof(text)) { failed = true; return; }
#ifdef MEDIA_RIPPER_GUI
    guiRecordLog(text);
#endif
    if (failed) return;
    if (logFd < 0) { failed = true; return; }
    size_t offset = 0;
    unsigned interrupts = 0;
    while (offset < static_cast<size_t>(n)) {
        const ssize_t wrote = write(logFd, text + offset, static_cast<size_t>(n) - offset);
        if (wrote < 0 && errno == EINTR && ++interrupts < 3) continue;
        if (wrote <= 0) { failed = true; return; }
        offset += static_cast<size_t>(wrote);
        interrupts = 0;
    }
}
void error(const char* op, const char* path, int code) {
    logf("ERROR %s path=%s errno=%d (%s)\n", op, path, code, std::strerror(code));
}
void nativeProbe(const char* path, bool readPrefix) {
    logf("NATIVE_OPEN_BEGIN path=%s\n", path);
    const int fd = sceKernelOpen(path, O_RDONLY | O_NOFOLLOW | O_NONBLOCK, 0);
    logf("NATIVE_OPEN_RESULT path=%s result=%d raw=0x%08x\n", path, fd, static_cast<unsigned>(fd));
    if (fd < 0) return; // Native return codes are not libc errno values.
    if (readPrefix) {
        unsigned char buffer[4096];
        logf("NATIVE_READ_BEGIN path=%s\n", path);
        // OpenOrbis declares size_t: preserve the raw result, including errors.
        const size_t n = sceKernelRead(fd, buffer, sizeof(buffer));
        logf("NATIVE_READ_RESULT path=%s raw=0x%llx within_buffer=%s\n", path,
             static_cast<unsigned long long>(n), n <= sizeof(buffer) ? "yes" : "no");
    }
    logf("NATIVE_CLOSE_BEGIN path=%s\n", path);
    const int result = sceKernelClose(fd);
    logf("NATIVE_CLOSE_RESULT result=%d raw=0x%08x\n", result, static_cast<unsigned>(result));
}
bool directory(const char* path) {
    const int result = mkdir(path, 0755);
    const int e = errno;
    logf("STARTUP mkdir path=%s result=%d errno=%d\n", path, result, result ? e : 0);
    // Avoid depending on libc struct stat layout for this startup gate.
    // The descriptor flags verify a directory and reject a final symlink.
    const int check = open(path, O_RDONLY | O_DIRECTORY | O_NOFOLLOW);
    const int checkError = check < 0 ? errno : 0;
    logf("STARTUP open directory path=%s result=%d errno=%d\n", path, check, checkError);
    if (check < 0) return false;
    const int closed = close(check);
    logf("STARTUP close directory result=%d errno=%d\n", closed, closed ? errno : 0);
    return closed == 0;
}
// Escape names so control characters cannot forge log lines.
void name(const char* path) {
    for (const unsigned char* p = reinterpret_cast<const unsigned char*>(path); *p; ++p) {
        if (*p < 32 || *p >= 127 || *p == '\\') logf("\\x%02x", *p);
        else logf("%c", *p);
    }
}
#include "TestArchive.h"
#include "FtpProbe.h"
void walk(const char* path, unsigned depth, bool sampleFiles) {
    DIR* dir = opendir(path);
    if (!dir) { error("opendir", path, errno); return; }
    for (;;) {
        if (entries >= 2000) { logf("LIMIT entries=2000; inventory incomplete\n"); break; }
        errno = 0;
        dirent* ent = readdir(dir);
        if (!ent) { if (errno) error("readdir", path, errno); break; }
        if (!std::strcmp(ent->d_name, ".") || !std::strcmp(ent->d_name, "..")) continue;
        ++entries;
        char child[4096];
        const int n = std::snprintf(child, sizeof(child), "%s/%s", path, ent->d_name);
        if (n < 0 || static_cast<size_t>(n) >= sizeof(child)) { error("path length", path, ENAMETOOLONG); continue; }
        struct stat st{};
        if (lstat(child, &st)) { error("lstat", child, errno); continue; }
        logf("ENTRY mode=%lo size=%lld path=", static_cast<unsigned long>(st.st_mode), static_cast<long long>(st.st_size));
        name(child); logf("\n");
        // Never follow links or open special files. Prefix reads stay in memory.
        if (sampleFiles && S_ISREG(st.st_mode) && samples < 16) {
            ++samples;
            int fd = open(child, O_RDONLY | O_NOFOLLOW | O_NONBLOCK);
            if (fd < 0) { error("open read-only", child, errno); continue; }
            struct stat current{};
            if (fstat(fd, &current) != 0) error("fstat", child, errno);
            else if (S_ISREG(current.st_mode)) {
                unsigned char buffer[4096];
                ssize_t got;
                unsigned retry = 0;
                do { got = read(fd, buffer, sizeof(buffer)); } while (got < 0 && errno == EINTR && ++retry < 3);
                if (got < 0) error("read prefix", child, errno);
                else logf("PREFIX_READ bytes=%lld (does not prove full-file readability)\n", static_cast<long long>(got));
            }
            if (close(fd)) error("close", child, errno);
        }
        if (sampleFiles && S_ISDIR(st.st_mode)) {
            if (depth < 4) walk(child, depth + 1, true);
            else logf("LIMIT depth=4; subtree omitted\n");
        }
    }
    if (closedir(dir)) error("closedir", path, errno);
}
#ifdef MEDIA_RIPPER_GUI
#include "Gui.h"
#endif
}

int main() {
#ifdef MEDIA_RIPPER_GUI
    runGui();
#endif
    // Bootstrap directly under the known writable /data before any mkdir/stat.
    // Preserve this separate log even if normal log-folder setup fails.
    char bootstrap[256];
    for (unsigned i = 0; i < 1000; ++i) {
        std::snprintf(bootstrap, sizeof(bootstrap), "/data/MediaRipper-startup-%lld-%03u.log", static_cast<long long>(std::time(nullptr)), i);
        logFd = open(bootstrap, O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW, 0644);
        if (logFd >= 0 || errno != EEXIST) break;
    }
    if (logFd < 0) park();
    logf("PS4 Disc Ripper diagnostic v0.6 STARTUP\n");
    if (!directory("/data/MediaRipper") || !directory("/data/MediaRipper/logs")) {
        logf("STARTUP directory setup failed; sleeping, no disc checks performed\n");
        fsync(logFd); close(logFd); park();
    }
    char output[256];
    int fd = -1;
    for (unsigned i = 0; i < 1000; ++i) {
        std::snprintf(output, sizeof(output), "/data/MediaRipper/logs/probe-%lld-%03u.log", static_cast<long long>(std::time(nullptr)), i);
        fd = open(output, O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW, 0644);
        if (fd >= 0 || errno != EEXIST) break;
    }
    if (fd < 0) {
        logf("STARTUP normal log creation failed errno=%d\n", errno);
        fsync(logFd); close(logFd); park();
    }
    logf("STARTUP normal log opened path=%s; switching logs\n", output);
    fsync(logFd);
    close(logFd);
    logFd = fd;
    logf("PS4 Disc Ripper diagnostic v0.6\nFirmware/GoldHEN/model: record manually in test worksheet.\nNo mounts, ioctls, raw device opens or privilege changes.\nControlled extraction: 8192-byte test IFO only; exclusive folder, write/readback verification.\n");
    logf("BEGIN local GoldHEN FTP experiment\n");
    ftpProbe();
    logf("END local FTP experiment\n");
    logf("BEGIN namespace controls; root inventory limited to one level\n");
    walk("/", 0, false);
    entries = 0;
    nativeProbe("/", false);
    nativeProbe("/app0", false);
    nativeProbe("/data", false);
    nativeProbe("/mnt", false);
    nativeProbe("/mnt/disc", false);
    nativeProbe("/mnt/disc/VIDEO_TS/VIDEO_TS.IFO", true);
    // /mnt/disc is source-backed, but existence alone never establishes a mounted disc.
    logf("BEGIN /mnt inventory (one level; not a mount-table query)\n");
    walk("/mnt", 0, false);
    entries = 0;
    logf("BEGIN /mnt/disc bounded inventory and prefix reads\n");
    walk("/mnt/disc", 0, true);
    logf("END entries=%u sampled=%u; filesystem/profile/label/capacity UNKNOWN\n", entries, samples);
    logf("STAGE before log fsync\n");
    const int syncResult = fsync(fd);
    const int syncError = syncResult ? errno : 0;
    logf("STAGE fsync result=%d errno=%d; next close log and sleep; close via PS menu\n", syncResult, syncError);
    if (syncResult) failed = true;
    if (close(fd)) failed = true;
    std::printf("Diagnostic %s: %s\n", failed ? "log write failed" : "finished", output);
    park();
}
