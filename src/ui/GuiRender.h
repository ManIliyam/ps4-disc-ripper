#pragma once
// Scene2D is provided by the installed OpenOrbis SDK; no GPU/SDK APIs are invented.
// The host preview supplies a tracing Scene2D with this same drawing interface.
struct GuiPainter {
    Scene2D& scene;
    FT_Face small, body, subheading, heading, hero;
    const Color bg{10, 15, 24}, panel{17, 25, 38}, card{23, 34, 49};
    const Color white{237, 243, 247}, muted{141, 157, 177}, faint{81, 99, 122};
    const Color cyan{91, 222, 200}, border{37, 52, 70}, amber{240, 186, 111};
    void rect(int x, int y, int w, int h, Color c) {
        if (x < 0 || y < 0 || w <= 0 || h <= 0 || x+w > 1920 || y+h > 1080) return;
        scene.DrawRectangle(x,y,w,h,c);
    }
    void text(const std::string& value, FT_Face font, int x, int y, Color color, unsigned limit=100) {
        std::string safe;
        for (unsigned char c : value) {
            if (safe.size() >= limit) break;
            safe += c >= 32 && c <= 126 ? static_cast<char>(c) : '?';
        }
        if (value.size() > limit && safe.size() >= 3) safe.replace(safe.size()-3,3,"...");
        scene.DrawText(const_cast<char*>(safe.c_str()),font,x,y,bg,color);
    }
    void ring(int x,int y,int radius,int thickness,Color c) {
        for (int dy=-radius;dy<=radius;++dy) for (int dx=-radius;dx<=radius;++dx) {
            const int d=dx*dx+dy*dy;
            if (d<=radius*radius && d>=(radius-thickness)*(radius-thickness) && x+dx>=0 && x+dx<1920 && y+dy>=0 && y+dy<1080)
                scene.DrawPixel(x+dx,y+dy,c);
        }
    }
    void button(const std::string& label,int x,int y,int width,bool enabled=true) {
        rect(x,y,width,66,enabled ? cyan : card);
        text(label,body,x+24,y+43,enabled ? bg : muted,55);
    }
    void label(const std::string& title,const std::string& value,int x,int y,unsigned limit=52) {
        text(title,small,x,y,muted);
        text(value,body,x,y+38,white,limit);
    }
    void draw(const GuiView& v) {
        scene.FrameBufferFill(bg);
        rect(0,0,444,1080,panel);
        rect(443,0,1,1080,border);
        ring(100,116,28,3,cyan);ring(100,116,9,2,cyan);
        text("PS4",small,150,102,muted);
        text("DISC RIPPER",subheading,150,143,white);
        text("PERSONAL MEDIA ARCHIVE",small,70,209,faint);
        const char* items[]={"Detect disc","Browse disc","Extract disc","Selected files","Export to USB","FTP information","Settings"};
        for(int i=0;i<7;++i) {
            const int y=275+i*76;
            if(i==v.menu) { rect(46,y-42,352,62,card);rect(46,y-42,4,62,cyan); }
            char n[8]; std::snprintf(n,sizeof(n),"%02d",i+1);
            text(n,small,70,y,i==v.menu ? cyan : faint);
            text(items[i],body,119,y,i==v.menu ? white : muted,22);
        }
        rect(70,859,304,1,border);
        text("EARLY ACCESS  /  1.15",small,70,908,cyan);
        text("Created by iTZiLiYA",small,70,954,white);
        text("Original files. Preserved.",small,70,990,muted);
        text("YOUR MEDIA, YOUR MEMORIES",small,510,95,muted);
        rect(1610,64,228,42,card);
        text(v.data.busy ? "WORKING" : "READY",small,1640,93,v.data.busy ? amber : cyan);
        rect(510,980,1328,1,border);
        text("[UP/DOWN] Navigate     [X] Open / select     [O] Back",small,510,1023,muted,85);
        if(!v.controller) text("Reconnect your controller",small,1460,1023,amber,32);
        if(v.screen==GuiScreen::Overview) overview(v);
        else if(v.screen==GuiScreen::Browser) browser(v);
        else if(v.screen==GuiScreen::Test) test(v);
        else if(v.screen==GuiScreen::Ftp) ftp(v);
        else if(v.screen==GuiScreen::Settings) settings(v);
        else if(v.screen==GuiScreen::Details) details(v);
        else if(v.screen==GuiScreen::Copy) copy(v);
        else {
            text(v.unavailableTitle,heading,510,218,white,37);
            text("Not available in this build",subheading,510,301,amber);
            text(v.unavailableMessage,body,510,369,muted,75);
            text("Disc browsing and the small HDD test are available now.",body,510,425,muted,75);
            text("Press Circle to return.",body,510,510,white);
        }
        if(v.data.busy) {
            rect(510,861,1328,89,panel);
            rect(531+(v.frame/3)%32,891,8,8,cyan);
            text(v.data.status,body,591,899,white,65);
            text(v.data.activity,small,591,932,muted,95);
        }
    }
    void overview(const GuiView& v) {
        text("Start with",hero,510,247,white);
        text("your disc.",hero,510,335,white);
        text("Browse your recordings. Keep every original file.",body,515,409,muted,63);
        text("Choose Detect disc to inspect accessible files.",body,515,454,muted,63);
        const char* actions[]={"[X] Detect disc","[X] Browse disc","[X] Open HDD test","[X] Selected files","[X] USB export","[X] FTP information","[X] Settings"};
        button(v.data.busy ? "Working..." : actions[v.menu],515,503,385,!v.data.busy);
        ring(1590,344,145,4,border);ring(1590,344,119,2,card);
        ring(1590,344,44,4,cyan);ring(1590,344,15,2,muted);
        rect(1548,190,84,6,cyan);
        text("READ ONLY",small,1524,542,cyan);
        rect(510,639,644,175,panel);rect(1178,639,660,175,panel);
        label("DISC STATUS",v.data.scanned ? v.data.layout : "Not inspected",542,684,34);
        text(v.data.scanned ? std::to_string(v.data.entries.size())+" entries in the current folder" : "No disc type assumed before a scan.",small,542,779,muted,49);
        label("TRANSFER OPTIONS","Windows or internal HDD",1210,684,40);
        text("Selected-file HDD copies. FTP to Windows.",small,1210,779,muted,48);
        if(!v.data.busy) text(v.data.status,body,515,891,cyan,75);
    }
    void browser(const GuiView& v) {
        text(v.data.local ? "Saved archives" : "Browse disc",heading,510,202,white);
        text(v.data.path,small,515,251,cyan,95);
        text("NAME",small,540,316,muted);
        text("Select an entry to inspect it",small,1398,316,muted,33);
        rect(510,338,1328,1,border);
        const int count=static_cast<int>(v.data.entries.size());
        const int top=v.selectedFile<7 ? 0 : v.selectedFile-6;
        for(int i=top;i<count && i<top+7;++i) {
            const int y=383+(i-top)*64;
            const GuiEntry& e=v.data.entries[i];
            if(i==v.selectedFile) { rect(510,y-35,1328,56,card);rect(510,y-35,4,56,cyan); }
            text(e.typeKnown && e.directory ? ">" : "-",body,540,y,cyan);
            text(e.name,body,586,y,white,67);
            text(e.typeKnown && e.directory ? "Folder" : "Inspect",small,1700,y,muted,14);
        }
        if(!count) {
            text(v.data.busy ? "Reading directory..." : "No entries to display",subheading,540,425,white);
            text(v.data.activity,body,540,489,muted,78);
        }
        if(!v.data.busy) {
            text(v.data.status,small,515,826,white,92);
            text(v.data.activity,small,515,866,muted,94);
            text("[TRIANGLE] Refresh     [O] Parent folder / menu",small,515,932,muted,80);
        }
    }
    void test(const GuiView& v) {
        text("Save a small test",heading,510,202,white);
        text("Verify the path from disc to internal storage.",body,515,261,muted,75);
        rect(510,312,1328,119,panel);
        text("VIDEO_TS.IFO",subheading,542,361,white);
        text("8 KB test file only. No video files will be copied.",small,542,403,muted,75);
        const char* stages[]={"Connect to local FTP","Read the navigation file","Write to a unique HDD folder","Read back and compare every byte"};
        for(int i=0;i<4;++i) {
            const int y=490+i*57;
            text(v.data.testStep>static_cast<unsigned>(i) ? "OK" : "--",small,542,y,v.data.testStep>static_cast<unsigned>(i) ? cyan : faint);
            text(stages[i],body,610,y,white,75);
        }
        button(v.data.busy ? "Test in progress" : "[X] Run 8 KB HDD test",515,740,420,!v.data.busy);
        text("[TRIANGLE] Browse saved archives",small,1000,781,muted,50);
        if(!v.data.savedPath.empty() && !v.data.busy) {
            text("SAVED AND VERIFIED",small,515,864,cyan);
            text(v.data.savedPath,small,515,907,white,99);
        } else if(!v.data.busy) text(v.data.status,body,515,895,muted,85);
    }
    void ftp(const GuiView& v) {
        text("Keep it on Windows",heading,510,202,white);
        text("Connect with FileZilla or WinSCP on the same network.",body,515,267,muted,80);
        rect(510,328,1328,186,panel);
        label("PS4 ADDRESS",v.data.ip,546,377,28);
        label("FTP PORT","2121",1130,377,16);
        label("CONNECTION","FTP / unencrypted",1440,377,27);
        label("DISC FILES","/mnt/disc/",515,579,70);
        label("SAVED ARCHIVES","/data/MediaRipper/",515,702,70);
        text("Enable GoldHEN FTP before connecting. Use your trusted local network.",small,515,824,muted,97);
        text("Desktop transfer app and PS4-initiated transfers are planned.",small,515,877,amber,90);
        text("[TRIANGLE] Refresh address",small,515,932,white,60);
    }
    void settings(const GuiView&) {
        text("Built for your recordings",heading,510,202,white,42);
        label("AUTHOR","iTZiLiYA",515,305);
        label("APPLICATION","PS4 Disc Ripper / GUI preview 1.15",515,423);
        label("READER","GoldHEN local FTP on port 2121",515,541);
        text("Selected-file copying is experimental. Full-disc copying and USB are planned.",body,515,676,muted,85);
        text("Video playback previews are not supported in this build.",body,515,730,amber,85);
        text("OpenOrbis graphics, font and controller interfaces.",small,515,840,muted,90);
        text("[TRIANGLE] Browse saved archives",small,515,914,white,65);
    }
    void details(const GuiView& v) {
        text("File details",heading,510,202,white);
        rect(510,280,1328,148,panel);
        text(v.data.fileName,subheading,546,342,white,65);
        text(guiFileKind(v.data.fileName),small,546,395,cyan,70);
        label("FILE SIZE",v.data.fileSize,515,507,70);
        label("LOCATION",v.data.filePath.empty() ? v.data.path : v.data.filePath,515,629,92);
        text("Video preview unavailable",subheading,515,777,amber,65);
        text("Playback must be verified before preview can be enabled.",body,515,832,muted,85);
        text(v.data.local ? "Saved file / read only" : "[X] Copy this file to internal HDD (experimental)",small,515,918,cyan,90);
    }
    void copy(const GuiView& v) {
        text("Copy selected file",heading,510,202,white);
        text(v.data.fileName,body,515,270,white,75);
        text(v.data.status,subheading,515,352,v.data.copyVerified ? cyan : white,64);
        const uint64_t total=v.data.copyTotal,done=v.data.copyDone;
        const double fraction=total ? std::min(1.0,static_cast<double>(done)/total) : 0;
        rect(515,400,1300,22,card);
        if(fraction>0) rect(515,400,static_cast<int>(1300*fraction),22,cyan);
        char line[160];
        std::snprintf(line,sizeof(line),"%s: %.1f / %.1f MiB  (%.1f%%)",v.data.copyVerifying ? "HDD readback" : "Disc copy",
                      done/1048576.0,total/1048576.0,100*fraction);
        if(total && total<1048576)
            std::snprintf(line,sizeof(line),"%s: %llu / %llu bytes (%.1f%%)",v.data.copyVerifying ? "HDD readback" : "Disc copy",
                          static_cast<unsigned long long>(done),static_cast<unsigned long long>(total),100*fraction);
        text(line,body,515,478,white,85);
        const double speed=v.data.copySpeed;
        if(v.data.busy && speed>0) {
            const double eta=total>=done ? (total-done)/speed : 0;
            std::snprintf(line,sizeof(line),"%.2f MiB/s / phase remaining about %.0f seconds",speed/1048576.0,eta);
            text(line,small,515,529,muted,90);
        }
        if(v.data.freeSpaceKnown)
            std::snprintf(line,sizeof(line),"HDD available at start: %.2f GiB",v.data.freeBytes/1073741824.0);
        else std::snprintf(line,sizeof(line),v.data.copyManualSpace ? "HDD free space: Unknown / manually checked" : "HDD available at start: Unknown");
        text(line,small,515,592,muted,90);
        if(v.data.copyNeedsSpaceConfirmation && !v.data.busy) {
            text("REQUIRED FREE SPACE / FILE PLUS 512 MiB RESERVE",small,515,659,amber,85);
            const uint64_t requiredMiB=total/1048576+(total%1048576 ? 1 : 0)+512;
            std::snprintf(line,sizeof(line),"Check at least %llu MiB free in PS4 Settings > Storage",static_cast<unsigned long long>(requiredMiB));
            text(line,body,515,708,white,85);
            button("[X] I checked: enough free space",515,749,760);
            text("Automatic measurement is unavailable. Copying can fail if the HDD fills.",small,515,864,amber,96);
            text("[O] Back / no copy has started",small,515,919,muted,85);
            return;
        }
        text("DESTINATION",small,515,659,muted);
        text(v.data.copyPath,small,515,704,white,97);
        if(v.data.copyVerified) {
            text("SHA-256 / saved file matches received bytes",small,515,766,cyan,90);
            text(v.data.copyHash,small,515,809,white,80);
        } else text(v.data.activity,small,515,790,amber,96);
        if(!v.data.busy) text("[O] Return to menu / transfer the saved file through FTP",small,515,919,muted,95);
        else text("[O] Cancel / keep the disc inserted until copying finishes",small,515,833,amber,95);
    }
};
