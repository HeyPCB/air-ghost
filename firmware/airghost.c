/*
 * Air Ghost - firmware for the CH32V003F4P6 (heypcb.ai merch 05), on ch32fun.
 *
 * The ghost blushes the colour of the air:
 *   CO2 < 800 ppm green, < 1000 yellow-green, < 1400 yellow, < 2000 orange, >= 2000 red and pulsing
 *   ("open a window"). While the sensor warms up after power-on, the cheeks breathe white.
 *   Boop the mouth (the button): 4 s of temperature colour (blue cold .. green comfy .. red hot),
 *   boop again: 4 s of humidity colour (orange dry .. green 40-60 % .. blue damp).
 *   Hold 2 s: night mode, cheeks dark unless CO2 passes 1400 ppm. Hold again: day mode.
 *
 * Sensor: Sensirion SCD40 at I2C 0x62, in periodic mode (a new reading every 5 s), on bit-banged I2C
 * (ch32fun's static_i2c.h) with 10k pull-ups to 5 V. Every 16-bit word carries a CRC-8 (poly 0x31,
 * init 0xFF), checked before use.
 *
 * Pins (from the board's netlist; KiCad symbol MCU_WCH_RiscV:CH32V003FxPx)
 *   PC1 (pin 11) SDA, PC2 (pin 12) SCL, PC6 (pin 16) LED_DIN to the two WS2812C-2020 cheeks,
 *   PC4 (pin 14) BOOP (button to GND, internal pull-up), PD1 SWIO; the rest unconnected (pulled up).
 *
 * Power: 5 V USB-C; SCD40 about 15 mA average in periodic mode, MCU about 4 mA, cheeks at most a few mA
 * at the brightness used here (SPEC.md).
 */
#include "ch32fun.h"

#define WS2812BSIMPLE_IMPLEMENTATION
#include "ws2812b_simple.h"

#define PIN_SDA PC1
#define PIN_SCL PC2
#define PIN_BOOP PC4

#define DELAY1 Delay_Us(2);
#define DELAY2 Delay_Us(4);
#define DSCL_IHIGH  { funPinMode(PIN_SCL, GPIO_CFGLR_IN_PUPD); funDigitalWrite(PIN_SCL, 1); }
#define DSDA_IHIGH  { funPinMode(PIN_SDA, GPIO_CFGLR_IN_PUPD); funDigitalWrite(PIN_SDA, 1); }
#define DSDA_INPUT  { funPinMode(PIN_SDA, GPIO_CFGLR_IN_PUPD); funDigitalWrite(PIN_SDA, 1); }
#define DSCL_OUTPUT { funDigitalWrite(PIN_SCL, 0); funPinMode(PIN_SCL, GPIO_CFGLR_OUT_2Mhz_PP); }
#define DSDA_OUTPUT { funDigitalWrite(PIN_SDA, 0); funPinMode(PIN_SDA, GPIO_CFGLR_OUT_2Mhz_PP); }
#define READ_DSDA funDigitalRead(PIN_SDA)
#define I2CNEEDGETBYTE 1
#include "static_i2c.h"

#define SCD40_ADDR 0x62
#define BRIGHT 40                          /* of 255: a glow, not a lamp */

volatile uint32_t ms;
void SysTick_Handler(void) __attribute__((interrupt));
void SysTick_Handler(void)
{
    SysTick->CMP += DELAY_MS_TIME;
    SysTick->SR = 0;
    ms++;
}

static void systick_init(void)
{
    SysTick->CTLR = 0;
    SysTick->CMP = DELAY_MS_TIME - 1;
    SysTick->CNT = 0;
    SysTick->CTLR = SYSTICK_CTLR_STE | SYSTICK_CTLR_STIE | SYSTICK_CTLR_STCLK;
    NVIC_EnableIRQ(SysTick_IRQn);
}

/* ---- SCD40 ---- */
static uint8_t crc8(const uint8_t *d, int n)
{
    uint8_t c = 0xFF;
    for (int i = 0; i < n; i++) {
        c ^= d[i];
        for (int b = 0; b < 8; b++)
            c = (c & 0x80) ? (uint8_t)((c << 1) ^ 0x31) : (uint8_t)(c << 1);
    }
    return c;
}

