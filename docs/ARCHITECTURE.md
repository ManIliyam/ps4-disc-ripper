# Architecture and implementation gates

Only `src/diagnostic/main.cpp` exists as runtime implementation today.
The following is the intended application architecture, not implemented modules.

- `disc/DiscReader`: capability query, inventory and read-only file-stream interface.
  `MountedDiscReader` is the first backend. Report unknown metadata explicitly.
- `DiscDetector`: observations and confidence; keep physical medium, filesystem,
  directory layout and DRM status separate. DVDReader and BlurayReader classify
  structures; they do not assume decryption. CDDAReader initially returns unsupported.
- `ExtractionManager`: one worker, state machine (idle/scanning/copying/verifying/
  cancelling/complete/failed), immutable progress snapshots and cancellation token.
  UI thread must never perform optical I/O. Cancellation acts between calls; an
  OS-blocked optical read cannot be assumed interruptible.
- `FileCopier`: bounded buffers, short-read/write handling, limited retries,
  per-file temporary output, final size and optional independent SHA-256 verification.
  Fail explicitly rather than silently padding damaged reads. Resume requires a
  validated manifest and source identity; filenames alone are insufficient.
- `StorageManager`: exclusive unique archive directories under `/data/MediaRipper`,
  overflow-safe space calculations and margin; space can still run out after checking.
  Preserve relative names safely and reject path traversal and symlinks.
- `Logger`: append progress/errors to the archive; failed log writes are visible.
- `FTPInfo`: display configured IP/port/path; rely on GoldHEN server. Do not claim
  that enabling FTP grants the app optical access. No new FTP server initially.
- `USBExporter`: later copy from verified archives with destination checks, large
  file support and the same safe copy engine. Discover mounts, do not guess USB names.
- `ui/MainMenu`, `DiscBrowser`, `ProgressScreen`: controller-friendly renderer,
  disabled unsupported actions, readable errors and paths, worker-driven progress.

Gate A: compile and install diagnostic; complete no-disc/finalized DVD/BD controls.
Gate B: confirm file reads in the app, then implement mounted DVD file copying.
Validate complete hashes against a PC copy and playback of personal VOB/VRO files.
Gate C: document FTP archive retrieval and recheck downloaded hashes.
Gate D: extend to proven BD layouts/UDF revisions and USB exports.
Gate E: only investigate CD-DA after model-specific TOC and audio-sector reading
are demonstrated on 11.00. WAV requires 44.1 kHz, signed 16-bit stereo PCM with
verified byte order, track boundaries and error handling. FLAC is a later licensed
dependency; MP3 is deferred. No placeholder encoder or invented optical API.

Raw device recovery, unfinalized/multisession recovery and direct disc FTP streaming
remain separate research projects. They cannot be unlocked merely by adding a menu.
