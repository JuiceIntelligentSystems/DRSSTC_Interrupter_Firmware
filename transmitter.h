#ifndef TRANSMITTER_H
#define TRANSMITTER_H

#include <pico/stdlib.h>
#include <pico/float.h>
#include <hardware/pwm.h>
#include <hardware/irq.h>
#include <math.h>
#include "util.h"

#define TC_TX 24
#define STATUS_LED 25

#define MAX_PULSE_WIDTH 400.f // us
#define MIN_PULSE_WIDTH 30.f  // us

volatile bool pwm_off = false;
volatile bool pwm_music = false;
volatile bool pwm_set = false;

volatile uint8_t note_tx;
volatile uint8_t velocity_tx;

volatile uint16_t freq_input;
volatile uint16_t duty_input;

uint16_t previous_pot_freq, previous_pot_duty;

// Store Range of frequencies supported by Tesla Coil
const uint16_t frequency_table[18] = {
    15, 20, 25, 30, 35, 40, 45, 50, 100, 200, 300, 400, 500, 600, 700, 800, 900, 1000};

// Utility
int map(int, int, int, int, int);

uint32_t set_transmitter_freq_duty(uint, uint, uint32_t, int);
uint32_t set_transmitter_freq_duty(uint, uint, uint32_t, float);
void transmitter_init();
void pwm_irq_handler();
void transmitt_music(uint16_t, uint16_t);
void transmitt_off();
void set_transmitter(uint16_t, uint16_t);
void reset_transmitter(void);

int map(int v, int a1, int a2, int b1, int b2)
{
    return b1 + (v - a1) * (b2 - b1) / (a2 - a1);
}

void transmitter_init()
{
    gpio_set_function(TC_TX, GPIO_FUNC_PWM);
    gpio_set_function(STATUS_LED, GPIO_FUNC_PWM);

    uint slice_num = pwm_gpio_to_slice_num(TC_TX);
    uint slice_num_stat = pwm_gpio_to_slice_num(STATUS_LED);

    pwm_clear_irq(slice_num);
    pwm_clear_irq(slice_num_stat);

    pwm_set_irq_enabled(slice_num, true);
    pwm_set_irq_enabled(slice_num_stat, true);

    irq_set_exclusive_handler(PWM_IRQ_WRAP, pwm_irq_handler);
    irq_set_enabled(PWM_IRQ_WRAP, true);

    pwm_config config = pwm_get_default_config();
    // pwm_config_set_clkdiv(&config, 16.0);
    // pwm_config_set_wrap(&config, 1000);

    pwm_init(slice_num, &config, true);
    pwm_init(slice_num_stat, &config, true);
}

uint32_t set_transmitter_freq_duty(uint slice_num, uint chan, uint32_t f, int d)
{
    // Calculate best clock and wrap value for pwm
    uint32_t clock = 125000000;
    uint32_t divider16 = clock / f / 4096 + (clock % (f * 4096) != 0);

    if (divider16 / 16 == 0)
        divider16 = 16;

    uint32_t wrap = clock * 16 / divider16 / f - 1;
    uint32_t high_time = wrap * d / 100;

    pwm_set_clkdiv_int_frac(slice_num, divider16 / 16, divider16 & 0xF);
    pwm_set_wrap(slice_num, wrap);
    pwm_set_chan_level(slice_num, chan, high_time);

    return wrap;
}

uint32_t set_transmitter_freq_duty(uint slice_num, uint chan, uint32_t f, float d)
{
    // Calculate best clock and wrap value for pwm
    uint32_t clock = 125000000;
    uint32_t divider16 = clock / f / 4096 + (clock % (f * 4096) != 0);

    if (divider16 / 16 == 0)
        divider16 = 16;

    uint32_t wrap = clock * 16 / divider16 / f - 1;
    uint32_t high_time = (uint32_t)(wrap * d / 100.0); // If the duty % is a floating point value

    pwm_set_clkdiv_int_frac(slice_num, divider16 / 16, divider16 & 0xF);
    pwm_set_wrap(slice_num, wrap);
    pwm_set_chan_level(slice_num, chan, high_time);

    return wrap;
}

