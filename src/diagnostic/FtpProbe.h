// Included inside the diagnostic namespace, after logf() and name().
// Read-only experiment: no FTP write, delete, rename or mount commands.
struct ProbeSocket {
    int id = -1;
    ~ProbeSocket() { if (id >= 0) sceNetSocketClose(id); }
    bool connectTo(const char* host, unsigned port) {
        id = sceNetSocket("MediaRipperProbe", ORBIS_NET_AF_INET, ORBIS_NET_SOCK_STREAM, 0);
        if (id < 0) { logf("FTP socket raw=0x%08x\n", static_cast<unsigned>(id)); return false; }
        OrbisNetSockaddr address{};
        address.len = sizeof(address);
        address.sa_family = ORBIS_NET_AF_INET;
        const uint16_t netPort = sceNetHtons(static_cast<uint16_t>(port));
        std::memcpy(address.sa_data, &netPort, sizeof(netPort));
        if (sceNetInetPton(ORBIS_NET_AF_INET, host, address.sa_data + 2) != 1) return false;
        logf("FTP connect begin host=%s port=%u\n", host, port);
        const int result = sceNetConnect(id, &address, sizeof(address));
        logf("FTP connect result=0x%08x\n", static_cast<unsigned>(result));
        return result == 0;
    }
};

struct ProbeFtp {
    ProbeSocket control;
    char last[1024]{};
    int reply() {
        // Absolute polling budget per reply; bound multiline responses as well.
        unsigned budget = 1000;
        int multiline = -1;
        for (unsigned lines = 0; lines < 32; ++lines) {
            size_t length = 0;
            bool ended = false;
            while (budget && length + 1 < sizeof(last)) {
                --budget;
                char c = 0;
                // Native value verified in Scene-Collective/ps4-payload-sdk
                // network.h. libc's 0x40 means WAITALL to sceNetRecv.
                const int n = sceNetRecv(control.id, &c, 1, 0x80);
                if (!n) { logf("FTP control EOF\n"); return -1; }
                if (n < 0) { sceKernelUsleep(10000); continue; }
                if (c == '\n') { ended = true; break; }
                if (c != '\r') last[length++] = c;
            }
            last[length] = 0;
            if (!ended) { logf("FTP reply timeout/length limit\n"); return -1; }
            logf("FTP reply "); name(last); logf("\n");
            if (length < 4 || last[0] < '0' || last[0] > '9' ||
                last[1] < '0' || last[1] > '9' || last[2] < '0' || last[2] > '9') {
                if (multiline >= 0) continue;
                return -1;
            }
            const int code = (last[0]-'0')*100 + (last[1]-'0')*10 + last[2]-'0';
            if (multiline < 0 && last[3] == '-') { multiline = code; continue; }
            if (last[3] == ' ' && (multiline < 0 || multiline == code)) return code;
        }
        return -1;
    }
    int command(const char* text) {
        logf("FTP command %s\n", text);
        char line[256];
        const int length = std::snprintf(line, sizeof(line), "%s\r\n", text);
        if (length < 0 || static_cast<size_t>(length) >= sizeof(line)) return -1;
        int offset = 0;
        for (unsigned tries = 0; offset < length && tries < 32; ++tries) {
            // libc's MSG_DONTWAIT is 0x40 in the tested SDK. Do not pass it
            // to native send: v0.4 received 0x80410116 before sending login.
            // Use the native API's default flags for this controlled experiment.
            const int n = sceNetSend(control.id, line + offset, length - offset, 0);
            if (n <= 0) { logf("FTP send failed raw=0x%08x\n", static_cast<unsigned>(n)); return -1; }
            offset += n;
        }
        return offset == length ? reply() : -1;
    }
    bool transfer(const char* host, const char* request, bool listing,
                  unsigned char* contents = nullptr, unsigned* received = nullptr) {
        if (received) *received = 0;
        if (command("PASV") != 227) return false;
        const char* start = std::strchr(last, '(');
        unsigned a,b,c,d,p,q;
        if (!start || std::sscanf(start, "(%u,%u,%u,%u,%u,%u)", &a,&b,&c,&d,&p,&q) != 6 ||
            a>255 || b>255 || c>255 || d>255 || p>255 || q>255 || !(p*256+q)) {
            logf("FTP invalid PASV response\n"); return false;
        }
        ProbeSocket data;
        // Pin data host to the same console, ignoring the server-advertised IP.
        if (!data.connectTo(host, p*256+q)) return false;
        const int preliminary = command(request);
        if (preliminary != 125 && preliminary != 150) return false;
        unsigned total = 0;
        bool eof = false;
        unsigned char first[12]{};
        unsigned firstSize = 0;
        for (unsigned attempt = 0; attempt < 2000; ++attempt) {
            char bytes[1025];
            const int n = sceNetRecv(data.id, bytes, sizeof(bytes)-1, 0x80);
            if (!n) { eof = true; break; }
            if (n < 0) { sceKernelUsleep(10000); continue; }
            if (total + static_cast<unsigned>(n) > 65536) {
                logf("FTP data limit exceeded; incomplete\n"); return false;
            }
            for (int i = 0; i < n && firstSize < sizeof(first); ++i) first[firstSize++] = bytes[i];
            if (contents) std::memcpy(contents + total, bytes, static_cast<size_t>(n));
            total += n;
            if (listing) { bytes[n] = 0; logf("FTP listing "); name(bytes); logf("\n"); }
        }
        if (!eof) { logf("FTP data timeout; incomplete bytes=%u\n", total); return false; }
        const int complete = reply();
        logf("FTP transfer bytes=%u final=%d first_bytes=", total, complete);
        for (unsigned i = 0; i < firstSize; ++i) logf("%02x", first[i]);
        logf("\n");
        // Completion response + EOF required. Not a full DVD integrity test.
        const bool ok = (listing || total > 0) && (complete == 226 || complete == 250);
        if (ok && received) *received = total;
        return ok;
    }
};

void ftpProbe() {
    logf("FTP libc MSG_DONTWAIT=0x%x; native send uses flags=0\n", MSG_DONTWAIT);
    const int init = sceNetInit();
    logf("FTP sceNetInit raw=0x%08x; testing existing service only\n", static_cast<unsigned>(init));
    // A preinitialized network stack may report a negative result; log socket results.
    for (const char* host : {"127.0.0.1", "192.168.77.2"}) {
        ProbeFtp ftp;
        if (!ftp.control.connectTo(host, 2121) || ftp.reply() != 220) continue;
        int login = ftp.command("USER anonymous");
        if (login == 331) login = ftp.command("PASS MediaRipperDiagnostic");
        if (login != 230 || ftp.command("TYPE I") != 200) continue;
        const bool listing = ftp.transfer(host, "NLST /mnt/disc/VIDEO_TS/", true);
        logf("FTP directory result=%s\n", listing ? "success" : "failed");
        // A failed transfer may leave control replies pending: stop this session.
        if (!listing) continue;
        static unsigned char contents[65536];
        unsigned received = 0;
        const bool reading = ftp.transfer(host, "RETR /mnt/disc/VIDEO_TS/VIDEO_TS.IFO", false, contents, &received);
        logf("FTP IFO read result=%s\n", reading ? "success" : "failed");
        if (reading) {
            ftp.command("QUIT");
            const bool saved = saveTestIfo(contents, received);
            logf("HDD extraction test result=%s\n", saved ? "success" : "failed");
            return;
        }
    }
    logf("FTP probe ended without confirmed IFO read\n");
}
