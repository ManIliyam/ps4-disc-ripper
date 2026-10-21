#include <cassert>
#include <iostream>
#include "../src/ui/FtpInspect.h"
enum class Mode { Directory,EmptyDirectory,File,Missing,Disconnected,BadListing,BadSize };
Mode mode;
int sessions=0;
std::vector<std::string> commands;
struct FakeFtp {
    int id=++sessions;
    char last[1024]{};
    int command(const char* text) {
        commands.push_back(text);
        if(std::strncmp(text,"CWD ",4)==0) return 250; // reproduce permissive server
        if(std::strncmp(text,"SIZE ",5)==0) {
            assert(id==2); // fallback must use a fresh control session
            if(mode==Mode::Missing) {std::strcpy(last,"550 Missing");return 550;}
            std::strcpy(last,mode==Mode::BadSize ? "213 invalid" : "213 8192");return 213;
        }
        assert(std::strcmp(text,"QUIT")==0);return 221;
    }
    bool transfer(const char*,const char* request,bool,unsigned char* data,unsigned* count) {
        commands.push_back(request);
        assert(std::strncmp(request,"NLST ",5)==0);
        if(mode==Mode::Directory || mode==Mode::EmptyDirectory || mode==Mode::BadListing) {
            const std::string listing=mode==Mode::EmptyDirectory ? "" : mode==Mode::BadListing ? "../escape\r\n" : "VIDEO_TS.IFO\r\n";
            std::memcpy(data,listing.data(),listing.size());*count=listing.size();
            std::strcpy(last,"226 Complete");return true;
        }
        std::strcpy(last,mode==Mode::Disconnected ? "150 Transfer interrupted" : "550 File unavailable");
        return false;
    }
};
int main() {
    for(Mode scenario:{Mode::Directory,Mode::EmptyDirectory,Mode::File,Mode::Missing,Mode::Disconnected,Mode::BadListing,Mode::BadSize}) {
        mode=scenario;sessions=0;commands.clear();
        std::vector<GuiEntry> entries;bool directory=false;std::string size="Unknown";
        const bool ok=guiInspectRemote<FakeFtp>("/mnt/disc/VIDEO_TS/VIDEO_TS.IFO",entries,directory,size,[](FakeFtp&){return true;});
        assert(ok==(mode==Mode::Directory || mode==Mode::EmptyDirectory || mode==Mode::File));
        if(mode==Mode::File) assert(!directory && size=="8192 bytes" && entries.empty() && sessions==2);
        if(mode==Mode::Directory) assert(directory && entries.size()==1 && sessions==1);
        if(mode==Mode::EmptyDirectory) assert(directory && entries.empty() && sessions==1);
        if(mode==Mode::Disconnected || mode==Mode::BadListing) assert(sessions==1);
        for(const auto& command:commands) assert(command.compare(0,4,"CWD ")!=0);
    }
    std::string size;
    assert(guiSizeReply("213 4294967297",size) && size=="4294967297 bytes");
    for(const char* reply:{"213 ","213 -1","213 18446744073709551616","213 12 junk","550 Missing"})
        assert(!guiSizeReply(reply,size));
    std::cout<<"FTP file/directory regression checks passed (including permissive CWD server)\n";
}
