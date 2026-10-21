#pragma once
#include <string>
#include <vector>
#include <algorithm>
#include <cstdint>

enum class GuiScreen { Overview, Browser, Test, Ftp, Settings, Unavailable, Details, Copy };
struct GuiEntry {
    std::string name;
    bool directory = false;
    bool typeKnown = false;
};
struct GuiSnapshot {
    bool busy = false;
    bool local = false;
    bool scanned = false;
    bool testVerified = false;
    unsigned testStep = 0;
    std::string status = "Ready when you are";
    std::string activity = "Insert a personal disc, then choose Detect disc.";
    std::string path = "/mnt/disc";
    std::string ip = "Unavailable";
    std::string layout = "Not inspected";
    std::string fileName;
    std::string fileSize = "Unknown";
    std::string savedPath;
    std::string logPath;
    bool detailsReady = false;
    bool directoryReady = false;
    bool copyActive = false, copyVerifying = false, copyVerified = false;
    bool freeSpaceKnown = false;
    bool copyNeedsSpaceConfirmation=false, copyManualSpace=false;
    uint64_t copyDone=0, copyTotal=0, freeBytes=0;
    double copySpeed=0;
    std::string filePath, copyPath, copyHash;
    std::vector<GuiEntry> entries;
};
struct GuiView {
    GuiScreen screen = GuiScreen::Overview;
    GuiSnapshot data;
    int menu = 0;
    int selectedFile = 0;
    unsigned frame = 0;
    bool controller = true;
    std::string unavailableTitle;
    std::string unavailableMessage;
};
inline bool guiSafeLeaf(const std::string& value) {
    if (value.empty() || value.size() > 180 || value == "." || value == "..") return false;
    for (unsigned char c : value) if (c < 32 || c > 126 || c == '/' || c == '\\') return false;
    return true;
}
inline bool guiParseListing(const unsigned char* bytes, unsigned count, std::vector<GuiEntry>& output) {
    output.clear();
    std::string leaf;
    for (unsigned i = 0; i <= count; ++i) {
        const unsigned char c = i == count ? '\n' : bytes[i];
        if (c == '\r') continue;
        if (c == '\n') {
            if (leaf.empty()) continue;
            if (!guiSafeLeaf(leaf) || output.size() >= 256) return false;
            if (std::none_of(output.begin(), output.end(), [&](const GuiEntry& e) { return e.name == leaf; }))
                output.push_back({leaf, false, false});
            leaf.clear();
        } else {
            if (c < 32 || c > 126 || leaf.size() >= 180) return false;
            leaf += static_cast<char>(c);
        }
    }
    std::sort(output.begin(), output.end(), [](const GuiEntry& a, const GuiEntry& b) { return a.name < b.name; });
    return true;
}
inline std::string guiParent(std::string path, bool local) {
    const std::string root = local ? "/data/MediaRipper" : "/mnt/disc";
    if (path == root || path.size() <= root.size() || path.compare(0, root.size()+1, root+"/") != 0) return root;
    const size_t slash = path.find_last_of('/');
    return slash == std::string::npos || slash < root.size() ? root : path.substr(0, slash);
}
inline const char* guiFileKind(const std::string& name) {
    std::string upper = name;
    std::transform(upper.begin(), upper.end(), upper.begin(), [](unsigned char c) { return c >= 'a' && c <= 'z' ? c - 32 : c; });
    if (upper.size() >= 4) {
        const std::string ext = upper.substr(upper.size()-4);
        if (ext == ".VOB" || ext == ".VRO" || ext == ".MP4" || ext == ".MTS") return "Video file";
        if (ext == ".IFO") return "DVD navigation";
        if (ext == ".BUP") return "DVD navigation backup";
        if (ext == ".WAV") return "Audio file";
    }
    if (upper.size() >= 5 && upper.substr(upper.size()-5) == ".M2TS") return "Blu-ray video file";
    return "File";
}
