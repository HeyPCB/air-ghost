# Air Ghost firmware (CH32V003)

One file, [`airghost.c`](airghost.c), on [ch32fun](https://github.com/cnlohr/ch32fun) (MIT), which
provides the startup code, the registers, a bit-banged I²C (`extralibs/static_i2c.h`) and the WS2812
driver (`extralibs/ws2812b_simple.h`). The comments at the top of the file say what it does.

## Build

```sh
git clone https://github.com/cnlohr/ch32fun
make build CH32FUN=/path/to/ch32fun/ch32fun PREFIX=/path/to/bin/riscv-none-elf
```

Always name the `build` target: ch32fun's own default target flashes the chip.

Built on 2026-10-05 with xPack riscv-none-elf-gcc 15.2.0 and ch32fun at commit 6b247c7, no warnings:

```
Memory region         Used Size  Region Size  %age Used
           FLASH:        3564 B        16 KB     21.75%
             RAM:          12 B         2 KB      0.59%
```

[`airghost.hex`](airghost.hex) and [`airghost.bin`](airghost.bin) are that build.

## The SCD40 conversation

| Command | Code | Used for |
|---|---|---|
| stop_periodic_measurement | 0x3F86 | at power-up, in case the sensor kept running |
| start_periodic_measurement | 0x21B1 | a reading every 5 s |
| get_data_ready_status | 0xE4B8 | polled once a second |
| read_measurement | 0xEC05 | CO2 ppm, T = -45 + 175 x raw / 65535, RH = 100 x raw / 65535 |

Each 16-bit word arrives with a CRC-8 (polynomial 0x31, initial 0xFF); a word that fails it is dropped.

## Flash

A WCH-LinkE on the three SWIO pads (VDD, SWIO, GND) under the left eye: `make flash CH32FUN=...`.

## Status

Compiled, not run on hardware. On the first prototype, check the I²C waveform (the bus runs near
100 kHz) and compare the temperature with a thermometer; if the board warms the sensor, send
`set_temperature_offset` (0x241D) once at start-up.

## License

MIT (see [`LICENSE`](LICENSE)).
