#include "ads_bee.hh"

#include <hardware/structs/systick.h>
#include <stdio.h>  // for printing

#include "hardware/adc.h"
#include "hardware/clocks.h"
#include "hardware/exception.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "hardware/pwm.h"
#include "pico/stdlib.h"
#include "pico/time.h"
// #include "hardware/dma.h"
#include "capture.pio.h"
#include "hal.hh"
#include "hardware/irq.h"
#include "pico/binary_info.h"

// #include <charconv>
#include <string.h>  // for strcat

#include "comms.hh"  // For debug prints.

const uint16_t kStatusLEDBootupNumBlinks = 4;
const uint16_t kStatusLEDBootupBlinkPeriodMs = 200;
constexpr float kPreambleDetectorFreq = 48e6;    // Running at 16MHz (8 clock cycles per half bit).
constexpr float kMessageDemodulatorFreq = 16e6;  // Run at 16 MHz to demodulate bits at 1Mbps.

const uint8_t kRxGainDigipotI2CAddr = 0b0101111;  // MCP4017-104e
const uint32_t kRxgainDigipotOhmsPerCount = 100e3 / 127;

ADSBee *isr_access = nullptr;

void on_demod_complete() { isr_access->OnDemodComplete(); }

void on_systick_exception() { isr_access->OnSysTickWrap(); }

void on_demod_begin(uint gpio, uint32_t event_mask) { isr_access->OnDemodBegin(gpio, event_mask); }

ADSBee::ADSBee(ADSBeeConfig config_in) {
    config_ = config_in;

    preamble_detector_sm_ = pio_claim_unused_sm(config_.preamble_detector_pio, true);
    preamble_detector_offset_ = pio_add_program(config_.preamble_detector_pio, &preamble_detector_program);

    message_demodulator_sm_ = pio_claim_unused_sm(config_.message_demodulator_pio, true);
    message_demodulator_offset_ = pio_add_program(config_.message_demodulator_pio, &message_demodulator_program);

    // Put IRQ parameters into the global scope for the on_demod_complete ISR.
    isr_access = this;

    // Figure out slice and channel values that will be used for setting PWM duty cycle.
    tl_pwm_slice_ = pwm_gpio_to_slice_num(config_.tl_pwm_pin);
    tl_pwm_chan_ = pwm_gpio_to_channel(config_.tl_pwm_pin);
}

bool ADSBee::Init() {
    // Initialize the TL bias PWM output.
    gpio_set_function(config_.tl_pwm_pin, GPIO_FUNC_PWM);
    pwm_set_wrap(tl_pwm_slice_, kTLMaxPWMCount);

    SetTLMilliVolts(SettingsManager::kDefaultTLMV);
    pwm_set_enabled(tl_pwm_slice_, true);

    // Initialize the ML bias ADC input.
    adc_init();
    adc_gpio_init(config_.tl_adc_pin);
    adc_gpio_init(config_.rssi_hold_adc_pin);

    // Initialize RSSI peak detector clear pin.
    gpio_init(config_.rssi_clear_pin);
    gpio_set_dir(config_.rssi_clear_pin, GPIO_OUT);
    gpio_put(config_.rssi_clear_pin, 1);  // RSSI clear is active LO.

    // Initialize I2C for talking to the EEPROM and rx gain digipot.
    i2c_init(config_.onboard_i2c, config_.onboard_i2c_clk_freq_hz);
    gpio_set_function(config_.onboard_i2c_sda_pin, GPIO_FUNC_I2C);
    gpio_set_function(config_.onboard_i2c_scl_pin, GPIO_FUNC_I2C);
    uint8_t wiper_value_counts;
    if (i2c_read_blocking(config_.onboard_i2c, kRxGainDigipotI2CAddr, &wiper_value_counts, 1, false) != 1) {
        CONSOLE_ERROR("ADSBee::Init", "Failed to read wiper position from Rx Gain Digipot at I2C address 0x%x.",
                      kRxGainDigipotI2CAddr);
        return false;
    }

    // Enable the MLAT timer using the 24-bit SysTick timer connected to the 125MHz processor clock.
    // SysTick Control and Status Register
    systick_hw->csr = 0b110;  // Source = External Reference Clock, TickInt = Enabled, Counter = Disabled.
    // SysTick Reload Value Register
    systick_hw->rvr = 0xFFFFFF;  // Use the full 24 bit span of the timer value register.
    // 0xFFFFFF = 16777215 counts @ 125MHz = approx. 0.134 seconds.
    // Call the OnSysTickWrap function every time the SysTick timer hits 0.
    exception_set_exclusive_handler(SYSTICK_EXCEPTION, on_systick_exception);
    // Let the games begin!
    systick_hw->csr |= 0b1;  // Enable the counter.

    // Calculate the PIO clock divider.
    float preamble_detector_div = (float)clock_get_hz(clk_sys) / kPreambleDetectorFreq;

    // Initialize the program using the .pio file helper function
    preamble_detector_program_init(config_.preamble_detector_pio, preamble_detector_sm_, preamble_detector_offset_,
                                   config_.pulses_pins[0], config_.demod_pins[0], preamble_detector_div);

    // Handle GPIO interrupts.
    gpio_set_irq_enabled_with_callback(config_.demod_pins[0], GPIO_IRQ_EDGE_RISE, true, on_demod_begin);

    // Enable the DEMOD interrupt on PIO1_IRQ_0.
    pio_set_irq0_source_enabled(config_.preamble_detector_pio, pis_interrupt0, true);  // PIO1 state machine 0 IRQ 0

    // Handle PIO0 IRQ0.
    irq_set_exclusive_handler(config_.preamble_detector_demod_complete_irq, on_demod_complete);
    irq_set_enabled(config_.preamble_detector_demod_complete_irq, true);

    /** MESSAGE DEMODULATOR PIO **/
    float message_demodulator_div = (float)clock_get_hz(clk_sys) / kMessageDemodulatorFreq;
    message_demodulator_program_init(config_.message_demodulator_pio, message_demodulator_sm_,
                                     message_demodulator_offset_, config_.pulses_pins[0], config_.recovered_clk_pins[0],
                                     message_demodulator_div);

    // Set GPIO interrupts to be higher priority than the DEMOD interrupt to allow RSSI measurement.
    irq_set_priority(config_.preamble_detector_demod_complete_irq, 1);
    irq_set_priority(config_.preamble_detector_demod_begin_irq, 0);

    CONSOLE_INFO("ADSBee::Init", "PIOs initialized.");

    gpio_init(config_.status_led_pin);
    gpio_set_dir(config_.status_led_pin, GPIO_OUT);

    // Enable the state machines
    pio_sm_set_enabled(config_.preamble_detector_pio, preamble_detector_sm_, true);
    pio_sm_set_enabled(config_.message_demodulator_pio, message_demodulator_sm_, true);

    // Set the last dictionary update timestamp.
    last_aircraft_dictionary_update_timestamp_ms_ = get_time_since_boot_ms();

    // Blink the LED a few times to indicate a successful startup.
    for (uint16_t i = 0; i < kStatusLEDBootupNumBlinks; i++) {
        gpio_put(config_.status_led_pin, 1);
        sleep_ms(kStatusLEDBootupBlinkPeriodMs / 2);
        gpio_put(config_.status_led_pin, 0);
        sleep_ms(kStatusLEDBootupBlinkPeriodMs / 2);
    }
    return true;
}

