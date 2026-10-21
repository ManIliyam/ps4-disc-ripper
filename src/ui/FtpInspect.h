#pragma once
#include "GuiModel.h"
#include <cstring>

inline bool guiSizeReply(const char* reply,std::string& size) {
    if(std::strncmp(reply,"213 ",4)!=0) return false;
    const std::string digits=reply+4;
    uint64_t value=0;
    if(digits.empty()) return false;
    for(unsigned char c:digits) {
        if(c<'0' || c>'9' || value>(UINT64_MAX-(c-'0'))/10) return false;
        value=value*10+c-'0';
    }
    size=std::to_string(value)+" bytes";
    return true;
}

// GoldHEN v2.2 can accept CWD on a regular file. Classify only from an
// actual directory listing or a successful SIZE response, never from CWD.
template<class Session,class Login>
bool guiInspectRemote(const std::string& path,std::vector<GuiEntry>& entries,
                      bool& isDirectory,std::string& size,Login login) {
    Session directory;
    if(!login(directory)) return false;
    std::vector<unsigned char> bytes(65536);unsigned count=0;
    if(directory.transfer("127.0.0.1",("NLST "+path+"/").c_str(),true,bytes.data(),&count)) {
        if(!guiParseListing(bytes.data(),count,entries)) return false;
        isDirectory=true;directory.command("QUIT");return true;
    }
    // Only a definite rejection permits the file probe. Do not reinterpret a
    // dropped connection, failed passive setup or incomplete transfer as a file.
    if(std::strncmp(directory.last,"550 ",4)!=0) return false;
    // A fresh control connection avoids stale replies/cwd state after failure.
    Session file;
    if(!login(file) || file.command(("SIZE "+path).c_str())!=213 ||
       !guiSizeReply(file.last,size)) return false;
    isDirectory=false;entries.clear();file.command("QUIT");return true;
}
