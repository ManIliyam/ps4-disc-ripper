"""Stage the installed OpenOrbis hello_world sample without modifying the SDK."""
import hashlib
import json
import os
import re
from pathlib import Path
import shutil
import argparse

parser = argparse.ArgumentParser()
parser.add_argument('--output', default='diagnostic', choices=['diagnostic', 'diagnostic-v02', 'diagnostic-v03', 'diagnostic-v04', 'diagnostic-v05', 'diagnostic-v06'])
args = parser.parse_args()

root = Path(__file__).resolve().parents[1]
sdk = Path(os.environ["OO_PS4_TOOLCHAIN"]).resolve()
sample = sdk / "samples" / "hello_world"
required = ["Makefile", "hello_world/main.cpp", "sce_sys/icon0.png", "sce_sys/about/right.sprx"]
for relative in required:
    if not (sample / relative).is_file():
        raise SystemExit(f"Missing upstream sample asset: {sample / relative}")
out = root / "build" / args.output
if out.exists():
    raise SystemExit(f"Refusing to overwrite {out}; choose a fresh checkout for rebuilding.")
makefile = (sample / "Makefile").read_text()
replacements = {
    "OpenOrbis Hello World Sample": "PS4 Disc Ripper Diagnostic",
    "BREW00083": "BREW04117",
    "HELLOWORLD000000": "DISCRIPPERDIAG01",
}
# Both content-ID suffixes are sixteen characters.
for old, new in replacements.items():
    if old not in makefile:
        raise SystemExit(f"Upstream sample changed; review Makefile: missing {old!r}")
    makefile = makefile.replace(old, new)
makefile, count = re.subn(r'^PROJDIR\s*:=.*$', 'PROJDIR := hello_world', makefile, flags=re.MULTILINE)
if count != 1:
    raise SystemExit("Upstream PROJDIR changed; review Makefile before staging.")
makefile, count = re.subn(r'^VERSION\s*:=.*$', 'VERSION := 1.05', makefile, flags=re.MULTILINE)
if count != 1:
    raise SystemExit("Upstream VERSION changed; review Makefile before staging.")
makefile, count = re.subn(r'^LIBS\s*:=.*$', 'LIBS := -lc -lkernel -lc++ -lSceNet', makefile, flags=re.MULTILINE)
if count != 1:
    raise SystemExit("Upstream LIBS changed; review Makefile before staging.")
shutil.copytree(sample, out)
(out / "Makefile").write_text(makefile)
shutil.copyfile(root / "src/diagnostic/main.cpp", out / "hello_world/main.cpp")
shutil.copyfile(root / "src/diagnostic/FtpProbe.h", out / "hello_world/FtpProbe.h")
shutil.copyfile(root / "src/diagnostic/TestArchive.h", out / "hello_world/TestArchive.h")
manifest = {
    "sdk": str(sdk),
    "source_sha256": hashlib.sha256((root / "src/diagnostic/main.cpp").read_bytes()).hexdigest(),
    "ftp_probe_sha256": hashlib.sha256((root / "src/diagnostic/FtpProbe.h").read_bytes()).hexdigest(),
    "test_archive_sha256": hashlib.sha256((root / "src/diagnostic/TestArchive.h").read_bytes()).hexdigest(),
    "upstream_makefile_sha256": hashlib.sha256((sample / "Makefile").read_bytes()).hexdigest(),
    "status": "staged, not compiled",
}
(out / "build-inputs.json").write_text(json.dumps(manifest, indent=2) + "\n")
print(f"Staged {out}\nRun: make -C build/{args.output} EXTRAFLAGS=-std=c++17")
