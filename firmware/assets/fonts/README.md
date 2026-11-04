<p align="right"><a href="README.zh_CN.md">简体中文</a> · <strong>English</strong></p>

# Meeting recorder font

`meeting_font_16.c` is an uncompressed 4-bit, 16 px LVGL subset of [Noto Sans CJK SC Regular](https://github.com/notofonts/noto-cjk/blob/main/Sans/OTF/SimplifiedChinese/NotoSansCJKsc-Regular.otf), version 2.004, under the [SIL Open Font License 1.1](OFL-Noto-CJK.txt). The input OTF has SHA-256 `2c76254f6fc379fddfce0a7e84fb5385bb135d3e399294f6eeb6680d0365b74b`.

It contains printable ASCII and the Chinese characters listed in `meeting_font_chars.txt`. It covers the recorder's own UI strings, not arbitrary names or Wi-Fi SSIDs. To regenerate it, install Python `fonttools` and `lv_font_conv` 1.5.3 outside the checkout, then run:

```sh
python3 tools/generate_meeting_font.py --font /path/to/NotoSansCJKsc-Regular.otf --converter /path/to/lv_font_conv
```

The generator checks the source hash and converter version, checks each selected glyph and derives the character set from the UI sources. The app compiles the generated source directly. `tests/meeting/test_ui_chinese.py` checks UI coverage and status message translations.
