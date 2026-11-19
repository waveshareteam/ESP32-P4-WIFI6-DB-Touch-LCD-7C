"""Generate the original, geometry-only CAN launcher icon."""

from pathlib import Path
import sys


BROOKESIA_DIR = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(BROOKESIA_DIR / "tools"))

from icon_renderer import (  # noqa: E402
    Canvas,
    highlighted_gradient,
    vertical_gradient,
    write_lvgl_argb8888_c,
    write_rgba_png,
)


SIZE = 112
SYMBOL = "img_app_can_tool"
ATTRIBUTE = "LV_ATTRIBUTE_IMAGE_IMG_APP_CAN_TOOL"
COMPONENT_DIR = Path(__file__).resolve().parent.parent
PNG_PATH = COMPONENT_DIR / "assets" / f"{SYMBOL}.png"
C_PATH = COMPONENT_DIR / "assets" / f"{SYMBOL}.c"


def render_icon() -> bytes:
    canvas = Canvas(size=SIZE, scale=4)

    canvas.rounded_rect(
        (4, 4, 108, 108),
        24,
        highlighted_gradient(
            (66, 185, 197, 255),
            (37, 82, 105, 255),
            4,
            108,
            (29, 19),
            78,
            13,
        ),
    )

    canvas.ellipse((24, 71, 88, 85), (3, 14, 22, 45))
    for center_x in (32, 80):
        canvas.rounded_rect(
            (center_x - 7, 66, center_x + 7, 84),
            7,
            vertical_gradient(
                (57, 64, 72, 255),
                (26, 32, 38, 255),
                66,
                84,
            ),
        )

    roof = (
        (29, 47),
        (36, 31),
        (43, 25),
        (69, 25),
        (76, 31),
        (83, 47),
    )
    canvas.polygon(
        roof,
        vertical_gradient(
            (255, 255, 255, 255),
            (217, 224, 229, 255),
            25,
            48,
        ),
    )
    canvas.rounded_rect(
        (18, 43, 94, 77),
        12,
        vertical_gradient(
            (250, 252, 253, 255),
            (207, 216, 222, 255),
            43,
            77,
        ),
    )

    canvas.polygon(
        (
            (41, 31),
            (71, 31),
            (78, 46),
            (34, 46),
        ),
        vertical_gradient(
            (51, 61, 71, 255),
            (27, 34, 41, 255),
            31,
            46,
        ),
    )

    for center_x in (32, 80):
        canvas.ellipse(
            (center_x - 6, 53, center_x + 6, 65),
            (34, 133, 146, 255),
        )
        canvas.ellipse(
            (center_x - 4.4, 54.6, center_x + 4.4, 63.4),
            vertical_gradient(
                (211, 247, 248, 255),
                (101, 209, 217, 255),
                54.6,
                63.4,
            ),
        )

    canvas.rounded_rect(
        (45, 68, 67, 72),
        2,
        vertical_gradient(
            (107, 225, 230, 255),
            (37, 169, 183, 255),
            68,
            72,
        ),
    )

    return canvas.rgba_bytes()


def main() -> None:
    rgba = render_icon()
    write_rgba_png(PNG_PATH, SIZE, SIZE, rgba)
    write_lvgl_argb8888_c(C_PATH, SYMBOL, ATTRIBUTE, SIZE, SIZE, rgba)
    print(f"written: {PNG_PATH}")
    print(f"written: {C_PATH}")


if __name__ == "__main__":
    main()
