#ifndef INPUTS_H
#define INPUTS_H

#include <pico/stdlib.h>
#include <pico/time.h>
#include <hardware/adc.h>
#include "util.h"

#define FREQ_PIN 27
#define DUTY_PIN 26
#define SEL_PIN 28
#define SCROLL_PIN 29

class Inputs
{
private:
    int selState = 0;
    int scrollState = 0;
    int lastSelState = 0;
    int lastScrollState = 0;

    int selReading = 0;
    int scrollReading = 0;

    unsigned long last_sel_debounce_time = 0;
    unsigned long last_scroll_debounce_time = 0;
    unsigned long debounce_delay = 50;

    uint32_t getTimeMs(void);

public:
    void init_pots();
    void init_buttons();
    uint16_t read_dutycycle();
    uint16_t read_frequency();
    bool select();
    bool scroll();
    void updateButtons();
};

uint32_t Inputs::getTimeMs(void)
{
    return to_ms_since_boot(get_absolute_time());
}

void Inputs::init_pots()
{
    adc_init();
    adc_gpio_init(FREQ_PIN);
    adc_gpio_init(DUTY_PIN);
}

void Inputs::init_buttons()
{
    gpio_init(SEL_PIN);
    gpio_init(SCROLL_PIN);
    gpio_set_dir(SEL_PIN, GPIO_IN);
    gpio_set_dir(SCROLL_PIN, GPIO_IN);
}

uint16_t Inputs::read_dutycycle()
{
    adc_select_input(0);

    uint16_t output = adc_read();
    return output;
}

uint16_t Inputs::read_frequency()
{
    adc_select_input(1);

    uint16_t output = adc_read();
    return output;
}

bool Inputs::select()
{
    selReading = gpio_get(SEL_PIN);

    if (selReading != lastSelState)
        last_sel_debounce_time = getTimeMs();

    if ((getTimeMs() - last_sel_debounce_time) > debounce_delay)
    {
        if (selReading != selState)
        {
            selState = selReading;

            if (selState == 0)
                return true;
        }
    }
    return false;
}

bool Inputs::scroll()
{
    scrollReading = gpio_get(SCROLL_PIN);

    if (scrollReading != lastScrollState)
        last_scroll_debounce_time = getTimeMs();

    if ((getTimeMs() - last_scroll_debounce_time) > debounce_delay)
    {
        if (scrollReading != scrollState)
        {
            scrollState = scrollReading;

            if (scrollState == 0)
                return true;
        }
    }
    return false;
}

void Inputs::updateButtons()
{
    lastScrollState = scrollReading;
    lastSelState = selReading;
}

#endif