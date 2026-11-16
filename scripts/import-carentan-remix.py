#!/usr/bin/env python3
"""Convert the owner's Carentan Remix textures for every supported browser map."""
import argparse
from pathlib import Path
from browser_remix_assets import PACK, build_pack, refresh_weapon_finishes, refresh_masonry_finishes, refresh_joinery, refresh_smoke, refresh_fire, refresh_road_bed

root = Path(__file__).resolve().parent.parent
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('source', type=Path)
parser.add_argument('--renderer', type=Path, default=root/'data/browser/main/cod2_browser_renderer.iwd')
parser.add_argument('--output', type=Path, default=root/'data/main'/PACK)
parser.add_argument('--size', type=int, choices=(512, 1024, 2048, 4096), default=2048)
partial = parser.add_mutually_exclusive_group()
partial.add_argument('--weapons-only', action='store_true', help='Refresh weapon finishes in an existing pack, preserving world assets')
partial.add_argument('--masonry-only', action='store_true', help='Refresh preserved stone trim finishes, retaining all other assets')
partial.add_argument('--joinery-only', action='store_true', help='Refresh complete donor doors, preserving other assets')
partial.add_argument('--smoke-only', action='store_true', help='Refresh dedicated grenade smoke shading, preserving alpha and other assets')
partial.add_argument('--fire-only', action='store_true', help='Refresh grenade fire billows, preserving other assets')
partial.add_argument('--road-bed-only', action='store_true', help='Restore stone beneath sparse road puddles, preserving other assets')
args = parser.parse_args()
convert = refresh_road_bed if args.road_bed_only else refresh_fire if args.fire_only else refresh_weapon_finishes if args.weapons_only else refresh_masonry_finishes if args.masonry_only else refresh_joinery if args.joinery_only else refresh_smoke if args.smoke_only else build_pack
report = convert(args.source, args.renderer, args.output, args.size)
print(f"Converted {len(report['materials'])} materials, {len(report['skies'])} skies; {len(report['sources'])} source textures.")
for name, materials in report['maps'].items():
    print(f'{name}: {len(materials)} world materials')
print('Run scripts/prepare-browser-bootstrap.py to integrate the private pack.')
