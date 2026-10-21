# v0.5 / package 1.04 — isolate native send flags

The v0.4 console log proves sceNetInit succeeded, both local TCP connections
succeeded and the GoldHEN FTP greeting was received. USER send failed on both
connections with raw 0x80410116. No login, directory or file read was demonstrated.
The user inspected the installed SDK sys/socket.h: MSG_DONTWAIT is 0x0040.

This narrow experiment changes native sceNetSend's flags to zero (default),
keeping the rest of the test stable. It does not claim verified PS4-native
nonblocking flag values. Receive still uses the libc macro as in v0.4; its
interpretation by native recv is unresolved. In particular, BSD networking
headers define 0x40 as WAITALL, so the existing polling code does NOT establish
a real receive timeout. Connect and send can also block. Use known-good test
media only and Close Application if the log stops advancing; do not regard
elapsed time as completion. A production reader needs a verified native
nonblocking adapter and proper error handling before extraction is enabled.

The immediate validation question is whether the server answers USER/PASS
after removing the flags rejected during send. No kernel operations or FTP
writes are added. Version 1.04 source is not yet compiled or console-tested.

Reference for the ABI distinction (not proof of PS4-specific semantics):
https://cgit.freebsd.org/src/tree/sys/sys/socket.h
