"""Create the code-drawn application mark; Pillow is used only for asset authoring."""
from pathlib import Path
from PIL import Image,ImageDraw,PngImagePlugin

root=Path(__file__).resolve().parents[1]
out=root/'assets/gui'
out.mkdir(parents=True,exist_ok=True)
size=1536
im=Image.new('RGB',(size,size),(10,15,24))
d=ImageDraw.Draw(im)
for r in range(680,50,-1):
    k=(680-r)/630
    color=(int(10+8*k),int(15+17*k),int(24+22*k))
    d.ellipse((768-r,768-r,768+r,768+r),fill=color)
d.ellipse((295,295,1241,1241),outline=(43,65,82),width=12)
d.ellipse((372,372,1164,1164),outline=(29,49,65),width=7)
d.ellipse((612,612,924,924),outline=(91,222,200),width=19)
d.ellipse((711,711,825,825),outline=(155,179,193),width=9)
d.rounded_rectangle((632,268,904,289),radius=9,fill=(91,222,200))
d.rounded_rectangle((258,1221,1278,1278),radius=23,fill=(91,222,200))
metadata=PngImagePlugin.PngInfo()
metadata.add_text('Title','PS4 Disc Ripper')
metadata.add_text('Author','iTZiLiYA')
im.resize((512,512),Image.Resampling.LANCZOS).save(out/'icon0.png',pnginfo=metadata)
print(out/'icon0.png')
