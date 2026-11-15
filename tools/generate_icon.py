"""Vector source for fast-draw's pen and connected diagram nodes."""

import sys
from pathlib import Path

sys.dont_write_bytecode = True
from iconlib import Canvas, generate

ROOT = Path(__file__).resolve().parents[1]


def draw(canvas: Canvas) -> None:
    """A gold drawing nib over a blue connected diagram."""
    dark = (26, 40, 61)
    canvas.rect(18, 6, 64, 88, dark)
    canvas.rect(6, 18, 88, 64, dark)
    for x, y in ((18, 18), (82, 18), (82, 82), (18, 82)):
        canvas.circle(x, y, 12, dark)
    canvas.line(((27, 31), (27, 71), (70, 71)), 5, (224, 237, 252))
    canvas.rect(18, 22, 18, 18, (76, 207, 220))
    canvas.rect(61, 62, 18, 18, (88, 157, 241))
    canvas.polygon(((40, 55), (69, 16), (84, 28), (57, 65), (35, 72)), (255, 198, 71))
    canvas.line(((41, 64), (52, 52)), 3, dark)
    canvas.circle(53, 51, 3, dark)
    canvas.polygon(((66, 20), (70, 15), (85, 27), (81, 32)), (255, 227, 151))


if __name__ == "__main__":
    raise SystemExit(generate(draw, ROOT / "src/res/fast-draw.ico"))