static int scd_cmd(uint16_t cmd)
{
    SendStart();
    int nak = SendByte(SCD40_ADDR << 1) | SendByte(cmd >> 8) | SendByte(cmd & 0xFF);
    SendStop();
    return nak ? -1 : 0;
}

/* Read `words` 16-bit words after `cmd` (with `wait_ms` for the sensor to answer); 0 on success. */
static int scd_read(uint16_t cmd, uint16_t *out, int words, uint32_t wait_ms)
{
    if (scd_cmd(cmd))
        return -1;
    Delay_Ms(wait_ms);
    uint8_t buf[9];
    SendStart();
    if (SendByte((SCD40_ADDR << 1) | 1)) {
        SendStop();
        return -1;
    }
    for (int i = 0; i < words * 3; i++)
        buf[i] = GetByte(i == words * 3 - 1);          /* NAK the last byte */
    SendStop();
    for (int w = 0; w < words; w++) {
        if (crc8(&buf[w * 3], 2) != buf[w * 3 + 2])
            return -2;
        out[w] = (uint16_t)((buf[w * 3] << 8) | buf[w * 3 + 1]);
    }
    return 0;
}

static uint16_t co2_ppm;                   /* 0 until the first reading */
static int16_t temp_c10;                   /* tenths of a degree C */
static uint16_t rh_10;                     /* tenths of a percent */

static void poll_sensor(void)
{
    uint16_t ready;
    if (scd_read(0xE4B8, &ready, 1, 1) || (ready & 0x07FF) == 0)
        return;                            /* get_data_ready_status: nothing new yet */
    uint16_t m[3];
    if (scd_read(0xEC05, m, 3, 1))         /* read_measurement */
        return;
    co2_ppm = m[0];
    temp_c10 = (int16_t)(-450 + (int32_t)1750 * m[1] / 65535);
    rh_10 = (uint16_t)((uint32_t)1000 * m[2] / 65535);
}

/* ---- colour ---- */
typedef struct { uint8_t r, g, b; } rgb_t;

static rgb_t mix(rgb_t a, rgb_t b, int num, int den)   /* a + (b - a) * num / den, num clamped */
{
    if (num < 0) num = 0;
    if (num > den) num = den;
    rgb_t c = { (uint8_t)(a.r + (b.r - a.r) * num / den), (uint8_t)(a.g + (b.g - a.g) * num / den),
                (uint8_t)(a.b + (b.b - a.b) * num / den) };
    return c;
}

/* A colour scale: value v against ascending stops, each with its colour. */
static rgb_t scale(int v, const int *stops, const rgb_t *cols, int n)
{
    if (v <= stops[0])
        return cols[0];
    for (int i = 1; i < n; i++)
        if (v <= stops[i])
            return mix(cols[i - 1], cols[i], v - stops[i - 1], stops[i] - stops[i - 1]);
    return cols[n - 1];
}

static const rgb_t GREEN = { 0, 255, 40 }, LIME = { 140, 255, 0 }, YELLOW = { 255, 200, 0 },
                   ORANGE = { 255, 90, 0 }, RED = { 255, 0, 0 }, BLUE = { 0, 60, 255 }, CYAN = { 0, 200, 255 };

static rgb_t co2_colour(void)
{
    static const int s[] = { 600, 800, 1000, 1400, 2000 };
    static const rgb_t c[] = { GREEN, GREEN, LIME, YELLOW, ORANGE };
    return co2_ppm >= 2000 ? RED : scale(co2_ppm, s, c, 5);
}

static rgb_t temp_colour(void)
{
    static const int s[] = { 160, 190, 220, 250, 280 };
    static const rgb_t c[] = { BLUE, CYAN, GREEN, ORANGE, RED };
    return scale(temp_c10, s, c, 5);
}

