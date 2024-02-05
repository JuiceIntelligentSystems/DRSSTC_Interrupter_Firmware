#ifndef PLAYER_H
#define PLAYER_H

#include <stdio.h>
#include <pico/stdlib.h>
#include <pico/util/datetime.h>
#include <hardware/pwm.h>
#include <hardware/clocks.h>
#include <pico/float.h>
#include <math.h>
#include "ff.h"
#include "sd_card.h"
#include "util.h"
#include "transmitter.h"

#define MIDI_NOTE_ON 0x90
#define MIDI_NOTE_OFF 0x80
#define MIDI_META_EVENT 0xFF

typedef struct
{
    uint8_t format;
    uint16_t tracks;
    uint16_t division;
} MidiHeader;

typedef struct
{
    uint32_t length;
    uint8_t *data;
} MidiTrack;

class Player
{
private:
    FRESULT fr;
    FATFS fs;
    DIR dir;
    FILINFO fno;
    FIL fil;

    // Lookup table for all notes and octaves
    const char *note_names[129] = {
        "C-1 ", "C#-1", "D-1 ", "D#-1", "E-1 ", "F-1 ", "F#-1", "G-1 ", "G#-1", "A-1 ", "A#-1", "B-1 ",
        "C0  ", "C#0 ", "D0  ", "D#0 ", "E0  ", "F0  ", "F#0 ", "G0  ", "G#0 ", "A0  ", "A#0 ", "B0  ",
        "C1  ", "C#1 ", "D1  ", "D#1 ", "E1  ", "F1  ", "F#1 ", "G1  ", "G#1 ", "A1  ", "A#1 ", "B1  ",
        "C2  ", "C#2 ", "D2  ", "D#2 ", "E2  ", "F2  ", "F#2 ", "G2  ", "G#2 ", "A2  ", "A#2 ", "B2  ",
        "C3  ", "C#3 ", "D3  ", "D#3 ", "E3  ", "F3  ", "F#3 ", "G3  ", "G#3 ", "A3  ", "A#3 ", "B3  ",
        "C4  ", "C#4 ", "D4  ", "D#4 ", "E4  ", "F4  ", "F#4 ", "G4  ", "G#4 ", "A4  ", "A#4 ", "B4  ",
        "C5  ", "C#5 ", "D5  ", "D#5 ", "E5  ", "F5  ", "F#5 ", "G5  ", "G#5 ", "A5  ", "A#5 ", "B5  ",
        "C6  ", "C#6 ", "D6  ", "D#6 ", "E6  ", "F6  ", "F#6 ", "G6  ", "G#6 ", "A6  ", "A#6 ", "B6  ",
        "C7  ", "C#7 ", "D7  ", "D#7 ", "E7  ", "F7  ", "F#7 ", "G7  ", "G#7 ", "A7  ", "A#7 ", "B7  ",
        "C8  ", "C#8 ", "D8  ", "D#8 ", "E8  ", "F8  ", "F#8 ", "G8  ", "G#8 ", "A8  ", "A#8 ", "B8  ",
        "C9  ", "C#9 ", "D9  ", "D#9 ", "E9  ", "F9  ", "F#9 ", "G9  ", "G#9 "};

public:
    const char *note_name;
    int pitch = 0;
    bool play = false;
    bool paused = false;

    bool init();
    bool mountFileSystem();
    bool unmountCard();
    bool openCard();
    void readFileNames(const char ***, int *);
    void read_midi_header(const char *, MidiHeader *);
    void read_midi_track(const char *, MidiTrack *, uint32_t);
    void parse_midi_track(const MidiTrack *);
    const char *readFile(const char *);
    void pause();
    const char *getNoteName(uint8_t);
};

