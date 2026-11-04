<p align="right"><strong>简体中文</strong> · <a href="README.md">English</a></p>

# 会议录音界面字体

`meeting_font_16.c` 是 [Noto Sans CJK SC Regular](https://github.com/notofonts/noto-cjk/blob/main/Sans/OTF/SimplifiedChinese/NotoSansCJKsc-Regular.otf) 2.004 版本生成的 16 像素、4 位、未压缩 LVGL 子集，遵循 [SIL Open Font License 1.1](OFL-Noto-CJK.txt)。源 OTF 的 SHA-256 为 `2c76254f6fc379fddfce0a7e84fb5385bb135d3e399294f6eeb6680d0365b74b`。

字库包括可打印 ASCII 和 `meeting_font_chars.txt` 中列出的汉字，覆盖录音界面的固定文案，不保证覆盖任意姓名或 Wi-Fi 名称。重新生成时，在仓库之外安装 Python `fonttools` 和 `lv_font_conv` 1.5.3，然后运行：

```sh
python3 tools/generate_meeting_font.py --font /path/to/NotoSansCJKsc-Regular.otf --converter /path/to/lv_font_conv
```

生成器会核对源字体摘要和转换器版本、检查字形，并从界面源码提取字符；应用直接编译生成的源码。`tests/meeting/test_ui_chinese.py` 检查界面字形覆盖及状态提示翻译。