void transmitt_music(uint8_t note, uint8_t velocity)
{
    pwm_music = true;

    note_tx = note;
    velocity_tx = velocity;
}

void transmitt_off()
{
    pwm_off = true;
}

void set_transmitter(uint16_t frequency_pot, uint16_t duty_cycle_pot)
{
    pwm_set = true;

    freq_input = frequency_pot;
    duty_input = duty_cycle_pot;
}

void pwm_irq_handler()
{
    uint16_t slice_num_tx = pwm_gpio_to_slice_num(TC_TX);
    uint16_t slice_num_stat = pwm_gpio_to_slice_num(STATUS_LED);

    uint16_t tx_channel = pwm_gpio_to_channel(TC_TX);
    uint16_t stat_channel = pwm_gpio_to_channel(STATUS_LED);

    pwm_clear_irq(slice_num_tx);
    pwm_clear_irq(slice_num_stat);

    if (pwm_off == true)
    {
        pwm_set_chan_level(slice_num_tx, tx_channel, 0);
        pwm_set_chan_level(slice_num_stat, stat_channel, 0);

        pwm_off = false;
    }
    if (pwm_music == true)
    {
        // Make it only register notes C1-B5 so coil doesn't overload
        // In otherwords, limit frequencies from 32.70Hz to 987.77Hz
        if (note_tx > 23 && note_tx < 84 && velocity_tx < 128)
        {
            // Map midi values to frequencies and duty cycles
            // Map to frequencies with formula: f=440*2^((n-69)/12)) - tuned A4 at 440Hz
            uint32_t frequency = 440 * pow(2, ((float)note_tx - 69.0) / 12.0);
            float pulse_width = MIN_PULSE_WIDTH + ((MAX_PULSE_WIDTH - MIN_PULSE_WIDTH) * velocity_tx) / 127.0;
            float duty_cycle = ((pulse_width / 1000000.f) / (1.f / frequency)) * 100.f;

            set_transmitter_freq_duty(slice_num_tx, tx_channel, frequency, duty_cycle);
            set_transmitter_freq_duty(slice_num_stat, stat_channel, frequency, duty_cycle);
        }
        else
        {
            pwm_set_chan_level(slice_num_tx, tx_channel, 0);
            pwm_set_chan_level(slice_num_stat, stat_channel, 0);
        }

        pwm_music = false;
    }
    if (pwm_set == true)
    {
        if (previous_pot_duty != freq_input || previous_pot_freq != duty_input)
        {
            // Map pot values to frequencies and duty cycles
            uint8_t index = map(duty_input, 0, 4095, 0, 17);
            uint32_t frequency = frequency_table[index];

            float pulse_width = MIN_PULSE_WIDTH + ((MAX_PULSE_WIDTH - MIN_PULSE_WIDTH) * freq_input) / 4095.0;
            float duty_cycle = ((pulse_width / 1000000.f) / (1.f / (float)frequency)) * 100.f;

            set_transmitter_freq_duty(slice_num_tx, tx_channel, frequency, duty_cycle);
            set_transmitter_freq_duty(slice_num_stat, stat_channel, frequency, duty_cycle);
        }
        previous_pot_duty = duty_input;
        previous_pot_freq = freq_input;

        pwm_set = false;
    }
}

void reset_transmitter(void)
{
    pwm_off = false;
    pwm_music = false;
    pwm_set = false;

    note_tx = 0;
    velocity_tx = 0;
    freq_input = 0;
    duty_input = 0;

    previous_pot_freq = 0;
    previous_pot_duty = 0;

    uint16_t slice_num_tx = pwm_gpio_to_slice_num(TC_TX);
    uint16_t slice_num_stat = pwm_gpio_to_slice_num(STATUS_LED);
    pwm_set_chan_level(slice_num_tx, pwm_gpio_to_channel(TC_TX), 0);
    pwm_set_chan_level(slice_num_stat, pwm_gpio_to_channel(STATUS_LED), 0);
}

#endif