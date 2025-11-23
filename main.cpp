/*-------------------------------------------------------------
                        C. Juice
                      December 2023
                DRSSTC_Interrupter_Firmware
                  RP2040 Embedded System
---------------------------------------------------------------*/

#include <stdio.h>
#include <bits/stdc++.h>
#include <pico/stdlib.h>
#include <pico/multicore.h>
#include "player.h"
#include "gui.h"
#include "inputs.h"
#include "util.h"
#include "transmitter.h"

Inputs inputs;
GUI gui;
Player player;

void core1_main()
{
    // player.init_transmitter();

    while (1)
    {
        if (player.play == true)
        {
            // reset transmitter
            reset_transmitter();

            // Read the Midi Header
            MidiHeader header;
            player.read_midi_header(gui.contentList[gui.current_selection], &header);

            // Try to find a track with notes (skip metadata-only tracks)
            MidiTrack track;
            bool track_found = false;
            
            for (uint32_t track_num = 0; track_num < header.tracks && !track_found; track_num++)
            {
                player.read_midi_track(gui.contentList[gui.current_selection], &track, track_num);
                
                // Check if track has actual note events by scanning for NOTE_ON (0x90) or NOTE_OFF (0x80)
                bool has_notes = false;
                for (uint32_t i = 0; i < track.length - 1; i++)
                {
                    uint8_t byte = track.data[i];
                    if ((byte & 0xF0) == 0x90 || (byte & 0xF0) == 0x80)
                    {
                        has_notes = true;
                        break;
                    }
                }
                
                if (has_notes)
                {
                    printf("Found notes in track %lu\n", track_num);
                    track_found = true;
                    player.parse_midi_track(&track);
                    player.cleanupTrackData(&track);
                }
                else
                {
                    printf("Track %lu has no notes, skipping\n", track_num);
                    player.cleanupTrackData(&track);
                }
            }

            if (!track_found)
            {
                printf("ERROR: No track with notes found\n");
            }

            reset_transmitter();
            player.resetPlayback();

            sleep_ms(100);
        }
        if (player.play == false && gui.controlMenu == false)
        {
            transmitt_off();
            sleep_ms(1);
        }
    }
}

int main(int argc, char **argv)
{
    stdio_init_all();

    gui.init();
    player.init();

    inputs.init_buttons();
    inputs.init_pots();

    transmitter_init();

    multicore_launch_core1(core1_main);

    bool displayMenu = true;
    uint16_t pot_frequency, pot_duty_cycle;
    while (true)
    {
        if (gui.controlMenu == true)
        {
            pot_frequency = inputs.read_frequency();
            pot_duty_cycle = inputs.read_dutycycle();

            set_transmitter(pot_frequency, pot_duty_cycle);

            gui.printControls();
            gui.setDuty(pot_duty_cycle);
            gui.setFreq(pot_frequency);

            if (inputs.select() == true)
            {
                // player.transmitt_off();

                if (player.mountFileSystem() == false || player.openCard() == false)
                {
                    gui.sdCardError();
                }
                else
                {
                    player.readFileNames(&gui.contentList, &gui.contentSize);

                    gui.sdMenu = true;
                    gui.controlMenu = false;
                }
            }
        }
        if (gui.sdMenu == true)
        {
            if (displayMenu == true)
            {
                gui.sdCardMenu();
                displayMenu = false;
            }
            if (inputs.scroll() == true)
            {
                gui.sdCardMenuScroll();
                displayMenu = true;
            }
            if (inputs.select() == true)
            {
                if (gui.current_selection == 0)
                {
                    gui.sdMenu = false;
                    gui.controlMenu = true;
                    displayMenu = true;

                    lcd.clear();
                }
                else
                {
                    gui.sdMenu = false;
                    gui.midiStartMenu = true;
                    gui.midiStart();
                }
            }
        }
        if (gui.midiStartMenu == true)
        {
            if (inputs.select() == true)
            {
                gui.midiGui = true;
                gui.midiStartMenu = false;
                player.paused = false;

                if (player.play)
                {
                    player.play = false;
                    transmitt_off();
                    sleep_ms(10);
                }
                player.play = true;

                lcd.clear();
            }
            if (inputs.scroll() == true)
            {
                gui.midiStartMenu = false;
                gui.sdMenu = true;
                displayMenu = true;

                lcd.clear();
            }
        }
        if (gui.midiGui == true)
        {
            gui.showMidiGui(player.paused, player.pitch, player.note_name);
            if (inputs.select() == true)
            {
                player.pause();
            }
            if (inputs.scroll() == true)
            {
                player.play = false;
                transmitt_off();
                sleep_ms(10);

                gui.midiGui = false;
                gui.sdMenu = true;
                displayMenu = true;

                gui.current_selection = 0;

                lcd.clear();
            }
            if (player.play == false && gui.sdMenu == false)
            {
                gui.midiGui = false;
                gui.sdMenu = true;
                displayMenu = true;

                gui.current_selection = 0;

                lcd.clear();
            }
        }

        inputs.updateButtons();
    }
    free(gui.contentList);

    return 0;
}