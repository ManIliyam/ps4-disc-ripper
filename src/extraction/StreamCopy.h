#pragma once
#include "picosha2.h"
#include <cerrno>
#include <cstdint>
#include <fcntl.h>
#include <unistd.h>
#include <string>
#include <vector>
#include <stdexcept>

inline uint64_t copyDecimal(const std::string& text) {
    if(text.empty()) throw std::runtime_error("Missing file size");
    uint64_t n=0;
    for(unsigned char c:text) {
        if(c<'0' || c>'9' || n>(UINT64_MAX-(c-'0'))/10)
            throw std::runtime_error("Invalid or overflowing FTP file size");
        n=n*10+c-'0';
    }
    return n;
}
inline bool copyDiscPath(const std::string& path) {
    if(path.compare(0,10,"/mnt/disc/")!=0 || path.size()>220) return false;
    size_t begin=10;
    while(begin<path.size()) {
        const size_t end=path.find('/',begin);
        const std::string leaf=path.substr(begin,end==std::string::npos ? end : end-begin);
        if(leaf.empty() || leaf=="." || leaf=="..") return false;
        for(unsigned char c:leaf) if(c<32 || c>126 || c=='\\') return false;
        if(end==std::string::npos) return true;
        begin=end+1;
    }
    return false;
}
struct CopyFile {
    int fd=-1;
    explicit CopyFile(int value):fd(value){}
    ~CopyFile(){if(fd>=0) close(fd);}
    void checkedClose() {
        const int value=fd;fd=-1;
        if(close(value)) throw std::runtime_error("Closing output file failed");
    }
};

// receive returns a byte count, EOF (0), or throws. complete must validate the
// FTP completion response. Progress phase distinguishes copying from readback.
// The caller owns a newly created archive directory; final must not preexist.
template<class Receive,class Complete,class Cancel,class Progress>
std::string streamCopy(const std::string& final,uint64_t expected,Receive receive,
                       Complete complete,Cancel cancelled,Progress progress) {
    const std::string partial=final+".partial";
    CopyFile output(open(partial.c_str(),O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW,0644));
    if(output.fd<0) throw std::runtime_error("Cannot create exclusive partial file");
    bool published=false;
    bool reserved=false;
    try {
        std::vector<unsigned char> buffer(64*1024);
        picosha2::hash256_one_by_one source;
        uint64_t count=0;
        for(;;) {
            if(cancelled()) throw std::runtime_error("Copy cancelled");
            const size_t n=receive(buffer.data(),buffer.size());
            if(!n) break;
            if(n>buffer.size() || count>expected || n>expected-count)
                throw std::runtime_error("Disc sent more bytes than its reported size");
            size_t offset=0;unsigned retries=0;
            while(offset<n) {
                if(cancelled()) throw std::runtime_error("Copy cancelled");
                const ssize_t wrote=write(output.fd,buffer.data()+offset,n-offset);
                if(wrote<0 && errno==EINTR && retries++<3) continue;
                if(wrote<=0) throw std::runtime_error("HDD write failed (possibly full)");
                offset+=static_cast<size_t>(wrote);retries=0;
            }
            source.process(buffer.begin(),buffer.begin()+n);
            count+=n;progress(count,expected,false);
        }
        if(count!=expected) throw std::runtime_error("Disc read ended before the expected file size");
        complete();
        if(cancelled()) throw std::runtime_error("Copy cancelled");
        if(fsync(output.fd)) throw std::runtime_error("Flushing HDD output failed");
        output.checkedClose();source.finish();
        const std::string digest=picosha2::get_hash_hex_string(source);
        CopyFile input(open(partial.c_str(),O_RDONLY|O_NOFOLLOW));
        if(input.fd<0) throw std::runtime_error("Cannot reopen saved file for verification");
        picosha2::hash256_one_by_one saved;
        count=0;unsigned retries=0;
        for(;;) {
            if(cancelled()) throw std::runtime_error("Copy cancelled");
            const ssize_t n=read(input.fd,buffer.data(),buffer.size());
            if(n<0 && errno==EINTR && retries++<3) continue;
            if(n<0) throw std::runtime_error("HDD readback failed");
            if(!n) break;
            retries=0;
            if(count>expected || static_cast<uint64_t>(n)>expected-count)
                throw std::runtime_error("HDD readback length mismatch");
            saved.process(buffer.begin(),buffer.begin()+n);count+=n;
            progress(count,expected,true);
        }
        input.checkedClose();saved.finish();
        if(count!=expected || picosha2::get_hash_hex_string(saved)!=digest)
            throw std::runtime_error("Saved file failed SHA-256 verification");
        if(cancelled()) throw std::runtime_error("Copy cancelled");
        // The caller created an exclusive archive directory. Reserve the final
        // name exclusively, then replace only that empty file owned by this job.
        CopyFile reservation(open(final.c_str(),O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW,0644));
        if(reservation.fd<0) throw std::runtime_error("Final file already exists or cannot be created");
        reserved=true;reservation.checkedClose();
        if(rename(partial.c_str(),final.c_str())) throw std::runtime_error("Publishing verified file failed");
        published=true;
        return digest;
    } catch(...) {
        if(output.fd>=0) {close(output.fd);output.fd=-1;}
        if(!published) unlink(partial.c_str());
        if(reserved && !published) unlink(final.c_str());
        throw;
    }
}
