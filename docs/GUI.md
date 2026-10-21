# PS4 Disc Ripper GUI 1.10

Historical 1.10 design notes. For the released 1.15 capabilities and corrected
FTP/file/copy behavior, see README, COMPATIBILITY.md and VIDEO-COPY.md.

Created by **iTZiLiYA**. The exact credit appears in the persistent sidebar,
About screen, icon metadata and packaged author.txt. The same BREW04117 title
and content ID are retained so the package can replace the diagnostics.

## What is implemented

- Native 1920x1080 dark UI using OpenOrbis Scene2D / SceVideoOut and Gontserrat.
- One controller read per frame, edge-triggered Cross/Circle/Triangle, D-pad
  navigation and held-direction repeat. Reconnection is attempted periodically.
- Detect/Browse queries GoldHEN FTP through loopback; no direct sandbox escape.
  Directory names are obtained live, never populated with the test-disc fixture.
  Selecting an entry uses CWD to distinguish a directory, or SIZE for file details.
- Folder navigation stays within /mnt/disc; local saved-archive browsing stays
  within /data/MediaRipper. Returned names reject traversal, separators and controls.
- The verified 8 KB IFO test runs only after explicitly opening Extract disc
  and selecting Run 8 KB HDD test. Nothing is copied automatically on launch.
- Background worker for FTP and file operations; the main thread keeps drawing
  and reading the controller. A single job is allowed at a time.
- Test progress shows observed completion stages, not a fabricated speed or ETA.
- FTP screen queries the PS4 address through NetCtl, labels unavailable results,
  and shows the disc/archive paths and configured default port 2121.
- Updated code-drawn disc icon, included font license and author credit.

## Still unavailable

Full DVD/selected-file extraction, USB export, desktop receiver/control, CD-DA,
and video playback are not implemented. Their screens say so. Extract disc opens
the limited test screen; it does not run a full rip. No fake storage, disc label,
filesystem, playback thumbnails, duration or capacity values are shown.

The local FTP mode still requires GoldHEN FTP enabled. Native send uses the
hardware-tested flags=0 fix. Receive flags and connection timing retain the
limitations of v0.5: OS calls can block, and this build has no safe in-app
cancellation of a blocked operation. Close through the PS menu if needed.
The interface can remain responsive while its worker is blocked; this does not
mean the job completed. Only the fixed 8192-byte fixture can be written by the test.

Video preview investigation: the public OpenOrbis AvPlayer.h inspected here
contains incomplete no-argument declarations and does not establish VOB/MPEG-2
support or an appropriate stream/decoder interface. Those declarations are not
used. Selecting a VOB displays its server-reported size/type and a playback
unavailable message. Decoder/container validation is a separate future task.

## Build

The existing OpenOrbis v0.5.4 installation and .NET 8 PkgTool.Core built during
the diagnostics were used. In Linux, from the repository:

```sh
export OO_PS4_TOOLCHAIN=/home/iliya/ps4-dev/openorbis-sdk/OpenOrbis/PS4Toolchain
export DOTNET_SYSTEM_GLOBALIZATION_INVARIANT=1
python3 tools/prepare_gui.py
make -C build/gui-v01 pkg.gp4
cd build/gui-v01
dotnet /home/iliya/ps4-dev/LibOrbisPkg/PkgTool.Core/bin/Release/net8.0/PkgTool.Core.dll pkg_build pkg.gp4 .
dotnet /home/iliya/ps4-dev/LibOrbisPkg/PkgTool.Core/bin/Release/net8.0/PkgTool.Core.dll pkg_validate IV0000-BREW04117_00-DISCRIPPERDIAG01.pkg
```

The stager copies the installed SDK's sample assets without modifying the SDK.
It adds _GNU_SOURCE for the SDK's libc++ locale/thread declarations, binds the
actual source folder name, links graphics/controller/network/FreeType imports,
and includes a GP4 directory normalizer for nested credits/font assets.
It refuses to overwrite an existing staged folder. Header dependencies are
included in the generated Makefile. Pillow is only needed to regenerate the
icon and host preview, not to compile/run the PS4 application.

## Validation completed

Compiled and linked with OpenOrbis using Ubuntu clang 18; SELF and GP4 generated.
The final PKG passed every check reported by the updated PkgTool.Core validator.
Host C++ tests checked name validation, traversal controls, listing parsing,
duplicate removal, parent navigation and file-type labels. Eight render traces
checked rectangle/pixel bounds. Preview PNGs were rendered using the same C++
screen drawing code, traced on a host, and the packaged Gontserrat font. They
were visually inspected. Previews are not screenshots of a running console.

All native GUI display, controller behavior, worker operation and NetCtl behavior
still require testing on firmware 11.00 / GoldHEN 2.4b18.10. The earlier diagnostic
reader and small-file test have user-confirmed hardware results; that does not
automatically validate their new thread/UI integration.

## First console test

Install dist/PS4-Disc-Ripper-1.10.pkg. Confirm version 1.10. Enable GoldHEN FTP
and launch with no disc first: check the visible menu and controller navigation.
Then insert the known-good test DVD, choose Detect disc, open VIDEO_TS and select
a VOB to inspect its size. Circle returns; Triangle refreshes. Optionally open
Extract disc and explicitly run the small IFO test, or Triangle to browse saved
archives. This is not a request to copy a full video. Close with the normal PS
menu. Logs are exclusively created under /data/MediaRipper-gui-*.log.

## Source references

- https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain/tree/master/samples/font
- https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain/tree/master/samples/input
- https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain/tree/master/samples/threading
- https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain/blob/master/include/orbis/NetCtl.h
- https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain/blob/master/include/orbis/AvPlayer.h

Upstream SDK renderer sources and Gontserrat retain their upstream licenses.
The font's OFL.txt and SDK GPL-3.0 are packaged. Our own source remains MIT as in
LICENSE; the combined GUI is distributed under GPL-3.0. See THIRD-PARTY.md.
