# Experimental selected-file copying — 1.15

1.15 adds an explicit per-file manual storage check when the native query is
denied or unavailable. The user must inspect PS4 Settings > Storage and confirm
the displayed requirement (file size plus 512 MiB, rounded up to MiB). No archive
file or archive subdirectory is created until confirmation. The existing base
directory and log may be created. This is a manual fallback, not a repaired
automatic measurement. The app continues to display **Unknown / manually checked**.
Confirmation is not saved as a preference and is invalidated if the file size
changes. Other query errors, invalid results and arithmetic overflow still stop
the job. Cancellation, exact-size checking and SHA-256 readback remain enabled.

Hardware evidence from 1.14: archive-directory open succeeded, but fstatfs failed
with `errno=1` (EPERM). A read-only probe of the running console's FTP service
returned its FEAT list and `502 Command not implemented` for
`AVBL /data/MediaRipper`. GoldHEN's existing FTP therefore did not provide that
alternative space query in this test. No server changes were made.

1.14 replaces the failed dynamic `statfs` export lookup with the installed SDK's
`fstatfs` call on an open archive-directory descriptor. Disassembly verifies that
this wrapper jumps directly to `_fstatfs`, which is present in the SDK's
libkernel import library. This avoids the missing plain `statfs` export, module
name filtering and module enumeration. The console subsequently denied the call.
Unknown free space is displayed as **Unknown**, never as zero GiB. Small files
now display progress in bytes instead of rounded-to-zero MiB.

1.13 fixes the file browser failure observed on GoldHEN FTP v2.2: a regular IFO
received `250` for CWD, followed by `550` for a directory listing. The browser no
longer classifies entries using CWD. It attempts a real listing, then probes SIZE
on a fresh FTP connection after a definite 550 rejection. Other transfer failures
remain errors. Browser status/errors are now visible below the file list.

Created by **iTZiLiYA**. The 1.11 font correction is retained; FPS behavior is unchanged.

## Console test procedure

1. Close the running app normally through the PS menu. Install the 1.15 PKG.
2. Enable GoldHEN FTP (2121), insert the same test DVD, and wait for recognition.
3. Open **Browse disc**, open `VIDEO_TS`, then open `VIDEO_TS.IFO`.
4. On **File details**, press Cross. If automatic checking is unavailable, the
   screen pauses at **Check PS4 storage first**. Inspect free space in PS4
   Settings > Storage (for internal/system storage), return to the app, and
   press Cross on **I checked: enough free space** only if the displayed
   requirement is satisfied. Circle leaves without starting the copy.
5. If it reports **File saved and verified**, repeat with `VTS_01_1.VOB`.
   Each job creates its own `FILE_<time>_<counter>` directory and preserves
   subdirectories below `/mnt/disc`. A single VOB is not a complete DVD archive.
6. Keep the disc inserted. Progress distinguishes copying from HDD readback.
   Circle requests cancellation; wait for completion before closing the app.
7. Download the saved file using FileZilla/WinSCP from `/data/MediaRipper/`.
   Compare its SHA-256 against the app log and the original Windows reference,
   then play the VOB on Windows. Full-disc extraction is not enabled.

If preflight fails, report the on-screen error and newest
`/data/MediaRipper-gui-*.log`. Automatic free-space measurement is currently
unavailable on the tested console. A copy can proceed only after a successful
automatic check or the explicit per-file manual fallback. The new fallback and
streaming transfer still require console testing.

## Engine behavior and limits

- Loopback GoldHEN FTP, binary mode, `SIZE`, pinned passive data host, and `RETR`.
  No FTP writes, optical writes, mounts, kernel patches or privilege changes.
- 64 KiB buffer, short-write handling, bounded EINTR retries, exact byte count,
  EOF and positive FTP completion required. No unread bytes are skipped or filled.
- Data receives use the PS4 native nonblocking flag `0x80`. No-progress errors
  are polled at 10 ms intervals, failing after 30 seconds. The existing control
  reply parser has a bounded polling budget. Socket connect/send and local disk
  operations can still block: immediate cancellation is not guaranteed.
