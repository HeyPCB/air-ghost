# 05 · Air Ghost, the desk CO₂ monitor: SPEC

## Who it's for, and the use case

People who work, study or sleep in small rooms: home offices, dorms, meeting rooms, kids' bedrooms.
CO₂ from breathing builds up fast with the door shut; above about 1000 ppm people feel drowsy and focus
drops, and nobody notices because CO₂ has no smell. Air Ghost sits on the desk and blushes the colour
of the air: green is fine, orange means it's getting stuffy, red means open a window now.

## The cute concept

A small white ghost drawn in black ink, with big eyes that have gold sparkles, three gold stars
floating by its head and a wavy hem. Its cheeks are two RGB LEDs: it blushes green, yellow, orange or
red with the CO₂. Its mouth is a round "O": the tactile button. Boo.

## Features

| | |
|---|---|
| Sensor | Sensirion SCD40: photoacoustic NDIR CO₂, 400-2000 ppm, plus temperature and humidity; a new reading every 5 s |
| Display | two WS2812C-2020 RGB LEDs (the cheeks), dimmed to a glow: green < 800 ppm, yellow-green < 1000, yellow < 1400, orange < 2000, red pulsing at 2000+; white breathing while the sensor warms up |
| Mouth (button) | boop: 4 s of temperature colour, boop again: 4 s of humidity colour; hold 2 s: night mode (dark unless CO₂ passes 1400 ppm) |
| MCU | CH32V003F4P6 from 5 V; bit-banged I²C to the sensor with CRC checks |
| Power | USB-C, 5 V |
| Board | 2 layers, white mask, black silk, ENIG; single-sided SMT, 16 placed parts; QR and legend on the back |

Why a real CO₂ sensor and not a cheap "eCO₂" one: metal-oxide VOC sensors (SGP30, ENS160, CCS811)
*estimate* CO₂ from volatile organics and are fooled by a perfume, a marker pen or cooking. The SCD40
measures CO₂ itself. The SCD40 runs Sensirion's automatic self-calibration by default; it expects to see
fresh air (about 400 ppm) at least once a week, for example when the window is opened.

## The BOM target, broken on purpose

The collection's target is a BOM under $8 at 100 units. Air Ghost's is **$15.66**: everything except the
sensor costs $0.84, and the SCD40 costs $14.82 (live price, 100 units). It is the least expensive true
CO₂ sensor on the JLC/LCSC catalogue today; Sensirion's newer low-cost STCC4 is neither stocked there nor
in KiCad's library (checked 2026-10-05). Rather than ship a ghost that guesses, this board keeps the real
sensor and is priced as the collection's premium piece. A cheaper "comfort ghost" (temperature and
humidity only, an SHT40 at $1.22) would cost about $2 in parts; it is not designed here.

## Power budget

USB-powered. SCD40 in periodic mode about 15 mA average with short current peaks of up to about 0.2 A
(datasheet typical at 3.3 V, which the 10 µF beside it supplies); CH32V003 about 4 mA; the two cheeks at
BRIGHT = 40 of 255 at most about 5 mA together. About 25 mA, 0.13 W from any USB port or charger. These
are design estimates from datasheet typicals; no Air Ghost has been built.

## BOM (100 units, live JLC/LCSC prices read 2026-10-05)

| Refs | Value | Qty | LCSC | MPN | Class | Unit @100 | Line |
|---|---|---|---|---|---|---|---|
| C1,C3 | 10uF | 2 | C15850 | CL21A106KAYNNNE | basic | $0.0713 | $0.1426 |
| C2,C4 | 100nF | 2 | C14663 | CC0603KRX7R9BB104 | basic | $0.0106 | $0.0212 |
| C5,C6 | 100nF 0402 | 2 | C1525 | CL05B104KO5NNNC | basic | $0.0060 | $0.0120 |
| D1,D2 | WS2812C-2020-V1 | 2 | C2976072 | WS2812C-2020-V1 | extended | $0.0728 | $0.1456 |
| J1 | USB-C | 1 | C165948 | TYPE-C-31-M-12 | extended | $0.1468 | $0.1468 |
| R1,R2 | 5.1k | 2 | C25905 | 0402WGF5101TCE | basic | $0.0009 | $0.0018 |
| R3,R4 | 10k | 2 | C25744 | 0402WGF1002TCE | basic | $0.0029 | $0.0058 |
| SW1 | TS-1187A | 1 | C318884 | TS-1187A-B-A-B | basic | $0.0197 | $0.0197 |
| U1 | CH32V003F4P6 | 1 | C5187096 | CH32V003F4P6 | extended | $0.2227 | $0.2227 |
| U2 | SCD40-D-R2 | 1 | C3659421 | SCD40-D-R2 | extended | $14.8224 | $14.8224 |
| | **Parts** | | | | | | **$15.54** |
| | Extended-part loading fee ($3 x 4 / 100 boards) | | | | | | $0.12 |
| | **BOM per board @100** | | | | | | **$15.66** |

Not in the BOM: the PCB (2 layers, ENIG, 50.7 x 62.3 mm), assembly labour and a USB-C cable. TP1-TP3
are copper pads. Machine-readable: [`bom_cost.json`](bom_cost.json).

## Dimensions

50.7 x 62.3 mm (Edge.Cuts bounding box, pcbnew), 26.5 cm², 1.6 mm FR4. Tallest part: the SCD40 module
(6.5 mm).

## Mounting plan (no enclosure)

Two ways, both without a case: lean it against the monitor stand or a book, face out, with the cable
from its right side; or stick a 20 mm adhesive Velcro dot to the back (no parts there) and put the other
half on the monitor bezel or a shelf edge. Keep the sensor's top window uncovered and away from the
exhaust of a laptop.

## Safety

5 V USB only. Every corner of the outline is rounded (1.5 mm minimum, the hem scallops included).
Nothing on it gets warm: the SCD40's light source pulses briefly, and the LEDs are dimmed.

## Verification (measured by KiCad 10.0.6 on the files in this folder)

ERC 0 errors · netlist round trip identical (12 nets) · routing 50 → 0 open (176 tracks, 10 vias) · DRC
0 errors / 0 unconnected / 0 parity, 1 warning (`lib_footprint_mismatch` on the USB-C: its overhanging
footprint silk clipped) · DFM `jlcpcb_2layer` 0 errors · fab gate ready. Full numbers:
[`report.json`](report.json), [`measured.json`](measured.json).

Two SDK changes came out of this board: art polygons may carry real holes (a fractured outline rendered
a seam across each eye's sparkle), and KiCad's missing SCD4x 3D model is replaced by a generated
envelope (the sensor rendered as bare pads in the Circuit World thumbnail).

**Not built yet.** Not fabricated, assembled or powered; the firmware compiles but has not run on
hardware. Check the I²C timing with the SCD40 (the bit-banged bus runs near 100 kHz) and the temperature
reading against a thermometer (set the SCD40's temperature offset if the board warms it).
