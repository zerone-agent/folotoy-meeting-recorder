"""Guard the UI text against missing generated glyphs and untranslated toasts."""
from pathlib import Path
import re
import unittest

root = Path(__file__).resolve().parents[2]
ui = (root / 'main/sc_ui.c').read_text(encoding='utf-8')
status = (root / 'main/sc_ui_text.c').read_text(encoding='utf-8')
core = (root / 'main/sc_core.c').read_text(encoding='utf-8')
font = (root / 'assets/fonts/meeting_font_16.c').read_text(encoding='utf-8')

class ChineseUi(unittest.TestCase):
    def test_all_display_characters_exist_in_font(self):
        needed = {ord(c) for c in ui + status if ord(c) > 127}
        available = {int(code, 16) for code in re.findall(r'/\* U\+([0-9A-Fa-f]+)', font)}
        self.assertEqual(set(), needed - available, 'UI contains glyphs missing from the font')
        self.assertTrue(set(range(32, 127)).issubset(available))
    def test_status_messages_have_translations(self):
        translated = set(re.findall(r'\{"([^"\\]+)",\s*"[^"\\]+"\}', status))
        calls = re.findall(r'\b(?:toast|toast_for|wifi_end|bind_cancel)\((.*?)\)', core, re.S)
        emitted = set()
        for call in calls:
            emitted.update(re.findall(r'"([^"\\]+)"', call))
        self.assertEqual(set(), emitted - translated, 'Status messages lack translations')

if __name__ == '__main__': unittest.main()
