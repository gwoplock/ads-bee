#include "hal.hh"

#include <hardware/exception.h>
#include <hardware/structs/systick.h>

#include "hardware/spi.h"
#include "pico/stdlib.h"

uint64_t get_time_since_boot_us() { return to_us_since_boot(get_absolute_time()); }

uint32_t get_time_since_boot_ms() { return to_ms_since_boot(get_absolute_time()); }

volatile uint32_t ClockSource::interrupt_count_ __attribute__((aligned(4)));

ClockSource::ClockSource() {
    /* register description: ArmV6-M Manual B3.3 */

    systick_hw->csr = 0 |        /* Enable: false            */
                      (1 << 1) | /* Interrupt: true          */
                      (1 << 2);  /* Clock source: internal   */

    systick_hw->rvr = 0x00FFFFFF; /* Max counter size (24 bit)*/
    systick_hw->cvr = 0;          /* Clear current value */

    exception_set_exclusive_handler(SYSTICK_EXCEPTION, HandleIRQ);

    systick_hw->csr = systick_hw->csr | 1; /* Start timer */
}

ClockSource::~ClockSource() { systick_hw->csr = 0; }

ClockSource& ClockSource::instance() {
    static ClockSource inst;
    return inst;
}

uint64_t ClockSource::Get48MHzTickCount() {
    // Combine the wrap counter with the current value of the SysTick register and mask to 48 bits.
    // Note: 24-bit SysTick value is subtracted from UINT_24_MAX to make it count up instead of down.
    return (((interrupt_count_ << 24) | (0x00FFFFFF - systick_hw->cvr)) * 12) / 125;
}

uint64_t ClockSource::Get12MHzTickCount() { 
        // Piggyback off the higher resolution 48MHz timer function.
        return Get48MHzTickCount() >> 2; 
}

void ClockSource::HandleIRQ() {
    // Arm doesn't provide atomic instructions in ARMv6
    // and a Mutex would cause a deadlock with getTickCount.
    // So we word-align the interrupt count. That _should_
    // make the read/write a single instruction and therefore
    // atomic. I hope...

    interrupt_count_ = interrupt_count_ + 1;
}
