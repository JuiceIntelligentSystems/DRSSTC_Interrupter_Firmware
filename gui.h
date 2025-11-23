#ifndef GUI_H
#define GUI_H

#include <stdio.h>
#include <pico/stdio.h>
#include <chrono>
#include <string.h>
#include "lcd.h"
#include "util.h"

#define LCD_D4 20
#define LCD_D5 21
#define LCD_D6 22
#define LCD_D7 23
#define LCD_RS 14
#define LCD_E 15
#define LCD_COLS 20
#define LCD_ROWS 4

LCD lcd(LCD_D4, LCD_D5, LCD_D6, LCD_D7, LCD_RS, LCD_E, LCD_COLS, LCD_ROWS);

class GUI
{
private:
    const char *value[19] = {"                  ",
                             "\xff                 ",
                             "\xff\xff                ",
                             "\xff\xff\xff               ",
                             "\xff\xff\xff\xff              ",
                             "\xff\xff\xff\xff\xff             ",
                             "\xff\xff\xff\xff\xff\xff            ",
                             "\xff\xff\xff\xff\xff\xff\xff           ",
                             "\xff\xff\xff\xff\xff\xff\xff\xff          ",
                             "\xff\xff\xff\xff\xff\xff\xff\xff\xff         ",
                             "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff        ",
                             "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff       ",
                             "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff      ",
                             "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff     ",
                             "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff    ",
                             "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff   ",
                             "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff  ",
                             "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff ",
                             "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"};

public:
    const char **contentList;
    int contentSize = 0;

    // const char *contentList[6] = {"Test1", "Test2", "Test3", "Test4", "Test5", "Test6"};
    // int contentSize = sizeof(contentList) / sizeof(contentList[0]);
    int current_selection = 0;

    bool controlMenu = true;
    bool sdMenu = false;
    bool midiStartMenu = false;
    bool midiGui = false;

    bool lcd_cleared = false;

    void init();
    void printControls();
    void setDuty(uint16_t);
    void setFreq(uint16_t);
    void sdCardMenu();
    void sdCardError();
    void sdCardMenuScroll();
    void midiStart();
    void showMidiGui(bool, int, const char *);
};

void GUI::init()
{
    lcd.init();
    lcd.clear();
    lcd.goto_pos(0, 0);
}

void GUI::printControls()
{
    lcd.goto_pos(6, 0);
    lcd.print("Frequency");

    lcd.goto_pos(0, 1);
    lcd.print("[");
    lcd.goto_pos(19, 1);
    lcd.print("]");

    lcd.goto_pos(5, 2);
    lcd.print("Pulse Width");

    lcd.goto_pos(0, 3);
    lcd.print("[");
    lcd.goto_pos(19, 3);
    lcd.print("]");
}

void GUI::setDuty(uint16_t rawDutyPot)
{
    int mappedPot = map(rawDutyPot, 0, 4095, 1, 19);

    int previousValue = mappedPot;
    int currentValue;
    if (previousValue != currentValue)
    {
        lcd.goto_pos(1, 1);
        lcd.print(value[mappedPot]);
    }
    currentValue = previousValue;
}

void GUI::setFreq(uint16_t rawFreqPot)
{
    int mappedPot = map(rawFreqPot, 0, 4095, 1, 19);

    int previousValue = mappedPot;
    int currentValue;
    if (previousValue != currentValue)
    {
        lcd.goto_pos(1, 3);
        lcd.print(value[mappedPot]);
    }
    currentValue = previousValue;
}

void GUI::sdCardError()
{
    lcd.clear();
    lcd.goto_pos(2, 1);
    lcd.print("No SD Card Found");

    sleep_ms(5000);
}

void GUI::sdCardMenu()
{
    lcd.clear();

    lcd.goto_pos(0, 0);
    lcd.print("Select a Song:");

    int visible_items = 3;
    int start_index = current_selection - (visible_items - 1);
    start_index = (start_index < 0) ? 0 : start_index;

    int end_index = start_index + visible_items;
    end_index = (end_index > contentSize) ? contentSize : end_index;

    for (int i = start_index; i < end_index; i++)
    {
        lcd.goto_pos(0, i - start_index + 1);
        if (i == current_selection)
        {
            lcd.print(">");
        }
        else
        {
            lcd.print(" ");
        }
        lcd.print(contentList[i]);
        // free((void*)contentList[i]);
    }
}

void GUI::sdCardMenuScroll()
{
    current_selection = (current_selection + 1) % contentSize;
}

void GUI::midiStart()
{
    lcd.clear();
    lcd.goto_pos(0, 0);

    lcd.print("You want to play");
    lcd.goto_pos(0, 1);
    lcd.print(contentList[current_selection]);
    lcd.print("?");

    lcd.goto_pos(0, 2);
    lcd.print("SEL to continue or");
    lcd.goto_pos(0, 3);
    lcd.print("SCROLL to go back");
}

void GUI::showMidiGui(bool paused, int velocity, const char *note)
{
    std::string vel_string = std::to_string(velocity);

    lcd.goto_pos(0, 0);
    lcd.print("Now Playing:");
    lcd.goto_pos(0, 1);
    lcd.print(contentList[current_selection]);

    lcd.goto_pos(0, 2);
    lcd.print("Note: ");
    lcd.print(note);
    lcd.print(" Vel: ");
    lcd.print(vel_string.c_str());

    if (paused == true)
    {
        lcd.goto_pos(0, 3);
        lcd.print("       PAUSED       ");
    }
    else
    {
        lcd.goto_pos(0, 3);
        lcd.print("                    ");
    }
}

#endif