bool Player::init()
{
    if (sd_init_driver())
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool Player::mountFileSystem()
{
    fr = f_mount(&fs, "0:", 1);
    if (fr == FR_OK)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool Player::unmountCard()
{
    fr = f_unmount("0:");
    if (fr == FR_OK)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool Player::openCard()
{
    fr = f_opendir(&dir, "0:/");
    if (fr == FR_OK)
    {
        return true;
    }
    else
    {
        return false;
    }
}

void Player::readFileNames(const char ***files, int *file_count)
{
    // Count number of files in directory
    int count = 0;
    while (1)
    {
        fr = f_readdir(&dir, &fno);
        if (fr != FR_OK || fno.fname[0] == 0)
            break;
        if (fno.fattrib & AM_DIR)
            continue;
        count++;
    }

    // Allocate memory for array
    *files = (const char **)malloc((count + 1) * sizeof(const char *));
    if (*files == NULL)
    {
        f_closedir(&dir);
        f_unmount("0:");
        return;
    }

    // Make it so the element 'back' can be selected
    (*files)[0] = strdup("Back");

    // Rewind directory to read file names
    f_readdir(&dir, NULL);

    // Read names and store them
    int i = 1;
    while (i <= count)
    {
        fr = f_readdir(&dir, &fno);
        if (fr != FR_OK || fno.fname[0] == 0)
            break;
        if (fno.fattrib & AM_DIR)
            continue;

        (*files)[i] = strdup(fno.fname);
        i++;
    }

    *file_count = count + 1;
    f_closedir(&dir);
}

void Player::pause()
{
    paused = !paused;
}

void Player::read_midi_header(const char *file_name, MidiHeader *header)
{
    // Open Midi File for reading
    fr = f_open(&fil, file_name, FA_READ);
    if (fr != FR_OK)
    {
        f_unmount("0:");
        return;
    }

    // Read header
    f_read(&fil, header, sizeof(MidiHeader), NULL);

    f_close(&fil);
}

void Player::read_midi_track(const char *file_name, MidiTrack *track, uint32_t track_number)
{
    // Open midi file for reading
    fr = f_open(&fil, file_name, FA_READ);
    if (fr != FR_OK)
    {
        f_unmount("0:");
        return;
    }

    // Skip header chunk
    f_lseek(&fil, sizeof(MidiHeader));

    // Skip to specified track
    for (uint32_t i = 0; i < track_number; i++)
    {
        uint32_t track_length;
        f_read(&fil, &track_length, sizeof(uint32_t), NULL);
        f_lseek(&fil, f_tell(&fil) + track_length);
    }

    // Read track chunk length
    f_read(&fil, &track->length, sizeof(uint32_t), NULL);

    // Allocate memory for the track data
    track->data = (uint8_t *)malloc(sizeof(track->length));
    if (track->data == NULL)
    {
        f_close(&fil);
        f_unmount("0:");
        return;
    }

    // Read track data
    f_read(&fil, track->data, track->length, NULL);

    f_close(&fil);
}

void Player::parse_midi_track(const MidiTrack *track)
{
    const uint8_t *data = track->data;
    uint32_t offset = 0;

    while (offset < track->length && play == true)
    {
        uint32_t delta_time = 0;
        uint8_t value;

        do
        {
            value = *data++;
            delta_time = (delta_time << 7) | (value & 0x7F);
        } while (value & 0x80);

        offset += delta_time;

        // Read MIDI Event type
        uint8_t status_byte = *data++;

        // Check whether note-on or note-off
        // I spent way too long fixing bugs when all I had to do was change "*" to "&"
        // raaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
        if ((status_byte & 0xF0) == MIDI_NOTE_ON || (status_byte & 0xF0) == MIDI_NOTE_OFF)
        {
            uint8_t channel = status_byte & 0x0F;
            uint8_t note = *data++;
            uint8_t velocity = *data++;

            if ((status_byte & 0xF0) == MIDI_NOTE_ON && velocity > 0)
            {
                // TODO: DISPLAY NOTE AND VELOCITY on LCD
                note_name = getNoteName(note);
                pitch = velocity;

                // transmitt_music(note, velocity);

                // Wait for duration of note
                while (paused)
                {
                    transmitt_off();
                    sleep_ms(10);
                }
                sleep_ms(delta_time);
            }
            else
            {
                // TODO: DISPLAY NOTE AND VELOCITY on LCD
                note_name = getNoteName(note);
                pitch = velocity;

                // transmitt_music(note, 0);

                // Wait for duration of note
                while (paused)
                {
                    transmitt_off();
                    sleep_ms(10);
                }
                sleep_ms(delta_time);
            }
            transmitt_music(note, velocity);
        }
        else if (status_byte == MIDI_META_EVENT)
        {
            uint8_t meta_type = *data++;
            uint8_t meta_length = *data++;

            // Print Meta event Data
        }
        else
        {
            // Print other Midi events
        }
    }
}

const char *Player::getNoteName(uint8_t note_value)
{
    if (note_value < 128)
    {
        return note_names[note_value];
    }
    else
    {
        return "NA";
    }
}

const char *Player::readFile(const char *fileName)
{
    // Open file for reading
    fr = f_open(&fil, fileName, FA_READ);
    if (fr != FR_OK)
    {
        f_unmount("0:");
        return NULL;
    }

    // Get size of file
    UINT file_size = f_size(&fil);

    // Allocate memeory for file content
    char *file_content = (char *)malloc(sizeof(file_size + 1));
    if (file_content == NULL)
    {
        f_close(&fil);
        f_unmount("0:");
        return NULL;
    }

    // Read Content
    UINT bytes_read;
    fr = f_read(&fil, file_content, file_size, &bytes_read);
    if (fr != FR_OK)
    {
        free(file_content);
        f_close(&fil);
        f_unmount("0:");
        return NULL;
    }

    file_content[file_size] = '\0';

    return file_content;
}

#endif