bool ADSBee::Update() {
    uint32_t timestamp_ms = get_time_since_boot_ms();
    // Turn off the demod LED if it's been on for long enough.
    if (timestamp_ms > led_off_timestamp_ms_) {
        gpio_put(config_.status_led_pin, 0);
    }

    // Update PWM output duty cycle.
    pwm_set_chan_level(tl_pwm_slice_, tl_pwm_chan_, tl_pwm_count_);

    // Prune aircraft dictionary.
    if (last_aircraft_dictionary_update_timestamp_ms_ - timestamp_ms > config_.aircraft_dictionary_update_interval_ms) {
        aircraft_dictionary.Update(timestamp_ms);
    }
    return true;
}

void ADSBee::OnDemodBegin(uint gpio, uint32_t event_mask) {
    if (gpio == config_.demod_pins[0] && event_mask == GPIO_IRQ_EDGE_RISE) {
        gpio_acknowledge_irq(config_.demod_pins[0], GPIO_IRQ_EDGE_RISE);
        // Demodulation period is beginning!
        // Store the MLAT counter.
        rx_packet_.mlat_48mhz_64bit_counts = GetMLAT48MHzCounts();
    }
}

void ADSBee::OnDemodComplete() {
    pio_sm_set_enabled(config_.message_demodulator_pio, message_demodulator_sm_, false);
    // Read the RSSI level of the current packet.
    rx_packet_.rssi_dbm = ReadRSSIdBm();
    if (!pio_sm_is_rx_fifo_full(config_.message_demodulator_pio, message_demodulator_sm_)) {
        // Push any partially complete 32-bit word onto the RX FIFO.
        pio_sm_exec_wait_blocking(config_.message_demodulator_pio, message_demodulator_sm_,
                                  pio_encode_push(false, true));
    }

    // Clear the transponder packet buffer.
    memset((void *)rx_packet_.buffer, 0x0, RawTransponderPacket::kMaxPacketLenWords32);

    // Pull all words out of the RX FIFO.
    volatile uint16_t packet_num_words =
        pio_sm_get_rx_fifo_level(config_.message_demodulator_pio, message_demodulator_sm_);
    if (packet_num_words > RawTransponderPacket::kMaxPacketLenWords32) {
        // Packet length is invalid; dump everything and try again next time.
        // Only enable this print for debugging! Printing from the interrupt causes the USB library to crash.
        // CONSOLE_WARNING("ADSBee::OnDemodComplete", "Received a packet with %d 32-bit words, expected maximum of %d.",
        //                 packet_num_words, DecodedTransponderPacket::kExtendedSquitterPacketNumWords32);
        // pio_sm_clear_fifos(config_.message_demodulator_pio, message_demodulator_sm_);
        packet_num_words = RawTransponderPacket::kMaxPacketLenWords32;
    }
    // Create a RawTransponderPacket and push it onto the queue.
    for (uint16_t i = 0; i < packet_num_words; i++) {
        rx_packet_.buffer[i] = pio_sm_get(config_.message_demodulator_pio, message_demodulator_sm_);
        if (i == packet_num_words - 1) {
            // // Trim off extra ingested bit from last word in the packet.
            // word  = word >> 1;
            // Mask and left align final word based on bit length.
            switch (packet_num_words) {
                case DecodedTransponderPacket::kSquitterPacketNumWords32:
                    rx_packet_.buffer[i] = (rx_packet_.buffer[i] & 0xFFFFFF) << 8;
                    rx_packet_.buffer_len_bits = DecodedTransponderPacket::kSquitterPacketLenBits;
                    transponder_packet_queue.Push(rx_packet_);
                    break;
                case DecodedTransponderPacket::kExtendedSquitterPacketNumWords32:
                    rx_packet_.buffer[i] = (rx_packet_.buffer[i] & 0xFFFF) << 16;
                    rx_packet_.buffer_len_bits = DecodedTransponderPacket::kExtendedSquitterPacketLenBits;
                    transponder_packet_queue.Push(rx_packet_);
                    break;
                default:
                    // Don't push partial packets.
                    // Printing to tinyUSB from within an interrupt causes crashes! Don't do it.
                    // CONSOLE_WARNING(
                    //     "ADSBee::OnDemodComplete",
                    //     "Unhandled case while creating RawTransponderPacket, received packet with %d 32-bit words.",
                    //     packet_num_words);
                    break;
            }
        }
    }

    gpio_put(config_.rssi_clear_pin, 1);  // restore RSSI peak detector to working order.

    // Reset the demodulator state machine to wait for the next decode interval, then enable it.
    pio_sm_restart(config_.message_demodulator_pio, message_demodulator_sm_);  // Reset FIFOs, ISRs, etc.
    pio_sm_exec_wait_blocking(config_.message_demodulator_pio, message_demodulator_sm_,
                              pio_encode_jmp(message_demodulator_offset_));  // Jump to beginning of program.
    pio_sm_set_enabled(config_.message_demodulator_pio, message_demodulator_sm_, true);

    // Release the preamble detector from its wait state.
    pio_interrupt_clear(config_.preamble_detector_pio, 0);
}

