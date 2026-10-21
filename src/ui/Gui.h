#pragma once
#include "GuiRender.h"

std::mutex guiMutex;
GuiSnapshot guiData;
std::thread guiWorker;
bool guiTrackingTest=false;

GuiSnapshot guiSnapshot() {
    std::lock_guard<std::mutex> lock(guiMutex);
    return guiData;
}
void guiRecordLog(const char* line) {
    std::lock_guard<std::mutex> lock(guiMutex);
    std::string text(line);
    while (!text.empty() && (text.back()=='\n' || text.back()=='\r')) text.pop_back();
    if (guiTrackingTest) {
        if (text.find("FTP connect result=0x00000000") == 0) guiData.testStep=1;
        if (text.find("FTP IFO read result=success") == 0) guiData.testStep=2;
        if (text.find("ARCHIVE WRITE_COMPLETE") == 0) guiData.testStep=3;
        if (text.find("ARCHIVE VERIFIED") == 0) { guiData.testStep=4; guiData.testVerified=true; }
    }
    const std::string prefix="ARCHIVE SAVED path=";
    if (text.compare(0,prefix.size(),prefix)==0) guiData.savedPath=text.substr(prefix.size());
    if (text.find("FTP command ")==0) guiData.activity=text.substr(12);
    if (text.find("FTP reply 4")==0 || text.find("FTP reply 5")==0 || text.find("ERROR ")==0)
        guiData.activity=text.substr(0,140);
}
void guiMessage(const std::string& status,const std::string& activity) {
    std::lock_guard<std::mutex> lock(guiMutex);
    guiData.status=status; guiData.activity=activity;
}
void guiRefreshIp() {
    sceNetInit();
    const int init=sceNetCtlInit();
    OrbisNetCtlInfo info{};
    const int result=sceNetCtlGetInfo(ORBIS_NET_CTL_INFO_IP_ADDRESS,&info);
    logf("GUI NetCtl init=0x%08x get_ip=0x%08x\n",static_cast<unsigned>(init),static_cast<unsigned>(result));
    std::lock_guard<std::mutex> lock(guiMutex);
    info.ip_address[15]=0;
    guiData.ip=result==0 && info.ip_address[0] ? info.ip_address : "Unavailable";
}
bool guiLogin(ProbeFtp& ftp) {
    sceNetInit();
    if (!ftp.control.connectTo("127.0.0.1",2121) || ftp.reply()!=220) return false;
    int code=ftp.command("USER anonymous");
    if(code==331) code=ftp.command("PASS MediaRipperGUI");
    return code==230 && ftp.command("TYPE I")==200;
}
#include "GuiCopy.h"
bool guiLocalListing(const std::string& path,std::vector<GuiEntry>& entries) {
    DIR* dir=opendir(path.c_str());
    if(!dir) { error("GUI opendir",path.c_str(),errno); return false; }
    bool ok=true;
    for (;;) {
        errno=0;
        dirent* entry=readdir(dir);
        if(!entry) { if(errno) {error("GUI readdir",path.c_str(),errno);ok=false;} break; }
        std::string leaf=entry->d_name;
        if(leaf=="." || leaf=="..") continue;
        if(!guiSafeLeaf(leaf) || entries.size()>=256) { ok=false;break; }
        const std::string full=path+"/"+leaf;
        const int test=open(full.c_str(),O_RDONLY|O_DIRECTORY|O_NOFOLLOW);
        const bool directoryEntry=test>=0;
        if(test>=0) close(test);
        entries.push_back({leaf,directoryEntry,true});
    }
    if(closedir(dir)) ok=false;
    std::sort(entries.begin(),entries.end(),[](const GuiEntry& a,const GuiEntry& b){
        if(a.directory!=b.directory) return a.directory>b.directory;
        return a.name<b.name;
    });
    return ok;
}
void guiBrowse(const std::string& path,bool local) {
    std::vector<GuiEntry> entries;
    bool isDirectory=false,ok=false;
    std::string size="Unknown";
    if(local) {
        const int check=open(path.c_str(),O_RDONLY|O_DIRECTORY|O_NOFOLLOW);
        if(check>=0) {close(check);isDirectory=true;ok=guiLocalListing(path,entries);}
        else {
            const int fd=open(path.c_str(),O_RDONLY|O_NOFOLLOW|O_NONBLOCK);
            if(fd>=0) {
                const off_t end=lseek(fd,0,SEEK_END);
                if(end>=0) size=std::to_string(static_cast<unsigned long long>(end))+" bytes";
                close(fd);ok=true;
            }
        }
    } else {
        ok=guiInspectRemote<ProbeFtp>(path,entries,isDirectory,size,guiLogin);
        logf("GUI inspect path=%s result=%s kind=%s size=%s\n",path.c_str(),ok ? "success" : "failed",
             ok ? (isDirectory ? "directory" : "file") : "unknown",size.c_str());
    }
    if(!ok) {
        guiMessage("Unable to inspect this location","Check the disc and enable GoldHEN FTP. See the GUI log for server replies.");
        return;
    }
    std::string layout="Files accessible";
    if(!local) for(const GuiEntry& e:entries) {
        if(e.name=="VIDEO_TS" || path.find("/VIDEO_TS")!=std::string::npos) layout="DVD-Video layout";
        if(e.name=="DVD_RTAV" || path.find("/DVD_RTAV")!=std::string::npos) layout="DVD recording layout";
        if(e.name=="BDMV" || e.name=="BDAV" || path.find("/BDMV")!=std::string::npos) layout="Blu-ray video layout";
    }
    std::lock_guard<std::mutex> lock(guiMutex);
    guiData.local=local;
    if(!local) {guiData.scanned=true;guiData.layout=layout;}
    if(isDirectory) {
        guiData.entries=std::move(entries);guiData.path=path;guiData.directoryReady=true;
        guiData.status=local ? "Saved files loaded" : "Disc directory loaded";
        guiData.activity=std::to_string(guiData.entries.size())+" entries";
    } else {
        const size_t slash=path.find_last_of('/');
        guiData.fileName=path.substr(slash+1);guiData.fileSize=size;
        guiData.filePath=path;
        guiData.status="File details loaded";guiData.detailsReady=true;
        guiData.activity="Video playback is not enabled in this build.";
    }
}
enum class GuiJob { Browse, SmallTest, RefreshIp, CopyFile };
void guiStart(GuiJob job,const std::string& path="/mnt/disc",bool local=false,uint64_t confirmedBytes=0) {
    if(guiSnapshot().busy) return;
    if(guiWorker.joinable()) guiWorker.join();
    {
        std::lock_guard<std::mutex> lock(guiMutex);
        guiData.busy=true;guiData.detailsReady=false;guiData.directoryReady=false;
        guiData.copyActive=job==GuiJob::CopyFile;
        if(guiData.copyActive) {
            guiCopyCancel=false;guiData.copyDone=0;guiData.copyTotal=0;guiData.copySpeed=0;
            guiData.copyVerifying=false;guiData.copyVerified=false;guiData.copyHash.clear();
            guiData.copyPath.clear();guiData.freeBytes=0;guiData.freeSpaceKnown=false;guiData.savedPath.clear();
            guiData.copyNeedsSpaceConfirmation=false;guiData.copyManualSpace=false;
        }
        guiTrackingTest=job==GuiJob::SmallTest;
        guiData.status=job==GuiJob::SmallTest ? "Running the 8 KB HDD test" : "Reading...";
        guiData.activity="The interface remains available while the reader works.";
        if(job==GuiJob::SmallTest) {guiData.testStep=0;guiData.testVerified=false;guiData.savedPath.clear();}
    }
    try {
        guiWorker=std::thread([job,path,local,confirmedBytes]() {
            try {
                if(job==GuiJob::RefreshIp) {
                    guiRefreshIp();guiMessage("Network information refreshed","Use the address shown on the FTP information screen.");
                } else if(job==GuiJob::Browse) {
                    guiRefreshIp();guiBrowse(path,local);
                } else if(job==GuiJob::CopyFile) {
                    guiMessage("Checking file and HDD space","Copy will begin only after size and storage checks pass.");
                    guiCopyFile(path,confirmedBytes);
                } else {
                    if(directory("/data/MediaRipper") && directory("/data/MediaRipper/logs")) ftpProbe();
                    const GuiSnapshot result=guiSnapshot();
                    const bool verified=result.testVerified && !result.savedPath.empty();
                    guiMessage(verified ? "Small-file test passed" : "Small-file test did not complete",
                               verified ? "Saved and compared every byte. This is not a full DVD backup." : "Read the GUI log for details; leave FTP enabled and use the same test DVD.");
                }
                if(logFd>=0) fsync(logFd);
            } catch(const std::exception& exception) {
                logf("ERROR worker %s\n",exception.what());
                guiMessage(job==GuiJob::CopyFile && guiCopyCancel ? "Copy cancelled" : "Operation failed",exception.what());
            } catch(...) {guiMessage("Operation failed","An unexpected worker error occurred.");}
            std::lock_guard<std::mutex> lock(guiMutex);guiData.busy=false;guiData.copyActive=false;
        });
    } catch(...) {
        std::lock_guard<std::mutex> lock(guiMutex);
        guiData.busy=false;guiData.status="Unable to start worker";
        guiData.activity="Close and reopen the application.";
    }
}

