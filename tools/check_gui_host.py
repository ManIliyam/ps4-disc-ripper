"""Run the host UI model checks and trace every screen; no PS4 hardware simulated."""
from pathlib import Path
import subprocess

root=Path(__file__).resolve().parents[1]
folder=root/'build/gui-preview'
folder.mkdir(parents=True,exist_ok=True)
names=['overview','browser','saved-test','ftp','about','video-details','working','unavailable','copy-progress','copy-readback','copy-complete','copy-space-unknown','copy-space-confirmation','copy-manual-progress']
for index,name in enumerate(names):
    subprocess.run([str(root/'build/gui-host-test'),str(index),str(folder/f'{name}.jsonl')],check=True)
