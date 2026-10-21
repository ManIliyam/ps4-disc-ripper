"""Ensure every packaged file's parent is present in the GP4 directory tree."""
from pathlib import Path, PurePosixPath
import sys
import xml.etree.ElementTree as ET

path=Path(sys.argv[1])
tree=ET.parse(path)
root=tree.getroot()
directory=root.find('rootdir')
if directory is None: directory=ET.SubElement(root,'rootdir')
directory.clear()
nodes={():directory}
files=root.find('files')
if files is None: raise SystemExit('GP4 has no files')
for item in files.findall('file'):
    target=PurePosixPath(item.attrib['targ_path'])
    if target.is_absolute() or '..' in target.parts or '\\' in str(target):
        raise SystemExit(f'Unsafe GP4 target: {target}')
    parent=()
    for part in target.parts[:-1]:
        child=parent+(part,)
        if child not in nodes:nodes[child]=ET.SubElement(nodes[parent],'dir',{'targ_name':part})
        parent=child
ET.indent(tree,space='  ')
tree.write(path,encoding='utf-8',xml_declaration=True)
print(f'GP4 parent directories verified: {len(nodes)-1}')
