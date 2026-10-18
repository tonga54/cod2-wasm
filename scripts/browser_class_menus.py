"""First-party Create a Class menus and their entries in the retail menu list."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MENU_FILES = {
    'ui_mp/create_a_class.menu': ROOT / 'downstream/ui/create_a_class.menu',
    'ui_mp/scriptmenus/browser_class.menu': ROOT / 'downstream/ui/browser_class.menu',
}
MAIN_MENU_LINE = b'loadMenu { "ui_mp/main.menu" }'
CLASS_MENU_LINE = b'\tloadMenu { "ui_mp/create_a_class.menu" }'


def class_menus():
    return {name: path.read_bytes() for name, path in MENU_FILES.items()}


def add_class_menu(menus_txt):
    """Load the Create a Class menu with the main menu's."""
    lines = menus_txt.splitlines()
    matches = [index for index, line in enumerate(lines) if line.strip() == MAIN_MENU_LINE]
    if len(matches) != 1:
        raise ValueError('Unexpected multiplayer menu list')
    lines.insert(matches[0] + 1, CLASS_MENU_LINE)
    return b'\n'.join(lines) + b'\n'
