#include <cstdio>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <cassert>
#include "../src/ui/GuiModel.h"
struct Color {unsigned char r,g,b;};
using FT_Face=int;
std::ofstream trace;
bool outside=false;
std::string quote(const char* value) {
    std::string result="\"";
    for(const unsigned char* p=reinterpret_cast<const unsigned char*>(value);*p;++p) {
        if(*p=='"' || *p=='\\') result+='\\';
        if(*p>=32 && *p<127) result+=static_cast<char>(*p);
    }
    return result+'"';
}
struct Scene2D {
    void color(Color c) {trace<<"["<<int(c.r)<<","<<int(c.g)<<","<<int(c.b)<<"]";}
    void FrameBufferFill(Color c) {trace<<"[\"fill\",";color(c);trace<<"]\n";}
    void DrawRectangle(int x,int y,int w,int h,Color c) {
        outside |= x<0 || y<0 || x+w>1920 || y+h>1080;
        trace<<"[\"rect\","<<x<<","<<y<<","<<w<<","<<h<<",";color(c);trace<<"]\n";
    }
    void DrawPixel(int x,int y,Color c) {
        outside |= x<0 || y<0 || x>=1920 || y>=1080;
        trace<<"[\"pixel\","<<x<<","<<y<<",";color(c);trace<<"]\n";
    }
    void DrawText(char* value,FT_Face size,int x,int y,Color,Color c) {
        trace<<"[\"text\","<<quote(value)<<","<<size<<","<<x<<","<<y<<",";color(c);trace<<"]\n";
    }
};
#include "../src/ui/GuiRender.h"
void modelTests() {
    assert(guiSafeLeaf("VIDEO_TS.IFO"));
    for(const char* s:{"",".","..","../a","a/b","a\\b","a\r\nDELE bad"}) assert(!guiSafeLeaf(s));
    assert(!guiSafeLeaf(std::string(181,'a')));
    const std::string listing="VTS_01_1.VOB\r\nVIDEO_TS.IFO\r\nVIDEO_TS.IFO\r\n";
    std::vector<GuiEntry> files;
    assert(guiParseListing(reinterpret_cast<const unsigned char*>(listing.data()),listing.size(),files));
    assert(files.size()==2 && files[0].name=="VIDEO_TS.IFO");
    const std::string bad="../bad\r\n";
    assert(!guiParseListing(reinterpret_cast<const unsigned char*>(bad.data()),bad.size(),files));
    const std::string nul("ok\0bad\n",7);
    assert(!guiParseListing(reinterpret_cast<const unsigned char*>(nul.data()),nul.size(),files));
    assert(guiParent("/mnt/disc/VIDEO_TS",false)=="/mnt/disc");
    assert(guiParent("/mnt/disc",false)=="/mnt/disc");
    assert(guiParent("/etc",false)=="/mnt/disc");
    assert(guiParent("/mnt/discother/escape",false)=="/mnt/disc");
    assert(guiParent("/data/MediaRipper/IFO_TEST",true)=="/data/MediaRipper");
    assert(std::string(guiFileKind("vts_01_1.vob"))=="Video file");
}
int main(int argc,char** argv) {
    modelTests();
    if(argc<3) {std::cout<<"GUI model tests passed\n";return 0;}
    trace.open(argv[2]); if(!trace) return 1;
    Scene2D scene;GuiPainter painter{scene,22,28,36,54,84};GuiView view;
    const int scenario=std::stoi(argv[1]);
    if(scenario==1) {
        view.screen=GuiScreen::Browser;view.menu=1;view.data.path="/mnt/disc/VIDEO_TS";
        for(const char* s:{"VIDEO_TS.BUP","VIDEO_TS.IFO","VTS_01_0.BUP","VTS_01_0.IFO","VTS_01_1.VOB","VTS_01_2.VOB","VTS_01_3.VOB","VTS_01_4.VOB"}) view.data.entries.push_back({s,false,false});
        view.selectedFile=4;
    }
    if(scenario==2) {view.screen=GuiScreen::Test;view.menu=2;view.data.testStep=4;view.data.testVerified=true;view.data.status="Small-file test passed";view.data.savedPath="/data/MediaRipper/IFO_TEST_1792608833_000/VIDEO_TS/VIDEO_TS.IFO";}
    if(scenario==3) {view.screen=GuiScreen::Ftp;view.menu=5;view.data.ip="192.168.77.2";}
    if(scenario==4) {view.screen=GuiScreen::Settings;view.menu=6;}
    if(scenario==5) {view.screen=GuiScreen::Details;view.menu=1;view.data.fileName="VTS_01_1.VOB";view.data.fileSize="1073709056 bytes";view.data.path="/mnt/disc/VIDEO_TS";}
    if(scenario==6) {view.screen=GuiScreen::Test;view.menu=2;view.data.busy=true;view.data.testStep=2;view.data.status="Running the 8 KB HDD test";view.data.activity="Writing to a unique HDD folder";}
    if(scenario==7) {view.screen=GuiScreen::Unavailable;view.menu=4;view.unavailableTitle="Export to USB";view.unavailableMessage="USB export will be added after the copy engine is validated.";}
    if(scenario>=8) {
        view.screen=GuiScreen::Copy;view.menu=3;view.data.fileName="VTS_01_1.VOB";
        view.data.copyTotal=1073709056;view.data.copyDone=536854528;view.data.copySpeed=1850000;
        view.data.freeBytes=40ull*1024*1024*1024;
        view.data.freeSpaceKnown=true;
        view.data.copyPath="/data/MediaRipper/FILE_1792614631_0/VIDEO_TS/VTS_01_1.VOB";
        view.data.busy=scenario!=10;view.data.copyActive=view.data.busy;
        view.data.copyVerifying=scenario>=9;
        view.data.status=scenario==8 ? "Copying selected file" : "Verifying saved video";
        view.data.activity="RETR /mnt/disc/VIDEO_TS/VTS_01_1.VOB";
        if(scenario==10) {
            view.data.copyDone=view.data.copyTotal;view.data.copyVerified=true;
            view.data.status="File saved and verified";
            view.data.copyHash=std::string(64,'a');
        }
        if(scenario==11) {
            view.data.busy=false;view.data.copyActive=false;view.data.freeSpaceKnown=false;
            view.data.freeBytes=0;view.data.copyDone=0;view.data.copyTotal=8192;view.data.copyPath.clear();
            view.data.status="Operation failed";view.data.activity="HDD free-space query failed; copy was not started (see log)";
        }
        if(scenario==12) {
            view.data.busy=false;view.data.copyActive=false;view.data.freeSpaceKnown=false;
            view.data.copyDone=0;view.data.copyTotal=8192;view.data.copyPath.clear();
            view.data.copyNeedsSpaceConfirmation=true;view.data.copyVerifying=false;
            view.data.status="Check PS4 storage first";
        }
        if(scenario==13) {view.data.freeSpaceKnown=false;view.data.copyManualSpace=true;}
    }
    painter.draw(view);
    assert(!outside);
    std::cout<<"GUI model and render-bounds checks passed, scenario "<<scenario<<"\n";
}
