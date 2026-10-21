# Diagnostic v0.2 / package version 1.01

The user built and installed v0.1 on firmware 11.00. Its log reached END but
the application displayed CE-34878-0. Both /mnt and /mnt/disc returned ENOENT.
Separately, GoldHEN FTP listed a test DVD and transferred a roughly 1 GiB VOB
that the user reported played correctly. This proves access for that disc in
the FTP context, not direct access from our packaged application.

Version 0.2 adds a one-level root inventory and read-only native sceKernelOpen
probes alongside the existing libc inventory. Native results are logged raw;
they are not interpreted as libc errno. A native prefix read of the observed
VIDEO_TS.IFO path is limited to 4096 bytes and remains in RAM. No privilege,
mount, sandbox or device modifications are performed.

The process now sleeps after closing its log instead of returning from main.
This tests the hypothesis that application teardown caused the crash. It is
not a verified fix. Fsync/close stage markers help narrow later crashes.
There is still no rendered UI: after 15 seconds use PS > Close Application.
Kernel calls, read permissions and shutdown behavior need console testing.

Sources: [OpenOrbis kernel declarations](https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain/blob/master/include/orbis/libkernel.h)
and [upstream sample lifetime](https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain/blob/master/samples/hello_world/hello_world/main.cpp).

Build in a fresh staging directory using `python3 tools/prepare_build.py --output
diagnostic-v02`. Package version 1.01 retains title ID BREW04117. Build pkg.gp4
with make and package it with the user's locally compiled .NET 8 PkgTool.Core.
Do not rerun the old bundled pkg_build on Ubuntu 24.04: its OpenSSL runtime
already failed. Log filenames remain exclusive and old logs are preserved.
