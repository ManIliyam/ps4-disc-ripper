"""Assemble a publishable source tree/assets without rebuilding the tested PKG."""
import hashlib
import json
from pathlib import Path
import shutil
import zipfile

root=Path(__file__).resolve().parents[1]
version='1.15'
expected='375e56cd2f180661f094bab499e497fb2f087e5772ede44a9fc9599f104871b1'
stage=root/'build/gui-v06'
out=root/'release/v1.15'
if out.exists(): raise SystemExit(f'Refusing to overwrite release kit: {out}')
package=root/'dist/PS4-Disc-Ripper-1.15.pkg'
if hashlib.sha256(package.read_bytes()).hexdigest()!=expected:
    raise SystemExit('PKG differs from the hardware-tested version; stop release.')
manifest=json.loads((stage/'build-inputs.json').read_text())
for folder in ['diagnostic','ui','extraction','storage']:
    for file in (root/'src'/folder).glob('*'):
        if file.suffix not in ('.h','.cpp'): continue
        digest=hashlib.sha256(file.read_bytes()).hexdigest()
        if manifest['files'].get(file.name)!=digest:
            raise SystemExit(f'Runtime source differs from tested build: {file}')
checks=[line for line in (root/'dist/GUI-1.15-pkg_validate.txt').read_text().splitlines() if line.strip()]
if not checks or not all(line.startswith('[OK]') for line in checks):
    raise SystemExit('Original PKG validation log does not pass')

repo=out/'repository';repo.mkdir(parents=True)
# Deliberate allowlist: never include this workspace's unrelated projects,
# histories, user attachments, credentials, logs or WSL/build directories.
for folder in ['src','tools','tests','docs','assets']:
    shutil.copytree(root/folder,repo/folder,ignore=shutil.ignore_patterns('__pycache__','*.pyc'))
for name in ['LICENSE','COPYING','README.md','.gitignore']:shutil.copyfile(root/name,repo/name)
third=repo/'third_party/openorbis';third.mkdir(parents=True)
for name in ['graphics.cpp','graphics.h','log.h']:shutil.copyfile(stage/'hello_world'/name,third/name)
fonts=repo/'third_party/fonts';fonts.mkdir(parents=True)
for name in ['Gontserrat-Regular.ttf','OFL.txt']:shutil.copyfile(stage/'assets/fonts'/name,fonts/name)
manifest['sdk']='Installed OpenOrbis toolchain; see docs/BUILD.md for archive SHA-256'
(repo/'build-inputs.json').write_text(json.dumps(manifest,indent=2)+'\n')
assets=out/'assets';assets.mkdir()
shutil.copyfile(package,assets/package.name)
shutil.copyfile(root/'COPYING',assets/'OpenOrbis-GPL-3.0.txt')
shutil.copyfile(root/'assets/gui/icon0.png',out/'icon0.png')
shutil.copyfile(root/'docs/RELEASE-v1.15.md',out/'RELEASE-NOTES.md')
shutil.copyfile(root/'docs/STORE-SUBMISSION.md',out/'STORE-SUBMISSION.md')
source=assets/'PS4-Disc-Ripper-1.15-source.zip'
with zipfile.ZipFile(source,'w',zipfile.ZIP_DEFLATED) as archive:
    for file in sorted(repo.rglob('*')):
        if file.is_file():archive.write(file,file.relative_to(repo))
sums=[]
for file in sorted(assets.iterdir()):
    sums.append(hashlib.sha256(file.read_bytes()).hexdigest()+'  '+file.name)
(assets/'SHA256SUMS.txt').write_text('\n'.join(sums)+'\n')
record={'version':version,'author':'iTZiLiYA','intended_repository':'ManIliyam/ps4-disc-ripper',
        'published':False,'prerelease':True,'pkg_sha256':expected,'pkg_bytes':package.stat().st_size,
        'evidence':'Developer-reported DVD IFO and approximately 1 GiB VOB copy/readback; VOB Windows playback confirmed by developer',
        'limits':'One-file copy; manual space check; broader media/firmware and console fault tests unvalidated',
        'validation_checks':len(checks)}
(out/'RELEASE-CHECKS.json').write_text(json.dumps(record,indent=2)+'\n')
print(f'Release kit prepared: {out}\nPKG unchanged; runtime source matches build; {len(checks)} package checks passed.')
