# Rebuilds Pocket-Bomberman/images/explosion.png from the original sprite sheet.
# Usage (from the repository root, needs Pillow):
#   python resource/build_explosion.py resource/Game_Boy___GBC_-_Pocket_Bomberman_-_Characters_-_Overworld_Color.gif Pocket-Bomberman/images/explosion.png
# Output: 16x16 cells, columns = piece (CENTER, HORIZONTAL, VERTICAL, END_LEFT,
# END_RIGHT, END_UP, END_DOWN), rows = phase (0 small, 1 medium, 2 big).
import sys
from PIL import Image
src = Image.open(sys.argv[1]).convert('RGBA')
out = Image.new('RGBA', (128, 64), (0, 0, 0, 0))

def piece(x, y, w, h):
    return src.crop((x, y, x + w, y + h))

CENTER_X = [47, 65, 83]          # 16x16 centers, rows 89-104 (also x of vertical pieces)
H_SEG_X = [17, 27, 37]           # 8x16 horizontal segments, rows 71-86
LEFT_CAP_X = [17, 27, 37]        # 8x16 left ends, rows 89-104
RIGHT_CAP_X = [101, 111, 121]    # 8x16 right ends, rows 89-104
TOP_CAP_Y, V_SEG_Y, CENTER_Y, BOTTOM_CAP_Y = 69, 79, 89, 107

for p in range(3):
    cx = CENTER_X[p]
    hseg = piece(H_SEG_X[p], 71, 8, 16)
    vseg = piece(cx, V_SEG_Y, 16, 8)
    cells = [
        [(piece(cx, CENTER_Y, 16, 16), (0, 0))],
        [(hseg, (0, 0)), (hseg, (8, 0))],
        [(vseg, (0, 0)), (vseg, (0, 8))],
        [(piece(LEFT_CAP_X[p], CENTER_Y, 8, 16), (0, 0)), (hseg, (8, 0))],
        [(hseg, (0, 0)), (piece(RIGHT_CAP_X[p], CENTER_Y, 8, 16), (8, 0))],
        [(piece(cx, TOP_CAP_Y, 16, 8), (0, 0)), (vseg, (0, 8))],
        [(vseg, (0, 0)), (piece(cx, BOTTOM_CAP_Y, 16, 8), (0, 8))],
    ]
    for col, parts in enumerate(cells):
        for img, (dx, dy) in parts:
            out.alpha_composite(img, (col * 16 + dx, p * 16 + dy))
out.save(sys.argv[2])
