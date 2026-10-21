# PS4 Disc Ripper

**Created by iTZiLiYA — v1.15 experimental early access**

![PS4 Disc Ripper icon](assets/gui/icon0.png)

A native PS4 app for copying individual accessible disc files to the internal HDD,
then transferring them to Windows through GoldHEN FTP. Original bytes and the
selected file's directory structure are preserved; no transcoding is done.

## Tested features

The developer reports successful tests on firmware **11.00 with GoldHEN
2.4b18.10**, using one DVD containing VIDEO_TS files:

- Controller UI and corrected font rendering.
- DVD browsing through GoldHEN FTP on the same console.
- An 8192-byte VIDEO_TS.IFO copy, with SHA-256 matching an earlier Windows reference.
- A roughly 1 GiB VTS_01_1.VOB copy, successful HDD SHA-256 readback, FTP transfer
  to Windows, and working playback reported by the developer.

These results cover those files on that setup, not general DVD/BD compatibility.
Console model, media manufacturer and filesystem revision were not recorded.

## Installation and copying

Requires a jailbroken PS4, accessible disc files, GoldHEN FTP enabled on port
**2121** (even for internal copying), and enough internal space for the file plus
a **512 MiB reserve**. Other firmware/GoldHEN combinations are unvalidated.

1. Install `PS4-Disc-Ripper-1.15.pkg` through GoldHEN Package Installer.
2. Enable GoldHEN FTP, insert the disc and wait for console recognition.
3. Open **Browse disc**, open a folder, then select a file.
4. On **File details**, press **X** to copy.
5. If prompted, inspect **PS4 Settings > Storage > System Storage**. Return and
   confirm **I checked: enough free space** only when the displayed requirement
   is satisfied. Automatic free-space measurement is denied on the tested setup.
6. Wait through **Disc copy** and **HDD readback**. Only **File saved and verified**
   indicates completion. Circle requests cancellation.

Files go into unique folders under `/data/MediaRipper/`, preserving the selected
file's path, for example `FILE_<time>_<counter>/VIDEO_TS/VTS_01_1.VOB`.
Connect FileZilla or WinSCP to the PS4 IP, port 2121, using plain FTP on a trusted
LAN. Download, compare SHA-256, then test playback. See [Windows FTP](docs/WINDOWS-FTP.md).

Title ID: `BREW04117`. This replaces this project's earlier diagnostics with that
ID. It is a development ID, not globally assigned ownership; check any existing
unrelated application with the same ID before installing.

## Limits

- **One-file copying only.** No whole-disc/folder copy yet. Extract disc opens the
  older 8 KB diagnostic; Selected files opens the browser for one-file copying.
- Manual space confirmation is a user report, not a measured/reserved amount.
  A full HDD causes failure. Free space remains labeled Unknown.
- CD-DA, USB export, desktop receiver/control and video previews are absent.
- Blu-ray, VRO, unfinalized/multisession media and damaged discs are unvalidated.
- No CSS/AACS/BD+/CPRM decryption, raw sectors, mounts or kernel changes.
- Blocking OS calls may delay cancellation. Power loss or forced closure can
  leave partial files or empty filename reservations.
- SHA-256 verifies saved bytes against received bytes, not original media health.
  One VOB is not a complete DVD archive.

## Development

- [Build](docs/BUILD.md)
- [Compatibility and test evidence](docs/COMPATIBILITY.md)
- [Copy engine and troubleshooting](docs/VIDEO-COPY.md)
- [Release notes](docs/RELEASE-v1.15.md)
- [Roadmap](docs/ROADMAP.md)

For bug reports include app version, firmware, GoldHEN version, console model,
disc type and relevant `/data/MediaRipper-gui-*.log` lines. Review logs for personal
file names/network addresses before posting; do not upload recordings.

## Licensing

The combined GUI distribution uses **GPL-3.0 terms** because it includes OpenOrbis
Scene2D code: see [COPYING](COPYING). Original project code retains its [MIT
license](LICENSE). PicoSHA2 is MIT licensed; Gontserrat includes an OFL notice.
See [third-party notices](docs/THIRD-PARTY.md) and the corresponding-source ZIP.

This independent homebrew project is not affiliated with Sony or GoldHEN.