[[noreturn]] void runGui() {
    char output[256];
    for(unsigned i=0;i<1000;++i) {
        std::snprintf(output,sizeof(output),"/data/MediaRipper-gui-%lld-%03u.log",static_cast<long long>(std::time(nullptr)),i);
        logFd=open(output,O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW,0644);
        if(logFd>=0 || errno!=EEXIST) break;
    }
    guiData.logPath=logFd>=0 ? output : "Log unavailable";
    logf("PS4 Disc Ripper GUI 1.15 | Created by iTZiLiYA\n");
    // Avoid destructing a live Scene2D on a terminal failure; use normal PS menu close.
    Scene2D* scene=new Scene2D(1920,1080,4);
    if(!scene->Init(0x4000000,2)) {logf("GUI video initialization failed\n");park();}
    scene->SetActiveFrameBuffer(0);
    FT_Face small{},body{},subheading{},heading{},hero{};
    const char* font="/app0/assets/fonts/Gontserrat-Regular.ttf";
    if(!scene->InitFont(&small,font,22) || !scene->InitFont(&body,font,28) ||
       !scene->InitFont(&subheading,font,36) || !scene->InitFont(&heading,font,54) ||
       !scene->InitFont(&hero,font,84)) {
        logf("GUI font initialization failed\n");scene->FrameBufferFill({30,10,14});scene->SubmitFlip(0);park();
    }
    GuiPainter painter{*scene,small,body,subheading,heading,hero};
    scePadInit();
    OrbisUserServiceInitializeParams params{};params.priority=ORBIS_KERNEL_PRIO_FIFO_LOWEST;
    const int userInit=sceUserServiceInitialize(&params);
    int user=-1,pad=-1;
    if(sceUserServiceGetInitialUser(&user)==0) pad=scePadOpen(user,ORBIS_PAD_PORT_TYPE_STANDARD,0,nullptr);
    logf("GUI controller user_init=0x%08x pad=%d\n",static_cast<unsigned>(userInit),pad);
    GuiView view;
    uint32_t previous=0;
    unsigned frame=0;
    int heldDirection=0,heldFrames=0;
    for(;;) {
        view.frame=frame;view.data=guiSnapshot();
        if(!view.data.busy && guiWorker.joinable()) guiWorker.join();
        if(view.data.detailsReady || view.data.directoryReady) {
            view.screen=view.data.detailsReady ? GuiScreen::Details : GuiScreen::Browser;
            if(view.data.directoryReady) view.selectedFile=0;
            std::lock_guard<std::mutex> lock(guiMutex);guiData.detailsReady=false;guiData.directoryReady=false;
        }
        OrbisPadData state{};
        const int read=pad>=0 ? scePadReadState(pad,&state) : -1;
        view.controller=read>=0 && state.connected;
        const uint32_t buttons=view.controller ? state.buttons : 0;
        const uint32_t pressed=buttons & ~previous;previous=buttons;
        if(!view.controller && frame%180==0) {
            if(sceUserServiceGetInitialUser(&user)==0) {
                const int handle=scePadGetHandle(user,0,0);
                if(handle>=0) pad=handle;
                else if(pad<0) pad=scePadOpen(user,0,0,nullptr);
            }
        }
        const int direction=(buttons&ORBIS_PAD_BUTTON_DOWN) ? 1 : (buttons&ORBIS_PAD_BUTTON_UP) ? -1 : 0;
        if(direction==heldDirection) ++heldFrames; else {heldDirection=direction;heldFrames=0;}
        const bool move=direction && (heldFrames==0 || (heldFrames>=24 && heldFrames%7==0));
        if(move) {
            if(view.screen==GuiScreen::Browser) {
                const int count=static_cast<int>(view.data.entries.size());
                if(count) view.selectedFile=(view.selectedFile+direction+count)%count;
            } else if(view.screen==GuiScreen::Overview) view.menu=(view.menu+direction+7)%7;
        }
        if(pressed&ORBIS_PAD_BUTTON_CIRCLE) {
            if(view.data.copyActive) {guiCopyCancel=true;guiMessage("Cancelling copy","Waiting for the current operation; incomplete output will be removed.");}
            else if(view.screen==GuiScreen::Details) view.screen=GuiScreen::Browser;
            else if(view.screen==GuiScreen::Browser && !view.data.busy) {
                const std::string parent=guiParent(view.data.path,view.data.local);
                if(parent!=view.data.path) guiStart(GuiJob::Browse,parent,view.data.local);
                else view.screen=GuiScreen::Overview;
            } else view.screen=GuiScreen::Overview;
        }
        if((pressed&ORBIS_PAD_BUTTON_TRIANGLE) && !view.data.busy) {
            if(view.screen==GuiScreen::Browser) guiStart(GuiJob::Browse,view.data.path,view.data.local);
            else if(view.screen==GuiScreen::Ftp) guiStart(GuiJob::RefreshIp);
            else if(view.screen==GuiScreen::Test || view.screen==GuiScreen::Settings) guiStart(GuiJob::Browse,"/data/MediaRipper",true);
        }
        if((pressed&ORBIS_PAD_BUTTON_CROSS) && !view.data.busy) {
            if(view.screen==GuiScreen::Overview) {
                if(view.menu<2) {view.screen=GuiScreen::Browser;guiStart(GuiJob::Browse);}
                else if(view.menu==2) view.screen=GuiScreen::Test;
                else if(view.menu==5) {view.screen=GuiScreen::Ftp;guiStart(GuiJob::RefreshIp);}
                else if(view.menu==6) view.screen=GuiScreen::Settings;
                else if(view.menu==3) {view.screen=GuiScreen::Browser;guiStart(GuiJob::Browse);}
                else {
                    view.screen=GuiScreen::Unavailable;
                    view.unavailableTitle=view.menu==3 ? "Selected-file extraction" : "Export to USB";
                    view.unavailableMessage=view.menu==3 ? "Large-file extraction is still in development." : "USB export will be added after the copy engine is validated.";
                }
            } else if(view.screen==GuiScreen::Test) guiStart(GuiJob::SmallTest);
            else if(view.screen==GuiScreen::Copy && view.data.copyNeedsSpaceConfirmation) {
                guiStart(GuiJob::CopyFile,view.data.filePath,false,view.data.copyTotal);
            }
            else if(view.screen==GuiScreen::Details && !view.data.local && copyDiscPath(view.data.filePath)) {
                view.screen=GuiScreen::Copy;guiStart(GuiJob::CopyFile,view.data.filePath);
            }
            else if(view.screen==GuiScreen::Browser && view.selectedFile>=0 && static_cast<size_t>(view.selectedFile)<view.data.entries.size()) {
                const std::string leaf=view.data.entries[view.selectedFile].name;
                if(guiSafeLeaf(leaf)) guiStart(GuiJob::Browse,view.data.path+"/"+leaf,view.data.local);
            }
        }
        view.data=guiSnapshot();
        painter.draw(view);
        scene->SubmitFlip(static_cast<int>(frame));scene->FrameWait(static_cast<int>(frame));scene->FrameBufferSwap();
        ++frame;
    }
}
