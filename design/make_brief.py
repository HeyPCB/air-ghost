"""Writes boards/merch_air_ghost/board.json: Air Ghost, the desk CO2 monitor (heypcb.ai merch 05).

    "$KICAD_PYTHON" boards/merch_air_ghost/make_brief.py

A little white ghost that blushes green when the air is fresh and red when the room needs a window
opened. A Sensirion SCD40 measures real CO2 (photoacoustic NDIR, not a VOC estimate) plus temperature
and humidity; a CH32V003 from USB-C turns it into the colour of two WS2812 cheeks. Its "O" mouth is
the button. Edit this file, not board.json. Frame 58 x 68 mm, origin top-left, y down.
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.dirname(HERE))
import merch_kit as K  # noqa: E402

SLUG = "air-ghost"
W, H = 58.0, 68.0
CX = 28.0
EYE_Y = 23.5

MCU_PINS = {"U?.11": "SDA", "U?.12": "SCL", "U?.16": "LED_DIN", "U?.14": "BOOP"}
MCU_NC = ["U?.%d" % p for p in (1, 2, 3, 4, 5, 6, 8, 10, 13, 15, 17, 19, 20)]

BLOCKS = [
    {"block": "usb_c_data_5v", "as": "usb", "title": "USB-C: 5 V in (data pins unused)",
     "nets": {"USB_DP": "USB_DP", "USB_DM": "USB_DM"}},
    {"block": "merch_mcu_ch32v003", "as": "mcu", "title": "CH32V003: I2C to the sensor, WS2812 cheeks",
     "nets": {"VDD": "VBUS"}, "pinNets": MCU_PINS, "no_connect": MCU_NC},
    {"block": "merch_scd40", "as": "co2", "title": "SCD40: CO2, temperature, humidity",
     "nets": {"VDD": "VBUS", "SDA": "SDA", "SCL": "SCL"}},
    {"block": "irisring_pixel", "as": "cheek_l", "title": "Left cheek: WS2812C-2020",
     "nets": {"VDD": "VBUS"}, "pinNets": {"D?.3": "LED_DIN", "D?.1": "LED_CHAIN"}},
    {"block": "irisring_pixel", "as": "cheek_r", "title": "Right cheek: WS2812C-2020",
     "nets": {"VDD": "VBUS"}, "pinNets": {"D?.3": "LED_CHAIN"}, "no_connect": ["D?.1"]},
    {"block": "merch_button", "as": "mouth", "title": "The 'O' mouth: boop for temperature, then humidity",
     "nets": {"SIG": "BOOP"}},
]

# ---------------------------------------------------------------------------------------- outline
dome = K.Shape.circle(CX, 25.0, 23.0)
body = K.Shape.rect(5.0, 25.0, 51.0, 59.0)
hem = K.Shape.rect(0, 0, 0.1, 0.1)
for x in (10.75, 22.25, 33.75, 45.25):
    hem = hem | K.Shape.circle(x, 58.5, 5.75)
usb_flat = K.Shape.rect(44.0, 35.0, 55.6, 50.0)
outline = (dome | body | hem | usb_flat).rounded(1.5)

# ---------------------------------------------------------------------------------------- parts
parts = K.design_footprints(BLOCKS, os.path.join(os.path.dirname(HERE), "..", "blocks"))
L = K.Layout(parts)
L.lock("mouth:SW?", CX, 33.6, 0, "the 'O' mouth")
L.lock("cheek_l:D?", CX - 11.5, 31.0, 0, "left blush")
L.lock("cheek_r:D?", CX + 11.5, 31.0, 0, "right blush")
L.lock("cheek_l:C?", CX - 11.5, 33.1, 0, "inside the blush ring")
L.lock("cheek_r:C?", CX + 11.5, 33.1, 0, "inside the blush ring")
L.lock("co2:U?", CX + 1.0, 48.0, 0, "the sensor in the belly, in open air")
L.lock("co2:C1?", CX + 9.0, 44.6, 90, "sensor bulk")
L.lock("co2:C2?", CX + 9.0, 49.6, 90, "sensor bypass")
L.lock("co2:R1?", CX - 7.2, 52.6, 90, "SDA pull-up")
L.lock("co2:R2?", CX - 5.8, 52.6, 90, "SCL pull-up")
L.lock("mcu:U?", 13.4, 45.4, 0, "MCU at the left of the belly, I2C pins toward the sensor")
L.lock("mcu:C?", 13.4, 40.4, 0, "bypass")
for i in range(3):
    L.lock("mcu:TP%d?" % (i + 1), 10.8 + 2.54 * i, 53.2, 0, "SWIO pads (VDD, SWIO, GND)")
L.lock("usb:J?", 51.6, 42.5, 90, "USB-C on the right flat, mouth out", overhang=True)
L.lock("usb:C?", 44.6, 36.6, 0, "VBUS bulk")
L.lock("usb:R1?", 45.0, 48.6, 0, "CC1 5.1k")
L.lock("usb:R2?", 45.0, 50.4, 0, "CC2 5.1k")

# ---------------------------------------------------------------------------------------- art
art = []
for sx in (-1, 1):
    ex = CX + sx * 8.0
    eye = K.Shape.ellipse(ex, EYE_Y, 3.3, 4.6)
    sparkle = K.Shape.circle(ex + 1.2, EYE_Y - 1.7, 0.95)
    art += K.silk_fill(eye - sparkle.offset(0.35), "eye")
    art += K.gold(sparkle.offset(-0.3), "eye_sparkle")
    # dashed blush round each cheek LED
    for k in range(10):
        a0 = 36.0 * k
        art += K.silk_line(K.arc_points(CX + sx * 11.5, 31.0, 3.3, a0, a0 + 20.0, 4), "blush", 0.2)
# gold stars floating by the dome
for (sx, sy, r) in ((12.5, 12.5, 1.6), (43.5, 12.0, 1.4), (17.5, 7.5, 1.0)):
    art += K.gold(K.star(sx, sy, r), "star")
art += K.text("boo", CX, 39.4, size=1.0, italic=True, name="boo")
art += K.text("heypcb.ai", CX, 57.0, size=1.0, name="brand_front")
# back: QR, the legend, the brand
art += K.qr(K.world_url(SLUG), CX, 42.0, size=12.0, side="B")
legend = ["my cheeks are the CO2:", "green < 800 ppm  yellow < 1200",
          "orange < 2000  red: open a window", "boop my mouth: temperature,",
          "boop again: humidity", "hold 2 s: night mode"]
for i, line in enumerate(legend):
    art += K.text(line, CX, 11.5 + 2.0 * i, size=1.0, side="B", name="legend%d" % i)
art += K.text("heypcb.ai", CX, 56.5, size=1.3, side="B", bold=True, name="brand_back")

keep = {}
for i, it in enumerate(a for a in art if "F.Cu" in a.get("layers", [])):
    keep["gold%d" % i] = K.Shape.poly(it["points"]).offset(0.3)
L.check(outline, edge=0.5, keep=keep)

brief = {
    "name": "air_ghost",
    "title": "Air Ghost - desk CO2 / temperature / humidity monitor, SCD40, USB-C",
    "revision": "A",
    "brief": "A ghost-shaped desk air monitor on USB-C. A Sensirion SCD40 (photoacoustic CO2 400-2000 ppm plus "
             "temperature and humidity, I2C, 5 V) is read every 5 s by a CH32V003 running from VBUS, which sets the "
             "colour of two WS2812C-2020 cheeks: green under 800 ppm through yellow and orange to red above 2000. "
             "The tactile button is the ghost's 'O' mouth: boop for temperature and humidity colours, hold for "
             "night mode. 2 layers, single-sided SMT, white mask, black silk, ENIG.",
    "blocks": BLOCKS,
    "pwrFlags": ["GND", "VBUS"],
    "netClasses": [],
    "board": {
        "width": W, "height": H, "corner_radius": 0.0, "layers": 2, "origin": [100.0, 100.0],
        "shape": {"type": "polygon", "points": K.outline_points(outline), "fillet": 0},
        "rules": {"clearance": 0.2, "track": 0.2, "via": [0.6, 0.3], "edge_clearance": 0.5,
                  "hole_to_hole": 0.5, "min_through_hole": 0.3, "min_track": 0.15},
        "finish": {"mask": "White", "silk": "Black", "copper": "ENIG",
                   "why": "a white ghost drawn in black ink; gold stars and eye sparkles"},
        "unpoured": ["F.Cu"],
        "notes": "2 layers; the face is unpoured, GND plane on the back.",
    },
    "placement": L.locks,
    "route": {"passes": 30, "fanout": True, "solve_s": 240},
    "stitch_pitch": 6.0,
    "silk": {"designators": "hide",
             "title": [{"text": "Air Ghost", "y": 26.0, "size": 1.8, "bold": True},
                       {"text": "CO2 monitor  rev {revision}", "y": 28.8, "size": 0.9}]},
    "art": art,
    "captions": {"done_3d": "Air Ghost: an SCD40 CO2 sensor in its belly, WS2812 cheeks that blush with the air."},
}
K.write_brief(os.path.join(HERE, "board.json"), brief)
print("wrote board.json: %d outline points, %d art items, %d locks, QR -> %s"
      % (len(brief["board"]["shape"]["points"]), len(art), len(L.locks), K.world_url(SLUG)))
