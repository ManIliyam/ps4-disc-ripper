# Diagnostic v0.4 / package 1.03 — local FTP experiment

This revision tests a backend using GoldHEN's existing FTP service, not direct
optical device access. App access to the service is unproven until the hardware
test succeeds. Leave the test DVD inserted, network settings unchanged and
GoldHEN FTP enabled on 2121. It tries loopback first and the user's previously
confirmed local address 192.168.77.2 second. That second address is fixture-specific,
not a general hardcoded configuration for the finished product.

The read-only FTP sequence uses anonymous login, TYPE I, PASV, NLST on the observed
VIDEO_TS folder, and RETR on VIDEO_TS.IFO. The passive data connection always uses
the same host as the control connection, ignoring the PASV-advertised host.
Data is bounded to 64 KiB per transfer, kept in RAM, and discarded. Only logs
persist. No FTP writes/deletions, kernel changes, mounts or archives are performed.
Complete reads require data EOF and a positive FTP completion reply. The log
contains byte counts and the first twelve bytes for format inspection. This
does not establish protection status or full-disc integrity.

Native declarations and address layouts were inspected in OpenOrbis Net.h and
_types/net.h. The new build links libSceNet. Compilation and PS4 network-stack
initialization/permissions need validation with the user's installed toolchain.
The application logs sceNetInit's raw result and attempts socket operations even
if already-initialized status is returned. Negative receive results are bounded
by polling limits and reported as timeout/incomplete rather than silently accepted.
Nonblocking send/receive flags come from the SDK socket header. Connect calls are
blocking and have no application-enforced deadline; no claim of fully bounded
network latency is made. No network teardown of shared services is attempted.

Allow up to 90 seconds for the experiment, then close through the PS menu. Inspect
the new normal probe log via the existing Windows FTP connection. If there is no
normal log, inspect the startup log under /data as in v0.3. A blank screen is
not proof of successful testing. Stop a failed attempt normally via Close Application.

If local FTP succeeds, it can become a modular reader for HDD archiving. It will
remain dependent on GoldHEN FTP being enabled and will be labeled accordingly.
If it fails, preserve raw error/reply logs before selecting a different approach.

Sources:
- https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain/blob/master/include/orbis/Net.h
- https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain/blob/master/include/orbis/_types/net.h

No new PKG, successful compilation or hardware access is claimed at source-update time.
