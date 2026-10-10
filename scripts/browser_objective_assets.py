"""Retail objective assets named dynamically by the multiplayer scripts."""
def objective_asset_roots(asset_index):
    prefixes = (
        'xmodel/prop_flag_', 'xmodel/military_german_fieldradio_',
        'materials/compass_flag_', 'materials/hud_flagflash_',
        'materials/objpoint_flagpatch1_', 'materials/objpoint_flagmissing_',
        'materials/objective_',
    )
    return tuple(name for name in asset_index if name.startswith(prefixes))
