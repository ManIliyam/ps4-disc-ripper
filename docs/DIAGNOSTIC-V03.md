# Diagnostic v0.3 / package 1.02

User observation: v0.2 package version 1.01 installed; after fifteen seconds
the screen remained black. FTP still listed only the original v0.1 log.
This does not establish completion or confirm a shutdown-crash fix.

The directory-setup and log-open failure paths previously slept silently.
Version 0.3 adds an exclusive bootstrap log directly under the already tested
`/data` location: `/data/MediaRipper-startup-<timestamp>-<counter>.log`.
It logs directory and normal-log setup before starting optical checks.
Existing-directory validation now uses a read-only O_DIRECTORY/O_NOFOLLOW
descriptor instead of depending on libc's struct stat layout. A stat-layout
issue is a hypothesis, not an established explanation for the missing log.

If bootstrap log creation itself fails, no file can record that failure and
the application still sleeps. A visible error/status UI remains outstanding.
The native filesystem probes are unchanged. No sandbox escape is implemented.
Build using --output diagnostic-v03; installable package metadata is version
1.02. Compilation, packaging and hardware tests of this revision are pending.
