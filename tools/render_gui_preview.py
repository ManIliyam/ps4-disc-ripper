"""Render host traces of the actual C++ UI drawing code. Pillow is preview-only."""
import json
import sys
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

root=Path(__file__).resolve().parents[1]
folder=root/'build/gui-preview'
folder.mkdir(parents=True,exist_ok=True)
font_path=root/'build/gui-v01/assets/fonts/Gontserrat-Regular.ttf'
fonts={}
for path in sorted(folder.glob('*.jsonl')):
    picture=Image.new('RGB',(1920,1080))
    draw=ImageDraw.Draw(picture)
    text_bounds=[]
    for raw in path.read_text().splitlines():
        op=json.loads(raw)
        if op[0]=='fill':draw.rectangle((0,0,1919,1079),fill=tuple(op[1]))
        elif op[0]=='rect':draw.rectangle((op[1],op[2],op[1]+op[3]-1,op[2]+op[4]-1),fill=tuple(op[5]))
        elif op[0]=='pixel':picture.putpixel((op[1],op[2]),tuple(op[3]))
        elif op[0]=='text':
            _,value,size,x,y,color=op
            if size not in fonts:fonts[size]=ImageFont.truetype(str(font_path),size)
            font=fonts[size]
            bounds=draw.textbbox((x,y),value,font=font,anchor='ls')
            if bounds[2]>1850 or bounds[3]>1050: text_bounds.append((value,bounds))
            draw.text((x,y),value,font=font,fill=tuple(color),anchor='ls')
    picture.save(path.with_suffix('.png'))
    print(path.stem, 'text bounds:',text_bounds or 'OK')
