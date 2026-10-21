# Homebrew Store submission material

Use after the public GitHub prerelease URL is available. Store inclusion requires
the maintainers' process/acceptance; a GitHub release does not automatically add
the application to Homebrew Store. No submission has been sent.

Official PKG-Zone site: https://pkg-zone.com/
It currently links support Discord: https://discord.gg/CAz8aHBp7B
Ask maintainers for the current app-submission process; no self-service upload
procedure has been verified in this project.

## Listing fields

- Name: PS4 Disc Ripper
- Author: iTZiLiYA
- Version: 1.15 / experimental early access
- Platform: PS4; developer-tested firmware 11.00 / GoldHEN 2.4b18.10
- Category suggestion: Utility / Media (maintainers decide)
- Title ID: BREW04117 (development ID; request collision review)
- License: combined GUI GPL-3.0; MIT project/PicoSHA2 notices and font OFL retained
- Icon: icon0.png, 512 x 512
- Binary: PS4-Disc-Ripper-1.15.pkg
- Binary SHA-256: 375e56cd2f180661f094bab499e497fb2f087e5772ede44a9fc9599f104871b1
- Repository, release/source URLs: use the actual published GitHub URLs

## Description

Native controller-friendly app by iTZiLiYA for copying individual accessible DVD
files to the PS4 internal HDD through GoldHEN local FTP. Preserves original bytes
and relative paths, displays copy/readback progress, and verifies saved files with
SHA-256. Retrieve output from /data/MediaRipper/ using WinSCP or FileZilla.

Requires GoldHEN FTP enabled on port 2121. Automatic space queries are denied on
the tested console; users explicitly confirm free space in PS4 Settings before
each copy. Developer-reported tests cover one DVD's IFO and roughly 1 GiB VOB,
including Windows playback. Whole-disc/folder copy, USB export, CD-DA, previews
and DRM decryption are absent; Blu-ray and damaged media are unvalidated.

Provide the release URL, source ZIP, checksum, icon and screenshots of the app.
Do not claim store acceptance or broad optical compatibility before verification.
