# PS4 Disc Ripper v1.15 — Experimental Early Access

Created by **iTZiLiYA**.

Copy individual accessible DVD files to the PS4 internal HDD and transfer them to
Windows through GoldHEN FTP, preserving original bytes and the selected file path.

This first public prerelease includes a controller UI, browsing, background
one-file streaming, copy/readback progress, speed and phase ETA, cancellation
requests, unique folders and SHA-256 verification of the saved file.

The developer reports testing firmware 11.00 with GoldHEN 2.4b18.10: one DVD's
8192-byte IFO and roughly 1 GiB VOB were copied and verified; the VOB was transferred
to Windows and played successfully. Other media/consoles/firmware are unvalidated.

**Keep GoldHEN FTP enabled on port 2121, even for internal copying.** Automatic
space queries are denied on the tested setup. When prompted, inspect PS4 Settings
> Storage and confirm enough space for the selected file plus the displayed
reserve. Free space remains labeled Unknown; the reserve is not enforced.

Start with an IFO, then one VOB. Wait for **File saved and verified** and retrieve
the displayed file under `/data/MediaRipper/` through FTP.

No whole-disc/folder copy, USB export, previews, desktop receiver/control, CD-DA or
DRM decryption. Blu-ray, damaged media and unfinalized/multisession discs are
unvalidated. One VOB is not a complete DVD archive. OS calls can delay cancellation.

Publish tag `v1.15` as a **pre-release**. Assets:

- `PS4-Disc-Ripper-1.15.pkg` — installer, not a media archive.
- `PS4-Disc-Ripper-1.15-source.zip` — corresponding source and notices.
- `SHA256SUMS.txt` — asset integrity hashes.
- `OpenOrbis-GPL-3.0.txt` — combined-GUI license text.

The tested PKG is 6,619,136 bytes, SHA-256:

```text
375e56cd2f180661f094bab499e497fb2f087e5772ede44a9fc9599f104871b1
```

Experimental title ID `BREW04117`: check for an existing unrelated installation
using that ID. Read README/compatibility notes before installing.
