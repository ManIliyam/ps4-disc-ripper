"""Stage a native PS4 GUI using the installed OpenOrbis graphics/font sample assets."""
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
from fix_font_renderer import fix_font_renderer

root = Path(__file__).resolve().parents[1]
sdk = Path(os.environ['OO_PS4_TOOLCHAIN']).resolve()
sample = sdk / 'samples/hello_world'
out = root / 'build/gui-v06'
if out.exists():
    raise SystemExit(f'Refusing to overwrite {out}. Keep this build or use a fresh workspace.')
required = [sample/'Makefile', sample/'sce_sys/icon0.png', sample/'sce_sys/about/right.sprx',
            sdk/'samples/_common/graphics.cpp', sdk/'samples/_common/graphics.h', sdk/'samples/_common/log.h',
            sdk/'samples/font/assets/fonts/Gontserrat-Regular.ttf', sdk/'samples/font/assets/fonts/OFL.txt',sdk/'LICENSE']
for path in required:
    if not path.is_file(): raise SystemExit(f'Missing installed SDK asset: {path}')
makefile = (sample/'Makefile').read_text()
updates = {'TITLE':'PS4 Disc Ripper', 'VERSION':'1.15', 'TITLE_ID':'BREW04117',
           'CONTENT_ID':'IV0000-BREW04117_00-DISCRIPPERDIAG01', 'PROJDIR':'hello_world',
           'LIBS':'-lc -lkernel -lc++ -lSceNet -lSceNetCtl -lSceVideoOut -lScePad -lSceUserService -lSceSysmodule -lSceFreeType'}
for key,value in updates.items():
    makefile,count=re.subn(r'^'+key+r'\s*:=.*$',key+' := '+value,makefile,flags=re.MULTILINE)
    if count!=1: raise SystemExit(f'Upstream Makefile changed: inspect {key}')
makefile='EXTRAFLAGS := -std=c++17 -D_GNU_SOURCE -DGRAPHICS_USES_FONT -DMEDIA_RIPPER_GUI\n'+makefile
makefile+='\n$(OBJS): $(wildcard $(PROJDIR)/*.h)\n'
if '--files "$^"' not in makefile: raise SystemExit('Upstream GP4 rule changed')
makefile=makefile.replace('--files "$^"','--files "$^"\n\tpython3 fix_gp4.py $@')
# Include all font/license assets while retaining the upstream package recipe.
makefile,count=re.subn(r'^ASSETS\s*:=.*$', 'ASSETS := $(shell find assets -type f)',makefile,flags=re.MULTILINE)
if count!=1: raise SystemExit('Upstream ASSETS changed')
shutil.copytree(sample,out)
(out/'Makefile').write_text(makefile)
shutil.copyfile(root/'tools/fix_gp4.py',out/'fix_gp4.py')
source=out/'hello_world'
for path in (root/'src/diagnostic').glob('*'):
    if path.suffix in ['.cpp','.h']: shutil.copyfile(path,source/path.name)
for path in (root/'src/ui').glob('*.h'): shutil.copyfile(path,source/path.name)
for folder in ['extraction','storage']:
    for path in (root/'src'/folder).glob('*.h'): shutil.copyfile(path,source/path.name)
for name in ['graphics.cpp','graphics.h','log.h']: shutil.copyfile(sdk/'samples/_common'/name,source/name)
(source/'graphics.cpp').write_text(fix_font_renderer((source/'graphics.cpp').read_text()))
font=out/'assets/fonts'
font.mkdir(parents=True,exist_ok=True)
for name in ['Gontserrat-Regular.ttf','OFL.txt']: shutil.copyfile(sdk/'samples/font/assets/fonts'/name,font/name)
credits=out/'assets/credits'
credits.mkdir(parents=True,exist_ok=True)
(credits/'author.txt').write_text('PS4 Disc Ripper\nCreated by iTZiLiYA\nExperimental selected-file copy 1.15\n')
shutil.copyfile(root/'src/extraction/PicoSHA2-LICENSE',credits/'PicoSHA2-MIT.txt')
shutil.copyfile(sdk/'LICENSE',credits/'OpenOrbis-GPL-3.0.txt')
icon=root/'assets/gui/icon0.png'
if icon.is_file(): shutil.copyfile(icon,out/'sce_sys/icon0.png')
manifest={'version':'1.15','author':'iTZiLiYA','sdk':str(sdk),'files':{}}
for path in sorted(source.glob('*')):
    if path.is_file(): manifest['files'][path.name]=hashlib.sha256(path.read_bytes()).hexdigest()
(out/'build-inputs.json').write_text(json.dumps(manifest,indent=2)+'\n')
print(f'Staged native GUI in {out}\nRun: make -C {out} pkg.gp4')
