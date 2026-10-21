# Technical feasibility — 20 October 2026

Historical initial assessment. For verified subsequent console results, including
working loopback IFO/VOB copying, see COMPATIBILITY.md and the current README.

## Decision

Proceed with a mounted-filesystem diagnostic. Do not yet promise a DVD ripper on
firmware 11.00. No console is connected, and no tested homebrew access result for
this exact firmware/model/GoldHEN combination has been established here.

| Media / feature | Evidence | Project status |
|---|---|---|
| Finalized personal DVD video/VR | Sony lists DVD-ROM, DVD-R/RW, DVD+R/RW playback | Candidate for mounted-file recovery; homebrew access unverified |
| Personal BD-R/RE video | Sony lists BDAV/BDMV playback | Candidate for mounted-file copying; 2.50/2.60 and data-only discs require individual tests |
| CD-DA | Sony lists CDs as unsupported | Disabled; no verified 11.00 CD-DA ripping path |
| Unfinalized DVD | Sony lists as unsupported | No recovery claim; use original recorder to finalize or a PC recovery workflow |
| Multisession / ISO9660 / UDF revisions | No tested firmware matrix established | Unknown, not inferred from playback |
| DRM media | No decryption implemented | Outside scope, including CSS/AACS/BD+/CPRM |

[Sony's format list](https://www.playstation.com/en-gb/support/hardware/play-video-music-discs-usb-drives/)
also excludes BD-RE 1.0 and BD-XL and documents one-time playback activation.
Playback support is not a documented application API, nor a promise that a
particular worn, incompatible, or incompletely recorded disc will mount.

## Verified source mechanisms and their limits

1. **OpenOrbis userspace interfaces.** The public toolchain provides headers,
   import libraries and packaging tools. Its
   [libkernel header](https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain/blob/master/include/orbis/libkernel.h)
   declares open/read/stat/directory-related functions. Declarations establish
   callable interfaces, not optical permissions or media compatibility. Some
   header entries explicitly have incomplete prototypes; do not use those as
   an optical SDK. This diagnostic instead uses libc POSIX filesystem calls,
   whose exact release link/runtime support still needs compilation and testing.

2. **Mounted files.** Al-Azif's
   [disc_mount.c](https://github.com/Al-Azif/ps-disc-detect/blob/main/src/disc_mount.c)
   explicitly uses `/mnt/disc`, directory enumeration, stat and read-only file
   reads. This is concrete source evidence for the probe location. It does not
   prove auto-mounting for personal DVDs or access from an installed app's sandbox
   on 11.00. An existing empty directory does not prove a disc is mounted.

3. **Raw logical sectors.** Its
   [disc_dump.c](https://github.com/Al-Azif/ps-disc-detect/blob/main/src/disc_dump.c)
   contains read-only `/dev/cd0` streaming and media/sector-size ioctls. These
   are source-backed device names and an alternative investigation path, not
   an approved firmware-specific ABI for our OpenOrbis app. Logical block reads
   are not equivalent to recovering physical sectors or decrypting content.

4. **CAM pass-through.** Its
   [detect_disc.c](https://github.com/Al-Azif/ps-disc-detect/blob/main/src/detect_disc.c)
   opens `/dev/xpt0`, discovers a pass device, then uses CAM commands for profile
   detection. It distinguishes PS4 CCB/ioctl layouts from PS5. This is a different
   payload SDK and execution context; copying ioctl numbers or FreeBSD host
   structs into OpenOrbis would be unjustified. The source opens CAM endpoints
   read/write and contains speed-setting commands. None are used by our probe.
   Firmware coverage, privileges and sandbox visibility require independent tests.

5. **GoldHEN.** Its [public repository](https://github.com/GoldHEN/GoldHEN)
   documents homebrew enabling and FTP on port 2121, but explicitly says its
   implementation source is private. An audit of its optical permission changes
   is therefore unavailable from that repository. Do not infer unrestricted
   device access, root privileges, or successful application sandbox escape.

The [ps-disc-detect repository](https://github.com/Al-Azif/ps-disc-detect) is GPL-3.0
and builds payload ELFs using ps4-payload-sdk, not OpenOrbis PKGs. Its README makes
model-dependent CD claims. Those claims do not establish CD-DA operation on our
target and conflict with treating all PS4 models as interchangeable. No code
from it is incorporated here. A future reuse requires license review and exact
revision pinning, plus a controlled hardware test; do not run its automatic dump
loop as the read-only phase-2 diagnostic.

## What the diagnostic establishes

It records a bounded directory inventory and file-prefix read results. It does
not obtain a mount table, filesystem revision, physical profile, disc label,
capacity, free space, track TOC or full-file integrity. These remain UNKNOWN.
VIDEO_TS/DVD_RTAV/BDMV/BDAV names are layout evidence only, not physical type or
proof of unprotected content. A successful prefix read cannot establish that
the remainder of a VOB/VRO/M2TS is readable or playable.

ENOENT means that path was absent in this process namespace at that moment;
EACCES means access was denied. Neither alone proves a bad disc. EIO is an I/O
failure, not proof of DRM. Record time since insertion, app context and whether
Sony's player recognizes the same disc. No mount/privilege workaround is justified
until these observations isolate the failure.

If mounted access fails, compare namespace visibility through the existing FTP
server and the packaged probe. An FTP-visible file does not imply app visibility.
Only after that investigate a separate, explicitly controlled device diagnostic
against the payload SDK's exact headers and firmware ABI. No new kernel patches
are proposed. Practical fallback: a compatible PC DVD/BD drive; CD-DA recovery
should use a known-supported CD drive, and unfinalized recordings may need the
original recorder. PS4 Linux optical support is not established by this research.

## Research limits

The linked upstream files were inspected via web access. A local clone timed out;
no pinned source checkout or executable verification was obtained. Links above
refer to mutable branches: save commit IDs/hashes before making a release.
No public source inspection can substitute for an 11.00 console test. This report
deliberately stops short of claiming working extraction or optical hardware limits
based solely on vendor playback policy.