- Space checks require file size plus a 512 MiB reserve. This is a preflight
  snapshot, not a reservation against other apps consuming storage.
  A manual confirmation is a user-reported check, not an enforced reserve: if
  storage fills during copying, the app detects the failed write and stops.
- Unique archive folders, exclusive `.partial` files, fsync and close, followed
  by a separate readback SHA-256 pass. The final filename is reserved exclusively
  and only that job's empty reservation is replaced by the verified partial.
- SHA-256 compares bytes received from FTP with saved HDD bytes. It does not
  prove that an original recording is healthy or that a server read was correct.
- Failures/cancellation remove the partial where filesystem operations succeed;
  empty directories can remain. A crash/power loss may leave `.partial` files
  or an empty final-name reservation. Never treat such remnants as verified.
- No resume, automatic full-file retries, multi-file selection, full DVD copying,
  video previews, USB export, CD-DA or DRM decryption in this version.

## Sources and uncertainty

The installed OpenOrbis SDK supplies native socket interfaces and the descriptor
filesystem-statistics interface. Its `libc.a` disassembly shows `statvfs` calls
`statfs` using a local buffer but does not populate the caller's result, so it is
unsuitable here. The `fstatfs` wrapper instead jumps to `_fstatfs`; `nm -D` confirms
that symbol in the installed `lib/libkernel.so`. `include/sys/statfs.h` declares
the function and `include/bits/statfs.h` defines its PS4 structure. The app uses
those headers, not a hand-declared function signature or Linux structure.

- Native socket constants: [Scene-Collective PS4 payload SDK network.h](https://github.com/Scene-Collective/ps4-payload-sdk/blob/master/libPS4/include/network.h)
  defines `MSG_DONTWAIT=0x80`, `MSG_WAITALL=0x40`. They must not be confused with
  the installed musl socket header's constants.
- Native `statfs` export resolution is used in [Apollo source/sd.c](https://github.com/bucanero/apollo-ps4/blob/main/source/sd.c).
  Apollo loads libkernel_sys. The previous 1.13 attempt to resolve `statfs` from
  loaded modules failed on the user's console. 1.14 no longer uses that approach.
- Structure version `0x20030518` is documented in [FreeBSD 9 mount.h](https://github.com/freebsd/freebsd-src/blob/releng/9.0/sys/sys/mount.h).
  SDK-declared fields are validated for this version, block sizes/counts and
  arithmetic. Unknown results stop preflight. No privileged modules are loaded.
- SHA-256: [PicoSHA2](https://github.com/okdshin/PicoSHA2), MIT license preserved in
  source and package credits. Full header vendored in `src/extraction`.

Source revisions inspected: ps4-payload-sdk `b7326416c23ce14639e9180a24093bdc5eedb579`,
Apollo `29da777575c288e677b342fa21b6cfc45283ef6a`,
PicoSHA2 `161cb3fc4170fa7a3eca9e582cebd27cc4d1fe29`.

## Validation completed on development host

PS4 compilation and package validation are distinct from hardware testing.
The portable engine is exercised under AddressSanitizer/UndefinedBehaviorSanitizer:
multi-buffer transfer and readback, independent Python SHA-256 fixture, standard
SHA-256 `abc` vector, early EOF, excess data, negative completion, receive failure,
copy/readback cancellation, altered saved data, existing final/partial names,
FTP size overflow, path traversal/command injection, and space-prefix validation.
GUI traces cover copying, readback, completion, unknown space and manual-check
confirmation. Required-space overflow and changed-size confirmation are tested.
The new manual flow, actual large optical transfers, cancellation latency and
playback await console tests; fstatfs denial is hardware-confirmed.

Build with `tools/prepare_gui.py`, then `make -C build/gui-v06 pkg.gp4` using the
OpenOrbis environment. Package with `tools/package_gui.py --pkg-tool <net8 DLL>`.
