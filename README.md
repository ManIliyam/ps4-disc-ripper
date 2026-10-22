# PS4 Disc Ripper

Created by iTZiLiYA. Version 1.20 is an experimental release.

![PS4 Disc Ripper icon](assets/gui/icon0.png)

PS4 Disc Ripper copies individual accessible disc files to the PS4's internal
HDD. You can then download them to Windows through GoldHEN FTP. Files keep their
original contents and directory paths. The app does not convert or re-encode them.

## What I've tested

I tested the app on firmware 11.00 with GoldHEN 2.4b18.10, using one DVD with
VIDEO_TS files. Browsing and controller navigation worked. An 8192-byte
`VIDEO_TS.IFO` copied successfully, and its SHA-256 matched the copy downloaded
earlier to Windows.

A roughly 1 GiB `VTS_01_1.VOB` also copied and passed the app's HDD readback check.
After transferring it to Windows, the video played correctly.

Other firmware versions and disc formats haven't been tested. I haven't recorded
the console model, DVD manufacturer or filesystem revision for these tests.

## Installation

You'll need a jailbroken PS4, a readable disc and GoldHEN FTP enabled on port
2121. Leave FTP enabled even when copying to the PS4 HDD. The app uses that
service to read the disc.

Install `PS4-Disc-Ripper-1.20.pkg` through GoldHEN Package Installer.

The title ID is `BREW04117`. Installing this version replaces this project's
earlier diagnostics with the same ID. Check that an unrelated app isn't already
using it before installing.

## Copying a file

1. Enable GoldHEN FTP, insert the disc and wait for the PS4 to recognize it.
2. Open **Browse disc**, open a folder and select a file.
3. On **File details**, press **X** to start the copy process.
4. If the app asks you to check storage, open **PS4 Settings > Storage > System
   Storage**. Make sure there's room for the file plus the displayed 512 MiB
   reserve, then return and select **I checked: enough free space**.
5. Wait through **Disc copy** and **HDD readback**. The app shows **File saved and
   verified** when both stages finish successfully.

Automatic free-space checks don't work on my test setup. The manual confirmation
doesn't measure or reserve space, so the display stays at **Unknown**. Copying
will fail if the HDD fills.

Press Circle to request cancellation. It may take time if a system call is
waiting on the disc or network.

Each copy gets a new folder under `/data/MediaRipper/`. For example:

```text
/data/MediaRipper/FILE_<time>_<counter>/VIDEO_TS/VTS_01_1.VOB
```

## Transferring to Windows

Use FileZilla or WinSCP to connect to the PS4's IP address on port 2121. Choose
plain FTP and use it on a trusted local network.

Download the file from the path shown by the app. Compare its SHA-256 with the
app's result, then check playback. See the [Windows transfer instructions](docs/WINDOWS-FTP.md)
for details.

## Current limitations

- The PS4 app copies one file at a time. Whole-disc and folder copying aren't
  included. **Extract disc** still opens the older 8 KB test; **Selected files**
  opens the browser for individual copies.
- USB export, CD audio ripping, video previews and a Windows receiver controlled
  by the PS4 app aren't included in this release.
- Blu-ray, VRO, unfinalized or multisession recordings, and damaged discs haven't
  been tested.
- There is no CSS, AACS, BD+ or CPRM decryption. The app doesn't read raw sectors,
  mount filesystems or change kernel permissions.
- Forced closure or power loss can leave partial files or empty filename
  reservations. These aren't completed copies.
- The readback hash checks saved bytes against the bytes received through FTP.
  It doesn't establish whether the original recording is undamaged. A single
  VOB is not a complete DVD backup.

## Building and reporting problems

- [Build instructions](docs/BUILD.md)
- [Compatibility and test results](docs/COMPATIBILITY.md)
- [Copy engine and troubleshooting](docs/VIDEO-COPY.md)
- [Release notes](docs/RELEASE-v1.20.md)
- [Roadmap](docs/ROADMAP.md)

For a bug report, include the app version, firmware, GoldHEN version, console
model, disc type and relevant lines from `/data/MediaRipper-gui-*.log`. Remove
personal filenames or network addresses before posting a log. Please don't
upload your recordings.

## License and credits

The combined GUI uses GPL-3.0 terms because it includes OpenOrbis Scene2D code;
see [COPYING](COPYING). Original project code keeps its [MIT license](LICENSE).
PicoSHA2 is MIT licensed, and the Gontserrat font includes an OFL notice. The
[third-party notices](docs/THIRD-PARTY.md) and corresponding-source ZIP include
the license details.

PS4 Disc Ripper is an independent homebrew project. It isn't affiliated with Sony
or GoldHEN.
