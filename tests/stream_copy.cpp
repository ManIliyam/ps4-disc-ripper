#define COPY_HOST_TEST
#include "../src/extraction/StreamCopy.h"
#include "../src/storage/StorageSpace.h"
#include <functional>
#include <iostream>
#include <sys/stat.h>

int main() {
    char directory[]="/tmp/discripper-copy-XXXXXX";
    assert(mkdtemp(directory));
    const std::string root=directory,final=root+"/video.vob",partial=final+".partial";
    std::vector<unsigned char> bytes(1024*1024+19);
    for(size_t i=0;i<bytes.size();++i) bytes[i]=i%251;
    auto exists=[](const std::string& p){return access(p.c_str(),F_OK)==0;};
    auto run=[&](uint64_t expected,std::function<void()> complete,
                 std::function<bool()> cancel,std::function<void(uint64_t,uint64_t,bool)> progress) {
        size_t offset=0;
        return streamCopy(final,expected,[&](unsigned char* out,size_t capacity) {
            const size_t n=std::min(std::min(capacity,size_t(7171)),bytes.size()-offset);
            std::copy_n(bytes.data()+offset,n,out);offset+=n;return n;
        },complete,cancel,progress);
    };
    auto noCancel=[](){return false;};
    auto noProgress=[](uint64_t,uint64_t,bool){};
    auto noComplete=[](){};
    auto failure=[&](std::function<void()> f) {
        bool threw=false;try{f();}catch(const std::exception&){threw=true;}
        assert(threw);assert(!exists(partial));assert(!exists(final));
    };
    bool sawReadback=false;uint64_t copied=0,verified=0;
    const std::string hash=run(bytes.size(),noComplete,noCancel,[&](uint64_t n,uint64_t total,bool readback){
        assert(total==bytes.size());
        if(readback) {assert(n>=verified);verified=n;sawReadback=true;}
        else {assert(n>=copied);copied=n;}
    });
    assert(hash==picosha2::hash256_hex_string(bytes));
    // Independently calculated with Python hashlib for this multi-block fixture.
    assert(hash=="5aabd849f9cdf85025cdea82f1b8ccce2376284a22ae9c789e6f3acf8690af9c");
    assert(sawReadback && copied==bytes.size() && verified==bytes.size());
    assert(exists(final) && !exists(partial));
    // Never replace a preexisting destination.
    bool threw=false;try{run(bytes.size(),noComplete,noCancel,noProgress);}catch(...){threw=true;}
    assert(threw && exists(final) && !exists(partial));
    unlink(final.c_str());
    failure([&](){run(bytes.size()+1,noComplete,noCancel,noProgress);});
    failure([&](){run(bytes.size()-1,noComplete,noCancel,noProgress);});
    failure([&](){run(bytes.size(),[](){throw std::runtime_error("FTP completion failed");},noCancel,noProgress);});
    int polls=0;
    failure([&](){run(bytes.size(),noComplete,[&](){return ++polls>5;},noProgress);});
    bool cancelReadback=false;
    failure([&](){run(bytes.size(),noComplete,[&](){return cancelReadback;},[&](uint64_t,uint64_t,bool reading){cancelReadback=reading;});});
    failure([&](){run(bytes.size(),[&](){
        int fd=open(partial.c_str(),O_WRONLY);assert(fd>=0);
        const unsigned char changed=255;assert(pwrite(fd,&changed,1,0)==1);assert(close(fd)==0);
    },noCancel,noProgress);});
    failure([&](){streamCopy(final,1,[](unsigned char*,size_t)->size_t{throw std::runtime_error("Disc read error");},noComplete,noCancel,noProgress);});
    int fd=open(partial.c_str(),O_WRONLY|O_CREAT|O_EXCL,0600);assert(fd>=0);close(fd);
    threw=false;try{run(bytes.size(),noComplete,noCancel,noProgress);}catch(...){threw=true;}
    assert(threw && exists(partial));unlink(partial.c_str());
    assert(copyDecimal("4294967297")==4294967297ull);
    for(const std::string s:{"","-1","4 rubbish","18446744073709551616"}) {
        threw=false;try{copyDecimal(s);}catch(...){threw=true;}assert(threw);
    }
    assert(copyDiscPath("/mnt/disc/VIDEO_TS/VTS_01_1.VOB"));
    for(const std::string p:{"/mnt/disc/../data/a","/mnt/disc/a\r\nDELE x","/data/a","/mnt/disc/a/","/mnt/disc//a"}) assert(!copyDiscPath(p));
    CopyFsPrefix fs{0x20030518,0,0,4096,4096,100000,50000,40000};
    assert(copyAvailable(fs)==163840000);
    const CopyFsPrefix valid=fs;
    for(int scenario=0;scenario<6;++scenario) {
        fs=valid;
        if(scenario==0) fs.version=0;
        if(scenario==1) fs.blockSize=513;
        if(scenario==2) fs.availableBlocks=-1;
        if(scenario==3) fs.freeBlocks=fs.blocks+1;
        if(scenario==4) fs.availableBlocks=fs.freeBlocks+1;
        if(scenario==5) fs.blocks=UINT64_MAX;
        threw=false;try{copyAvailable(fs);}catch(...){threw=true;}assert(threw);
    }
    fs=valid;fs.availableBlocks=0;assert(copyAvailable(fs)==0);
    assert(copyRequiredSpace(8192)==536879104);
    assert(!copyManualMatches(0,8192));
    assert(!copyManualMatches(8192,1073709056));
    assert(copyManualMatches(8192,8192));
    for(uint64_t invalid:{uint64_t(0),UINT64_MAX}) {
        threw=false;try{copyRequiredSpace(invalid);}catch(...){threw=true;}assert(threw);
    }
    assert(picosha2::hash256_hex_string(std::string("abc"))=="ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    assert(rmdir(root.c_str())==0);
    std::cout<<"Streaming copy, cancellation, corruption, truncation, completion, collision, path, space and SHA-256 checks passed\n";
}
