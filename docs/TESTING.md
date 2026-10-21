# Firmware 11.00 hardware worksheet

This is the original diagnostic worksheet. See COMPATIBILITY.md for the IFO/VOB
tests now completed; the broader matrix below remains pending. Keep backups and begin
with known-good finalized personal test discs. This probe makes no drive writes.

1. Record console CUH model, firmware (11.00), exact GoldHEN version, SDK/compiler
   versions, PKG hash, disc type, recorder, finalization status and known filesystem.
2. Enable GoldHEN and its FTP server. Confirm the diagnostic title ID does not
   collide with an installed app, then install your successfully built PKG.
3. Launch once without a disc. Retrieve the log as the baseline.
4. Close the diagnostic, insert a finalized personal DVD, wait for spin-up, then
   launch it again. Retrieve the new uniquely named log. Record observed wait time.
5. Repeat with DVD VIDEO_TS, DVD_RTAV/VRO and personal BD BDMV/BDAV fixtures where
   available. Test ISO9660/UDF revisions independently; record actual PC metadata.
6. Compare directory names and sizes to a read-only PC listing. Prefix-read success
   is the initial access gate; a later extraction test must read every byte.
7. Include a readable but empty filesystem as a negative control. Distinguish it
   from absent media. Record EACCES, ENOENT and EIO without reinterpreting them.
8. CD/unfinalized/multisession cases are exploratory only. No audio extraction or
   track detection is implemented. Do not use these to establish initial success.
9. If the app appears stuck on optical I/O, record the circumstances and use the
   console's normal Close Application flow. Do not eject media mid-probe as a
   routine test or assume a software timeout exists. Damaged-disc tests come later.

Log results table:

| CUH / GoldHEN | Disc + filesystem | Shell recognition | /mnt listing | /mnt/disc entries | Prefix reads / errno | Result |
|---|---|---|---|---|---|---|
| Pending | No disc | — | Pending | Pending | Pending | Untested |
| Pending | Finalized personal DVD | Pending | Pending | Pending | Pending | Untested |
| Pending | Personal BD | Pending | Pending | Pending | Pending | Untested |

## Windows 10/11: retrieve logs through GoldHEN

Find the PS4 LAN IPv4 address in its network connection status. In GoldHEN enable
the existing FTP server. Its documented default port is 2121:
[GoldHEN README](https://github.com/GoldHEN/GoldHEN#features).

In WinSCP choose FTP, no encryption, PS4 address, port 2121. Use the authentication
expected by your installed GoldHEN version; if none is configured, try anonymous
login. Navigate to `/data/MediaRipper/logs/` and download the probe logs into a new
Windows directory. FileZilla can use equivalent settings. Do not upload or delete
anything on the disc. If listing `/mnt/disc` through FTP is useful, record it as a
separate FTP-context observation, not an app result. FTP is plaintext; use your
trusted local network and do not expose the server through router port forwarding.

No media archive exists in this phase. Later archives will use
`/data/MediaRipper/<unique-archive>/`; transfer them in binary mode and compare
SHA-256 values on Windows using `Get-FileHash -Algorithm SHA256`. Playback checks
and full hashes against PC originals are required before claiming lossless recovery.
