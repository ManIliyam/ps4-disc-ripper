# v0.6 / package 1.05 — controlled HDD write/readback

Prerequisite demonstrated on the user's console by v0.5: local FTP login,
listing of VIDEO_TS, complete 8192-byte VIDEO_TS.IFO read, EOF and FTP 226.
The next gate is storing that same small fixture on the internal HDD.

The FTP reader retains at most 64 KiB in RAM. Only an exact 8192-byte payload
beginning DVDVIDEO-VMG is eligible for this trial. It creates a unique directory
using mkdir, retains VIDEO_TS/VIDEO_TS.IFO structure, opens a .partial file with
O_EXCL/O_NOFOLLOW, handles short writes and limited EINTR, fsyncs/closes it,
reopens it and compares every byte against the FTP buffer. Exact EOF is checked.
Only after successful verification is it renamed to VIDEO_TS.IFO. Failures try
to remove the partial and explicitly log cleanup failures; empty test folders
can remain. Final path and verification status go to the normal diagnostic log.

The outcome is not known until console testing. Readback is a logical verification,
potentially served from cache; it does not establish power-loss persistence or
independently validate source integrity. The next check is downloading the saved
IFO and comparing SHA-256 with the original Windows reference from this disc.
There is no SHA-256 implementation in the app yet.

Limits: this is not the general extraction engine. No VOBs are copied. No free-space
preflight exists for this fixed 8 KiB test; storage failures are checked and logged.
Full-disc extraction stays disabled until a verified free-space query, streaming
copy, cancellation, native network error/timeout adapter and progress UI exist.
Existing network blocking limitations from v0.5 remain. All source-disc operations
use FTP reads; HDD writes stay under /data/MediaRipper. No kernel changes.

Expected output:
`/data/MediaRipper/IFO_TEST_<timestamp>_<counter>/VIDEO_TS/VIDEO_TS.IFO`

Use the same known-good test disc with GoldHEN FTP enabled. This is a storage
test, not a recovery claim for encrypted, damaged or unfinalized media.
