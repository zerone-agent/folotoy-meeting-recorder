#!/usr/bin/env python3
"""Generate the recorder UI font from a verified Noto Sans CJK SC font."""
import argparse
import hashlib
from pathlib import Path
import re
import subprocess

root = Path(__file__).resolve().parents[1]
EXPECTED = '2c76254f6fc379fddfce0a7e84fb5385bb135d3e399294f6eeb6680d0365b74b'

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--font', required=True, type=Path)
    parser.add_argument('--converter', required=True, type=Path)
    args = parser.parse_args()
    if hashlib.sha256(args.font.read_bytes()).hexdigest() != EXPECTED:
        raise SystemExit('Unexpected font source hash')
    version = subprocess.check_output([str(args.converter), '--version'], text=True).strip()
    if version != '1.5.3':
        raise SystemExit('lv_font_conv 1.5.3 required; got ' + version)
    text = ''.join((root / name).read_text(encoding='utf-8') for name in ('main/sc_ui.c', 'main/sc_ui_text.c'))
    characters = sorted({c for c in text if ord(c) > 127})
    ranges = '0x20-0x7E,' + ','.join(f'0x{ord(c):X}' for c in characters)
    from fontTools.ttLib import TTFont
    font = TTFont(args.font)
    supported = set().union(*(set(table.cmap) for table in font['cmap'].tables if table.isUnicode()))
    missing = [c for c in characters if ord(c) not in supported]
    if missing:
        raise SystemExit('Font lacks required glyphs: ' + ''.join(missing))
    output = root / 'assets/fonts/meeting_font_16.c'
    subprocess.run([str(args.converter), '--font', str(args.font), '--range', ranges,
                    '--size', '16', '--bpp', '4', '--format', 'lvgl', '--no-compress',
                    '--lv-font-name', 'meeting_font_16', '--lv-include', 'lvgl.h',
                    '--output', str(output)], check=True)
    generated = (output.read_text(encoding='utf-8')
                 .replace(str(args.font), 'NotoSansCJKsc-Regular.otf')
                 .replace(str(output), 'assets/fonts/meeting_font_16.c'))
    output.write_text(generated.rstrip() + '\n', encoding='utf-8')
    (root / 'assets/fonts/meeting_font_chars.txt').write_text(''.join(characters) + '\n', encoding='utf-8')
    print(f'Generated 16px font for {len(characters)} non-ASCII characters')

if __name__ == '__main__':
    main()
