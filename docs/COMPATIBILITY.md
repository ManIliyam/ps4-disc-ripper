# Compatibility — v1.15

Evidence is from developer-reported tests and pasted logs/screenshots. Reported
platform: firmware 11.00, GoldHEN 2.4b18.10, FTP banner v2.2. Console CUH model,
DVD brand, recorder and filesystem revision were not recorded.

| Capability | Result | Scope |
|---|---|---|
| GUI, fonts, controller navigation | Working, developer reported | One console |
| Direct packaged-app /mnt/disc access | ENOENT | App namespace |
| Loopback FTP listing/reading | Working | Test DVD VIDEO_TS |
| 8192-byte IFO copy/readback | Passed | Hash matches earlier Windows reference |
| Roughly 1 GiB VOB copy/readback | Passed | App reports matching SHA-256 |
| VOB transfer to Windows and playback | Working, developer reported | Windows copy hash not separately recorded |
| Native fstatfs space query | Denied: EPERM | Destination open/write works |
| GoldHEN FTP AVBL space command | 502 Not implemented | Direct read-only capability probe |
| Manual space confirmation | Working in successful test | Does not measure/reserve free space |
| Cancellation, corruption, cleanup faults | Host tests pass | Console fault tests pending |
| DVD_RTAV/VRO, Blu-ray, other filesystem revisions | Unvalidated | No general support claim |
| CD-DA, unfinalized recovery, raw sectors | Not implemented | No verified reading interface |
| DRM decryption | Not implemented | CSS/AACS/BD+/CPRM outside scope |

Readback compares HDD bytes with the received FTP stream, not media health.
EOF and positive FTP completion are also required. Damaged-disc and disc-removal
behavior remain untested on hardware. Begin with a readable personal test disc.

Historical diagnostic documents describe earlier stages. This table and current
README supersede their old statements that no console tests had taken place.
