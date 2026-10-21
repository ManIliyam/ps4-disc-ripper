#pragma once
std::atomic<bool> guiCopyCancel{false};
// Avoid musl/FreeBSD clock-id translation; this native SDK API reports elapsed
// process time in microseconds and does not depend on the console's wall clock.
struct CopyClock {
    using duration=std::chrono::microseconds;
    using rep=duration::rep;
    using period=duration::period;
    using time_point=std::chrono::time_point<CopyClock>;
    static constexpr bool is_steady=true;
    static time_point now() {return time_point(duration(sceKernelGetProcessTime()));}
};

void guiCopyFile(const std::string& sourcePath,uint64_t confirmedBytes=0) {
    if(!copyDiscPath(sourcePath)) throw std::runtime_error("Select a file inside /mnt/disc first");
    if(!directory("/data/MediaRipper")) throw std::runtime_error("Cannot access archive directory");
    ProbeFtp ftp;
    if(!guiLogin(ftp)) throw std::runtime_error("Cannot connect to GoldHEN FTP on port 2121");
    if(ftp.command(("SIZE "+sourcePath).c_str())!=213)
        throw std::runtime_error("Server did not report the selected file size");
    const uint64_t expected=copyDecimal(std::string(ftp.last+4));
    if(!expected) throw std::runtime_error("Selected file is empty");
    {std::lock_guard<std::mutex> lock(guiMutex);guiData.copyTotal=expected;}
    const uint64_t required=copyRequiredSpace(expected);
    uint64_t available=0;
    try {
        available=copyStorageSpace(logf);
        {std::lock_guard<std::mutex> lock(guiMutex);guiData.freeBytes=available;guiData.freeSpaceKnown=true;}
        if(available<required) throw std::runtime_error("Not enough HDD space (512 MiB reserve required)");
        logf("COPY preflight measured bytes=%llu available=%llu required=%llu\n",
             static_cast<unsigned long long>(expected),static_cast<unsigned long long>(available),static_cast<unsigned long long>(required));
    } catch(const CopySpaceUnavailable&) {
        if(!copyManualMatches(confirmedBytes,expected)) {
            logf("COPY waiting for manual space check source=%s bytes=%llu required=%llu; no copy started\n",sourcePath.c_str(),
                 static_cast<unsigned long long>(expected),static_cast<unsigned long long>(required));
            {std::lock_guard<std::mutex> lock(guiMutex);guiData.copyNeedsSpaceConfirmation=true;}
            guiMessage("Check PS4 storage first","Automatic check denied. Confirm enough free space in PS4 Settings > Storage.");
            return;
        }
        logf("COPY preflight MANUAL user-confirmed source=%s bytes=%llu required=%llu; available space UNKNOWN\n",sourcePath.c_str(),
             static_cast<unsigned long long>(expected),static_cast<unsigned long long>(required));
        {std::lock_guard<std::mutex> lock(guiMutex);guiData.copyManualSpace=true;}
        guiMessage("Copying with manual space check","Free space was confirmed manually; automatic measurement remains unavailable.");
    }
    std::string folder;
    for(unsigned i=0;i<1000;++i) {
        folder="/data/MediaRipper/FILE_"+std::to_string(static_cast<long long>(std::time(nullptr)))+"_"+std::to_string(i);
        if(!mkdir(folder.c_str(),0755)) break;
        if(errno!=EEXIST || i==999) throw std::runtime_error("Cannot create unique archive directory");
    }
    // Preserve every directory component below /mnt/disc.
    const std::string relative=sourcePath.substr(10);
    size_t slash=0;
    while((slash=relative.find('/',slash))!=std::string::npos) {
        if(mkdir((folder+"/"+relative.substr(0,slash)).c_str(),0755))
            throw std::runtime_error("Cannot create archive subdirectory");
        ++slash;
    }
    const std::string destination=folder+"/"+relative;
    {
        std::lock_guard<std::mutex> lock(guiMutex);
        guiData.copyTotal=expected;guiData.freeBytes=available;guiData.copyPath=destination;
    }
    if(guiCopyCancel) throw std::runtime_error("Copy cancelled");
    if(ftp.command("PASV")!=227) throw std::runtime_error("FTP passive mode failed");
    const char* start=std::strchr(ftp.last,'(');unsigned a,b,c,d,p,q;
    if(!start || std::sscanf(start,"(%u,%u,%u,%u,%u,%u)",&a,&b,&c,&d,&p,&q)!=6 ||
       a>255 || b>255 || c>255 || d>255 || p>255 || q>255 || !(p*256+q))
        throw std::runtime_error("Invalid FTP passive response");
    ProbeSocket data;
    if(!data.connectTo("127.0.0.1",p*256+q)) throw std::runtime_error("FTP data connection failed");
    const int preliminary=ftp.command(("RETR "+sourcePath).c_str());
    if(preliminary!=125 && preliminary!=150) throw std::runtime_error("Disc file could not be opened");
    auto lastRead=CopyClock::now(),phaseStart=lastRead,lastUpdate=lastRead,lastLog=lastRead;
    bool verifying=false;
    auto receive=[&](unsigned char* bytes,size_t capacity)->size_t {
        for(;;) {
            if(guiCopyCancel) throw std::runtime_error("Copy cancelled");
            // Scene-Collective native MSG_DONTWAIT, not libc's incompatible 0x40.
            const int n=sceNetRecv(data.id,bytes,capacity,0x80);
            if(n>=0) {lastRead=CopyClock::now();return static_cast<size_t>(n);}
            // Retry transient receive failures with a bounded no-progress timer;
            // never skip or substitute unread bytes, never restart silently.
            if(CopyClock::now()-lastRead>std::chrono::seconds(30)) {
                logf("COPY receive stalled raw=0x%08x\n",static_cast<unsigned>(n));
                throw std::runtime_error("Disc/FTP read stalled for 30 seconds");
            }
            sceKernelUsleep(10000);
        }
    };
    auto complete=[&]() {
        const int code=ftp.reply();
        if(code!=226 && code!=250) throw std::runtime_error("FTP did not confirm the complete transfer");
    };
    auto progress=[&](uint64_t done,uint64_t total,bool readback) {
        const auto now=CopyClock::now();
        if(readback!=verifying) {verifying=readback;phaseStart=now;lastUpdate=now-std::chrono::seconds(1);}
        if(done!=total && now-lastUpdate<std::chrono::milliseconds(200)) return;
        const double seconds=std::chrono::duration<double>(now-phaseStart).count();
        {std::lock_guard<std::mutex> lock(guiMutex);
         guiData.copyDone=done;guiData.copyTotal=total;guiData.copyVerifying=readback;
         guiData.copySpeed=seconds>0 ? done/seconds : 0;
         guiData.status=readback ? "Verifying saved video" : "Copying selected file";}
        if(now-lastLog>=std::chrono::seconds(5) || done==total) {
            logf("COPY progress phase=%s bytes=%llu total=%llu\n",readback ? "readback" : "receive",
                 static_cast<unsigned long long>(done),static_cast<unsigned long long>(total));
            lastLog=now;
        }
        lastUpdate=now;
    };
    const std::string hash=streamCopy(destination,expected,receive,complete,
                                    [](){return guiCopyCancel.load();},progress);
    logf("COPY VERIFIED path=%s bytes=%llu sha256=%s\n",destination.c_str(),
         static_cast<unsigned long long>(expected),hash.c_str());
    {std::lock_guard<std::mutex> lock(guiMutex);
     guiData.copyVerified=true;guiData.savedPath=destination;guiData.copyHash=hash;}
    guiMessage("File saved and verified","SHA-256 matches the received disc bytes. Test playback on Windows next.");
}
