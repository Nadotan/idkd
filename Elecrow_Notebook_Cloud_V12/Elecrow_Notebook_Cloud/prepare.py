#!/usr/bin/env python3
"""Prepare the official V1.2 Elecrow Lesson09 example as the notebook sketch."""
from pathlib import Path
import re, shutil
root=Path(__file__).parent.resolve()
source=root/'vendor/example/V1.2/Arduino_Code/Lesson09-LVGL_Lighting_Control'
assert source.exists(), 'Official Elecrow source missing. Run in GitHub Actions.'
target=root/'Sketch/Sketch'
target.mkdir(parents=True,exist_ok=True)
for f in source.iterdir():
    if f.is_file():shutil.copy2(f,target/f.name)
ino=target/'Lesson09-LVGL_Lighting_Control.ino'
s=ino.read_text()
start=s.index('/* Button callback function - turn on LED */')
end=s.index('/*---------------------------------------------------------------\n * Arduino entry points',start)
s=s[:start]+s[end:]
s=s.replace('#include "lvgl_port.h"','#include "lvgl_port.h"\n#include "NotebookUI.h"')
s=s.replace('    create_led_control_ui();','    Notebook::start();')
(target/'Sketch.ino').write_text(s)
ino.unlink()
shutil.copy2(root/'NotebookUI.h',target/'NotebookUI.h')
print('Prepared',target)