void ADSBee::OnSysTickWrap() { mlat_counter_1s_wraps_++; }

uint64_t ADSBee::GetMLAT48MHzCounts(uint16_t num_bits) {
    // Combine the wrap counter with the current value of the SysTick register and mask to 48 bits.
    // Note: 24-bit SysTick value is subtracted from UINT_24_MAX to make it count up instead of down.
    return (mlat_counter_1s_wraps_ << 24 | (0xFFFFFF - systick_hw->cvr)) & (UINT64_MAX >> (64 - num_bits));
}

uint64_t ADSBee::GetMLAT12MHzCounts(uint16_t num_bits) {
    // Piggyback off the higher resolution 48MHz timer function.
    return GetMLAT48MHzCounts(50) >> 2;  // Divide 48MHz counter by 4, widen the mask by 2 bits to compensate.
}

bool ADSBee::SetTLMilliVolts(int tl_mv) {
    if (tl_mv > kTLMaxMV || tl_mv < kTLMinMV) {
        CONSOLE_ERROR("ADSBee::SetTLMilliVolts", "Unable to set tl_mv_ to %d, outside of permissible range %d-%d.\r\n",
                      tl_mv, kTLMinMV, kTLMaxMV);
        return false;
    }
    tl_mv_ = tl_mv;
    tl_pwm_count_ = tl_mv_ * kTLMaxPWMCount / kVDDMV;

    return true;
}

inline int adc_counts_to_mv(uint16_t adc_counts) { return 3300 * adc_counts / 0xFFF; }

int ADSBee::ReadTLMilliVolts() {
    // Read back the low level TL bias output voltage.
    adc_select_input(config_.tl_adc_input);
    tl_adc_counts_ = adc_read();
    return adc_counts_to_mv(tl_adc_counts_);
}

int ADSBee::ReadRSSIdBm() {
    adc_select_input(config_.rssi_hold_adc_input);
    int rssi_adc_counts = adc_read();
    int rssi_mv = rssi_adc_counts * 3300 / 4095;
    return 60 * (rssi_mv - 1600) / 1000;  // AD8313 0dBm intercept at 1.6V, slope is 60dBm/V.
}

void ADSBee::FlashStatusLED(uint32_t led_on_ms) {
    gpio_put(config_.status_led_pin, 1);
    led_off_timestamp_ms_ = get_time_since_boot_ms() + kStatusLEDOnMs;
}