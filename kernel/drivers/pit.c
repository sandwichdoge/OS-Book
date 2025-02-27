#include "drivers/pit.h"

#include "builddef.h"
#include "drivers/io.h"
#include "utils/debug.h"

#define PIT_DATA_CH0_PORT 0x40
#define PIT_DATA_CH1_PORT 0x41
#define PIT_DATA_CH2_PORT 0x42
#define PIT_COMMAND_PORT 0x43
#define PIT_INPUT_FREQ 1193182

public
void pit_setfreq(uint32_t hz) {
    _dbg_log("Setup timer running at %u hz.\n", hz);

    if (hz < 20) {
        _dbg_log("Freq too low: %u.\n", hz);
        return;
    } else if (hz > PIT_INPUT_FREQ) {
        _dbg_log("Freq too high: %u.\n", hz);
        return;
    }

    // PIT's input clock (1193182 Hz). freq = 1193182 / divisor
    uint16_t divisor = PIT_INPUT_FREQ / hz;
    uint8_t l = (uint8_t)(divisor & 0xff);
    uint8_t h = (uint8_t)((divisor >> 8) & 0xff);

    /*
    Send command byte
    - Raise interrupts (use channel 0)
    - Send the divider as low byte then high byte
    - Use a square wave
    - Use binary mode
    */
    outb(PIT_COMMAND_PORT, 0x36);

    // Send frequency divisor
    outb(PIT_DATA_CH0_PORT, l);
    outb(PIT_DATA_CH0_PORT, h);
}