static rgb_t rh_colour(void)
{
    static const int s[] = { 250, 350, 450, 600, 700 };
    static const rgb_t c[] = { ORANGE, YELLOW, GREEN, CYAN, BLUE };
    return scale(rh_10, s, c, 5);
}

static void cheeks(rgb_t c, uint8_t level)  /* level 0..255 scales BRIGHT */
{
    uint8_t g = (uint8_t)((uint32_t)c.g * BRIGHT * level / 65025);
    uint8_t r = (uint8_t)((uint32_t)c.r * BRIGHT * level / 65025);
    uint8_t b = (uint8_t)((uint32_t)c.b * BRIGHT * level / 65025);
    uint8_t d[6] = { g, r, b, g, r, b };   /* WS2812: G, R, B per pixel */
    WS2812BSimpleSend(GPIOC, 6, d, 6);
}

static uint8_t breathe(uint32_t t, uint32_t period)
{
    uint32_t p = t % period, half = period / 2;
    uint32_t v = (p < half) ? p : period - p;
    return (uint8_t)(40 + v * 215 / half);
}

int main(void)
{
    SystemInit();
    funGpioInitAll();
    /* every unused pin an input with its pull-up */
    static const uint8_t unused[] = { PD4, PD5, PD6, PA1, PA2, PD0, PC0, PC3, PC5, PC7, PD2, PD3 };
    for (unsigned i = 0; i < sizeof unused; i++) {
        funPinMode(unused[i], GPIO_CNF_IN_PUPD);
        funDigitalWrite(unused[i], FUN_HIGH);
    }
    funPinMode(PIN_BOOP, GPIO_CNF_IN_PUPD);
    funDigitalWrite(PIN_BOOP, FUN_HIGH);
    funPinMode(PC6, GPIO_Speed_10MHz | GPIO_CNF_OUT_PP);
    funPinMode(PIN_SDA, GPIO_CFGLR_IN_PUPD);
    funPinMode(PIN_SCL, GPIO_CFGLR_IN_PUPD);
    ConfigI2C();
    systick_init();

    Delay_Ms(30);                          /* SCD40 power-up time */
    scd_cmd(0x3F86);                       /* stop_periodic_measurement, in case it was left running */
    Delay_Ms(500);
    scd_cmd(0x21B1);                       /* start_periodic_measurement: a reading every 5 s */

    enum { SHOW_CO2, SHOW_TEMP, SHOW_RH } show = SHOW_CO2;
    uint8_t night = 0;
    uint32_t show_until = 0, last_poll = 0, pressed_at = 0;
    uint8_t was_down = 0, hold_done = 0;
    for (;;) {
        uint32_t now = ms;
        if (now - last_poll >= 1000) {     /* ask once a second; the sensor answers every 5 s */
            last_poll = now;
            poll_sensor();
        }

        /* the mouth */
        uint8_t down = !funDigitalRead(PIN_BOOP);
        if (down && !was_down) {
            pressed_at = now;
            hold_done = 0;
        }
        if (down && !hold_done && now - pressed_at > 2000) {
            night = !night;
            hold_done = 1;
            show = SHOW_CO2;
        }
        if (!down && was_down && !hold_done && now - pressed_at > 30) {
            show = (show == SHOW_TEMP) ? SHOW_RH : SHOW_TEMP;
            show_until = now + 4000;
        }
        was_down = down;
        if (show != SHOW_CO2 && (int32_t)(now - show_until) > 0)
            show = SHOW_CO2;

        /* the cheeks */
        if (co2_ppm == 0) {
            cheeks((rgb_t){ 255, 255, 255 }, breathe(now, 2400));         /* warming up */
        } else if (show == SHOW_TEMP) {
            cheeks(temp_colour(), 255);
        } else if (show == SHOW_RH) {
            cheeks(rh_colour(), 255);
        } else if (night && co2_ppm < 1400) {
            cheeks((rgb_t){ 0, 0, 0 }, 0);
        } else {
            cheeks(co2_colour(), co2_ppm >= 2000 ? breathe(now, 1500) : 255);
        }
        Delay_Ms(20);
    }
}
