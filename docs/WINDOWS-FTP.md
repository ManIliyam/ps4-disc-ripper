# Windows transfers

Keep PS4 and PC on the same trusted LAN. Enable GoldHEN FTP on port **2121**.
The app's FTP information page displays the PS4 IP when available.

In WinSCP choose FTP, no encryption, PS4 IP and port 2121. In FileZilla enter the
same host/port and anonymous login if allowed by GoldHEN. The app currently expects
anonymous local FTP access. Use binary transfers and save archives in new folders.

Browse `/data/MediaRipper/`, open the `FILE_...` folder displayed by the app and
download the file. In Windows PowerShell:

```powershell
Get-FileHash -Algorithm SHA256 -LiteralPath 'D:\YourArchive\VIDEO_TS\VTS_01_1.VOB'
```

Replace the example path. Compare with the app's hash, then test playback. A
single VOB is not a complete DVD backup. In Ubuntu use `sha256sum /path/to/file`.

Direct Ethernet needs compatible addresses on both machines. This app does not
configure Windows/PS4 networking or provide a desktop receiver.
