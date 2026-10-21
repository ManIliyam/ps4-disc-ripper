"""Build, validate and export the GUI with notices and corresponding source."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import zipfile

parser=argparse.ArgumentParser()
parser.add_argument('--pkg-tool',type=Path,required=True,help='Built .NET 8 PkgTool.Core.dll')
args=parser.parse_args()
root=Path(__file__).resolve().parents[1]
stage=root/'build/gui-v06'
dist=root/'dist'
dist.mkdir(exist_ok=True)
env=dict(os.environ,DOTNET_SYSTEM_GLOBALIZATION_INVARIANT='1')
package='IV0000-BREW04117_00-DISCRIPPERDIAG01.pkg'
for verb,inputs in [('pkg_build',['pkg.gp4','.']),('pkg_validate',[package])]:
    run=subprocess.run(['dotnet',str(args.pkg_tool.resolve()),verb,*inputs],cwd=stage,env=env,capture_output=True,text=True)
    (dist/f'GUI-1.15-{verb}.txt').write_text(run.stdout+run.stderr)
    if run.returncode: raise SystemExit(f'{verb} failed. See dist/GUI-1.15-{verb}.txt')
    if verb=='pkg_validate':
        checks=[line for line in run.stdout.splitlines() if line.strip()]
        if not checks or not all(line.startswith('[OK]') for line in checks):
            raise SystemExit('Package validator did not report all checks OK; inspect validation log.')
    print(verb,'passed')
target=dist/'PS4-Disc-Ripper-1.15.pkg'
shutil.copyfile(stage/package,target)
digest=hashlib.sha256(target.read_bytes()).hexdigest()
(dist/'PS4-Disc-Ripper-1.15.sha256').write_text(digest+'  '+target.name+'\n')
git=subprocess.run(['git','-C',str(args.pkg_tool.resolve().parents[4]),'rev-parse','HEAD'],capture_output=True,text=True)
manifest={'version':'1.15','author':'iTZiLiYA','sha256':digest,'bytes':target.stat().st_size,
          'status':'Compiled and package-validated; native space query denied on console; manual per-file storage confirmation and streaming need console testing',
          'pkg_tool_commit':git.stdout.strip() if git.returncode==0 else 'unavailable'}
(dist/'PS4-Disc-Ripper-1.15.json').write_text(json.dumps(manifest,indent=2)+'\n')
with zipfile.ZipFile(dist/'PS4-Disc-Ripper-1.15-source.zip','w',zipfile.ZIP_DEFLATED) as archive:
    for folder in ['src','tools','tests','docs','assets']:
        for file in sorted((root/folder).rglob('*')):
            if file.is_file() and '__pycache__' not in file.parts: archive.write(file,file.relative_to(root))
    for name in ['LICENSE','COPYING','README.md','.gitignore']:archive.write(root/name,name)
    archive.write(stage/'build-inputs.json','build-inputs.json')
    for name in ['graphics.cpp','graphics.h','log.h']:
        archive.write(stage/'hello_world'/name,'third_party/openorbis/'+name)
    archive.write(stage/'assets/credits/OpenOrbis-GPL-3.0.txt','licenses/OpenOrbis-GPL-3.0.txt')
    for name in ['Gontserrat-Regular.ttf','OFL.txt']:
        archive.write(stage/'assets/fonts'/name,'third_party/fonts/'+name)
shutil.copyfile(stage/'assets/credits/OpenOrbis-GPL-3.0.txt',dist/'OpenOrbis-GPL-3.0.txt')
print(json.dumps(manifest))
