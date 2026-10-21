#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#ifndef COPY_HOST_TEST
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>
#include <sys/statfs.h>
#endif

// Only the versioned FreeBSD statfs prefix is interpreted; trailing filesystem
// strings differ across PS4/FreeBSD revisions. See docs/VIDEO-COPY.md sources.
struct CopyFsPrefix {
    uint32_t version,type;
    uint64_t flags,blockSize,ioSize,blocks,freeBlocks;
    int64_t availableBlocks;
};
struct CopySpaceUnavailable:std::runtime_error {
    explicit CopySpaceUnavailable(const char* message):std::runtime_error(message){}
};
inline uint64_t copyRequiredSpace(uint64_t fileBytes) {
    const uint64_t reserve=512ull*1024*1024;
    if(!fileBytes || fileBytes>UINT64_MAX-reserve)
        throw std::runtime_error("File size cannot be used for storage preflight");
    return fileBytes+reserve;
}
inline bool copyManualMatches(uint64_t confirmedBytes,uint64_t currentBytes) {
    return confirmedBytes!=0 && confirmedBytes==currentBytes;
}
static_assert(offsetof(CopyFsPrefix,availableBlocks)==48,"statfs prefix ABI");
inline uint64_t copyAvailable(const CopyFsPrefix& s) {
    if(s.version!=0x20030518 || s.blockSize<512 || s.blockSize>1024*1024 ||
       (s.blockSize&(s.blockSize-1)) || !s.blocks || s.freeBlocks>s.blocks ||
       s.availableBlocks<0 || static_cast<uint64_t>(s.availableBlocks)>s.freeBlocks ||
       s.blocks>UINT64_MAX/s.blockSize)
        throw std::runtime_error("Free-space query returned an unsupported or invalid structure");
    return static_cast<uint64_t>(s.availableBlocks)*s.blockSize;
}
#ifndef COPY_HOST_TEST
template<class Log> uint64_t copyStorageSpace(Log log) {
    // The installed SDK's libc fstatfs wrapper jumps directly to libkernel's
    // _fstatfs import. Its sys/statfs.h and bits/statfs.h supply the real type.
    // Unlike statvfs, this wrapper populates the caller's buffer. No dlsym or
    // privileged module loading is required.
    const int fd=open("/data/MediaRipper",O_RDONLY|O_DIRECTORY|O_NOFOLLOW);
    const int openError=fd<0 ? errno : 0;
    log("COPY space open fd=%d errno=%d\n",fd,openError);
    if(fd<0) throw std::runtime_error("Cannot open archive directory for free-space query");
    // Extra trailing space is defensive; only SDK-declared fields are read.
    struct {struct statfs info{};unsigned char padding[16384]{};} storage;
    const int result=fstatfs(fd,&storage.info);
    const int queryError=result ? errno : 0;
    const int closed=close(fd);
    const int closeError=closed ? errno : 0;
    const auto& s=storage.info;
    const CopyFsPrefix prefix{s.f_version,s.f_type,s.f_flags,s.f_bsize,s.f_iosize,
                              s.f_blocks,s.f_bfree,s.f_bavail};
    log("COPY fstatfs result=%d errno=%d close=%d close_errno=%d version=0x%08x block=%llu blocks=%llu free=%llu available=%lld\n",
        result,queryError,closed,closeError,prefix.version,
        static_cast<unsigned long long>(prefix.blockSize),static_cast<unsigned long long>(prefix.blocks),
        static_cast<unsigned long long>(prefix.freeBlocks),static_cast<long long>(prefix.availableBlocks));
    if(closed) throw std::runtime_error("Closing storage query directory failed");
    if(result && (queryError==EPERM || queryError==EACCES || queryError==ENOSYS))
        throw CopySpaceUnavailable("Automatic storage check is unavailable; manual check required");
    if(result) throw std::runtime_error("HDD free-space query failed; copy was not started (see log)");
    return copyAvailable(prefix);
}
#endif
