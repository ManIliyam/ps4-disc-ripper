# Sandbox-access follow-up

User reports GoldHEN version **2.4b18.10** on the target firmware 11.00.
Do not infer that a version-dependent kernel primitive works from this string.

Hardware observations from the user's v0.3 run:

- libc lstat returns ENOSYS (78), so v0.1's existing-directory validation was
  unusable in this environment. Descriptor-based validation succeeds in v0.3.
- Native open succeeds for /, /app0 and /data, but returns 0x80020002 for /mnt,
  /mnt/disc and the known DVD IFO path. The visible root has no mnt entry.
- The same DVD was readable through GoldHEN FTP, including a roughly 1 GiB VOB
  transfer with user-confirmed playback. FTP-context access is proven for that
  fixture; packaged-app direct disc access remains unproven.
- The run reached its final fsync marker. No immediate crash was reported while
  sleeping. This is evidence against repeating the original return-from-main
  pattern, not proof that every shutdown failure is resolved.

Source inspection: [ps4-libjbc](https://github.com/sleirsgoevy/ps4-libjbc)
exposes credential/root helpers, but is not an ordinary userspace file API.
[kernelrw.c](https://github.com/sleirsgoevy/ps4-libjbc/blob/master/kernelrw.c)
uses a kernel-execution primitive and a Mira-related kernel-copy check.
[jailbreak.c](https://github.com/sleirsgoevy/ps4-libjbc/blob/master/jailbreak.c)
traverses kernel objects and writes credentials/root state. Despite the README's
firmware-agnostic description, the implementation contains fixed field offsets;
one comment limits its offset observation to 5.05–9.60. Those observations do
not independently verify firmware 11.00 compatibility or the kernel primitive
available in the user's GoldHEN build. No libjbc code was incorporated or run.

Next evidence needed: exact GoldHEN version and a maintained consumer/source
that demonstrates this mechanism with that release on 11.00. Audit dependency
license and pin a revision before integration. Do not blindly call its helper
functions or invent a syscall to remove the app's sandbox. Consider an app FTP
client backend using the already verified GoldHEN server as an alternative;
console-to-itself connectivity and file-copy semantics still need validation.

Additional source evidence: GoldHEN's public release notes document adding
syscall 11 alongside firmware 11.00 support in 2.4b17. This supports investigating
the primitive, but does not prove libjbc's field offsets or credential/root
mutation code correct on 11.00 with 2.4b18.10. Itemzflow includes a separate
Jailbreak PRX that references libjbc; its existence alone is also not exact-target
validation. No kernel helper is enabled in this application's current build.

References:
- https://github.com/GoldHEN/GoldHEN/releases
- https://github.com/LightningMods/Itemzflow/tree/main/external/Jailbreak%20PRX
