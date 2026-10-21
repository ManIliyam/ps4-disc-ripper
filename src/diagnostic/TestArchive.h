// Small controlled storage test, not the general streaming extraction engine.
bool saveTestIfo(const unsigned char* contents, unsigned size) {
    // This exact 8192-byte test fixture was read successfully in v0.5.
    if (size != 8192 || std::memcmp(contents, "DVDVIDEO-VMG", 12)) {
        logf("ARCHIVE rejected: expected test fixture size=8192 and DVDVIDEO-VMG header\n");
        return false;
    }
    char folder[256];
    bool created = false;
    for (unsigned i = 0; i < 1000; ++i) {
        std::snprintf(folder, sizeof(folder), "/data/MediaRipper/IFO_TEST_%lld_%03u",
                      static_cast<long long>(std::time(nullptr)), i);
        if (mkdir(folder, 0755) == 0) { created = true; break; }
        if (errno != EEXIST) { error("archive mkdir", folder, errno); return false; }
    }
    if (!created) { logf("ARCHIVE cannot allocate unique folder\n"); return false; }
    char video[300], partial[340], finalPath[340];
    std::snprintf(video, sizeof(video), "%s/VIDEO_TS", folder);
    std::snprintf(partial, sizeof(partial), "%s/VIDEO_TS.IFO.partial", video);
    std::snprintf(finalPath, sizeof(finalPath), "%s/VIDEO_TS.IFO", video);
    if (mkdir(video, 0755)) { error("archive VIDEO_TS mkdir", video, errno); return false; }
    // No lstat/fstat: libc's lstat was proven unimplemented on this console.
    int fd = open(partial, O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW, 0644);
    if (fd < 0) { error("archive exclusive open", partial, errno); return false; }
    logf("ARCHIVE writing path=%s bytes=%u\n", partial, size);
    bool ok = true;
    unsigned offset = 0, interruptions = 0;
    while (offset < size) {
        const ssize_t n = write(fd, contents + offset, size - offset);
        if (n < 0 && errno == EINTR && ++interruptions < 3) continue;
        if (n <= 0) {
            error("archive write", partial, n < 0 ? errno : EIO); ok = false; break;
        }
        offset += static_cast<unsigned>(n);
        interruptions = 0;
    }
    if (ok && fsync(fd)) { error("archive fsync", partial, errno); ok = false; }
    if (close(fd)) { error("archive close", partial, errno); ok = false; }
    if (ok) logf("ARCHIVE WRITE_COMPLETE bytes=%u\n", size);
    if (ok) {
        fd = open(partial, O_RDONLY | O_NOFOLLOW);
        if (fd < 0) { error("archive reopen", partial, errno); ok = false; }
        else {
            offset = 0; interruptions = 0;
            unsigned char block[1024];
            while (offset < size) {
                const unsigned want = size - offset < sizeof(block) ? size - offset : sizeof(block);
                const ssize_t n = read(fd, block, want);
                if (n < 0 && errno == EINTR && ++interruptions < 3) continue;
                if (n <= 0) { error("archive verify read", partial, n < 0 ? errno : EIO); ok = false; break; }
                if (std::memcmp(block, contents + offset, static_cast<size_t>(n))) {
                    logf("ARCHIVE mismatch at block offset=%u\n", offset); ok = false; break;
                }
                offset += static_cast<unsigned>(n); interruptions = 0;
            }
            if (ok) {
                ssize_t n; interruptions = 0;
                do { n = read(fd, block, 1); } while (n < 0 && errno == EINTR && ++interruptions < 3);
                if (n != 0) { logf("ARCHIVE verify expected EOF, result=%lld\n", static_cast<long long>(n)); ok = false; }
            }
            if (close(fd)) { error("archive verify close", partial, errno); ok = false; }
        }
    }
    if (ok && rename(partial, finalPath)) { error("archive rename", partial, errno); ok = false; }
    if (!ok) {
        if (unlink(partial)) error("archive partial cleanup", partial, errno);
        logf("ARCHIVE FAILED; no verified archive produced; test folder=%s\n", folder);
        return false;
    }
    logf("ARCHIVE VERIFIED bytes=%u; saved file matches FTP buffer byte-for-byte; exact EOF confirmed\n", size);
    logf("ARCHIVE SAVED path=%s\n", finalPath);
    logf("ARCHIVE scope: one IFO only, not a DVD backup; compare against Windows reference next\n");
    return true;
}
