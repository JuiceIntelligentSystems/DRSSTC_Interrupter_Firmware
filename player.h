#ifndef PLAYER_H
#define PLAYER_H

#include <stdio.h>
#include <pico/stdlib.h>
#include <pico/util/datetime.h>
#include <hardware/pwm.h>
#include <hardware/clocks.h>
#include <pico/float.h>
#include <math.h>
#include "f_util.h"
#include "hw_config.h"
#include "ff.h"
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

    uint32_t current_tempo = 500000; // Default to 120bpm
    uint16_t time_division;

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

    uint32_t delta_to_ms(uint32_t delta);

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
    void resetPlayback(void);
    void cleanupTrackData(MidiTrack *);
    void resetFileSystem();
};

bool Player::init()
{
    if (sd_init_driver())
        return true;

    return false;
}

bool Player::mountFileSystem()
{
    fr = f_mount(&fs, "0:", 1);

    if (fr == FR_OK)
        return true;

    return false;
}

bool Player::unmountCard()
{
    fr = f_unmount("0:");

    if (fr == FR_OK)
        return true;

    return false;
}

bool Player::openCard()
{
    fr = f_opendir(&dir, "0:/");

    if (fr == FR_OK)
        return true;

    return false;
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

    // Read and skip "MThd" chunk identifier (4 bytes)
    char chunk_id[4];
    f_read(&fil, chunk_id, 4, NULL);

    // Read and skip chunk size (4 bytes)
    uint32_t chunk_size;
    f_read(&fil, &chunk_size, 4, NULL);

    // Read actual header (6 bytes) for format(2) number of tracks(2) and deltaTime(2)
    uint8_t header_data[6];
    f_read(&fil, header_data, 6, NULL);

    // Parse header in big-endian format (MIDI uses big-endian)
    header->format = (header_data[0] << 8) | header_data[1];
    header->tracks = (header_data[2] << 8) | header_data[3];
    header->division = (header_data[4] << 8) | header_data[5];

    time_division = header->division; // store time division

    printf("MIDI Header - Format: %u, Tracks: %u, Division: %u\n",
           header->format, header->tracks, header->division);

    f_close(&fil);
}

void Player::read_midi_track(const char *file_name, MidiTrack *track, uint32_t track_number)
{
    // Open midi file for reading
    fr = f_open(&fil, file_name, FA_READ);
    if (fr != FR_OK)
    {
        printf("ERROR: Failed to open file\n");
        f_unmount("0:");
        return;
    }

    // Read header to get actual header size
    f_lseek(&fil, 0);

    char chunk_id[4];
    uint32_t chunk_size;
    UINT bytes_read;

    // Read "MThd"
    f_read(&fil, chunk_id, 4, &bytes_read);
    if (bytes_read != 4)
    {
        printf("ERROR: Failed to read MThd chunk ID\n");
        f_close(&fil);
        f_unmount("0:");
        return;
    }

    f_read(&fil, &chunk_size, 4, &bytes_read);
    if (bytes_read != 4)
    {
        printf("ERROR: Failed to read MThd chunk size\n");
        f_close(&fil);
        f_unmount("0:");
        return;
    }

    // Convert chunk size from big-endian
    chunk_size = ((chunk_size >> 24) & 0xFF) |
                 ((chunk_size >> 8) & 0xFF00) |
                 ((chunk_size << 8) & 0xFF0000) |
                 ((chunk_size << 24) & 0xFF000000);

    printf("Header chunk size: %lu\n", chunk_size);

    // Current position should be at 8, skip the actual header data
    // New position = 8 + chunk_size
    f_lseek(&fil, 8 + chunk_size);

    // Now find the desired track by searching for "MTrk" chunks
    uint32_t tracks_found = 0;

    while (1)
    {
        uint32_t chunk_start = f_tell(&fil);

        // Read chunk ID
        f_read(&fil, chunk_id, 4, &bytes_read);
        if (bytes_read != 4)
        {
            printf("ERROR: Failed to read chunk ID at position %lu\n", chunk_start);
            f_close(&fil);
            f_unmount("0:");
            return;
        }

        // Read chunk size
        f_read(&fil, &chunk_size, 4, &bytes_read);
        if (bytes_read != 4)
        {
            printf("ERROR: Failed to read chunk size at position %lu\n", chunk_start + 4);
            f_close(&fil);
            f_unmount("0:");
            return;
        }

        // Convert chunk size from big-endian
        uint32_t chunk_size_le = ((chunk_size >> 24) & 0xFF) |
                                 ((chunk_size >> 8) & 0xFF00) |
                                 ((chunk_size << 8) & 0xFF0000) |
                                 ((chunk_size << 24) & 0xFF000000);

        printf("Found chunk at %lu: %c%c%c%c, size: %lu\n",
               chunk_start, chunk_id[0], chunk_id[1], chunk_id[2], chunk_id[3], chunk_size_le);

        // Check if this is an MTrk chunk
        bool is_mtrk = (chunk_id[0] == 'M' && chunk_id[1] == 'T' &&
                        chunk_id[2] == 'r' && chunk_id[3] == 'k');

        if (is_mtrk)
        {
            // Found an MTrk chunk
            if (tracks_found == track_number)
            {
                // This is the track we want
                track->length = chunk_size_le;

                // Safety check
                if (track->length == 0 || track->length > 1048576)
                {
                    printf("ERROR: Invalid track length: %lu\n", track->length);
                    f_close(&fil);
                    f_unmount("0:");
                    return;
                }

                // Allocate memory for the track data
                track->data = (uint8_t *)malloc(track->length);
                if (track->data == NULL)
                {
                    printf("ERROR: Failed to allocate %lu bytes for track data\n", track->length);
                    f_close(&fil);
                    f_unmount("0:");
                    return;
                }

                // Read track data
                fr = f_read(&fil, track->data, track->length, &bytes_read);
                if (bytes_read != track->length)
                {
                    printf("ERROR: Failed to read track data. Read %u of %lu bytes\n", bytes_read, track->length);
                    free(track->data);
                    track->data = NULL;
                    f_close(&fil);
                    f_unmount("0:");
                    return;
                }

                printf("Successfully read track %lu with length %lu\n", track_number, track->length);
                f_close(&fil);
                return;
            }
            tracks_found++;
        }

        // Move to next chunk
        // Current position is after the chunk size field, so we need to skip chunk_size_le bytes
        f_lseek(&fil, chunk_start + 8 + chunk_size_le);

        // Safety check to prevent infinite loop
        if (f_eof(&fil))
        {
            printf("ERROR: Reached end of file before finding track %lu\n", track_number);
            f_close(&fil);
            f_unmount("0:");
            return;
        }
    }
}

void Player::parse_midi_track(const MidiTrack *track)
{
    const uint8_t *data = track->data;
    uint32_t offset = 0;
    uint32_t event_count = 0;
    uint8_t running_status = 0;

    printf("Starting MIDI playback, track length: %lu\n", track->length);

    while (offset < track->length && play == true)
    {
        uint32_t delta_time = 0;
        uint8_t value;

        // read variable length delta time
        do
        {
            if (offset >= track->length)
            {
                printf("ERROR: Offset exceeded track length during delta time read\n");
                play = false;
                return;
            }
            value = data[offset];
            offset++;
            delta_time = (delta_time << 7) | (value & 0x7F);
        } while (value & 0x80);

        // Convert it to ms
        uint32_t wait_time = delta_to_ms(delta_time);

        if (offset >= track->length)
        {
            printf("ERROR: Offset exceeded track length after delta time read\n");
            play = false;
            return;
        }

        // Read MIDI Event type
        uint8_t status_byte = data[offset];

        // Handle running status
        if ((status_byte & 0x80) == 0)
        {
            // High bit not set - this is data, not a status byte
            // Use running status from previous event
            if (running_status == 0)
            {
                printf("ERROR: Running status but no previous status at offset %lu\n", offset);
                play = false;
                return;
            }
            status_byte = running_status;
            // Don't increment offset - we'll use this byte as data
        }
        else
        {
            // This is a real status byte
            offset++;
            running_status = status_byte;

            // Meta events and SysEx don't use running status
            if (status_byte == MIDI_META_EVENT || status_byte == 0xF0 || status_byte == 0xF7)
            {
                running_status = 0;
            }
        }

        printf("Event %lu: offset=%lu, delta=%lu, wait_time=%lu, status=0x%02X\n",
               event_count, offset - 1, delta_time, wait_time, status_byte);

        // Check whether note-on or note-off
        if ((status_byte & 0xF0) == MIDI_NOTE_ON || (status_byte & 0xF0) == MIDI_NOTE_OFF)
        {
            if (offset + 2 > track->length)
            {
                printf("ERROR: Not enough bytes for note event\n");
                play = false;
                return;
            }

            uint8_t channel = status_byte & 0x0F;
            uint8_t note = data[offset];
            uint8_t velocity = data[offset + 1];
            offset += 2;

            printf("  NOTE %s: note=%u, velocity=%u, channel=%u\n",
                   (status_byte & 0xF0) == MIDI_NOTE_ON ? "ON" : "OFF", note, velocity, channel);

            // Register the output only on note on event
            if (velocity > 0)
            {
                note_name = getNoteName(note);
                pitch = velocity;
            }

            transmitt_music(note, velocity);
            
            // Wait for duration of note
            while (paused)
            {
                transmitt_off();
                sleep_ms(10);
            }
            sleep_ms(wait_time);
        }
        else if (status_byte == MIDI_META_EVENT)
        {
            if (offset + 2 > track->length)
            {
                printf("ERROR: Not enough bytes for meta event\n");
                play = false;
                return;
            }

            uint8_t meta_type = data[offset];
            uint8_t meta_length = data[offset + 1];
            offset += 2;

            printf("  META event: type=0x%02X, length=%u\n", meta_type, meta_length);

            if (offset + meta_length > track->length)
            {
                printf("ERROR: Not enough bytes for meta event data\n");
                play = false;
                return;
            }

            // Handle tempo meta event
            if (meta_type == 0x51 && meta_length == 3)
            {
                current_tempo = (data[offset] << 16) | (data[offset + 1] << 8) | (data[offset + 2]);
                printf("    Tempo updated: %lu microseconds per beat\n", current_tempo);
            }

            offset += meta_length;

            // Still need to wait (even for meta events)
            // sleep_ms(wait_time);
        }
        else
        {
            printf("  Other event: 0x%02X\n", status_byte);

            // Other MIDI Events
            if ((status_byte & 0xF0) == 0xC0 || (status_byte & 0xF0) == 0xD0)
            {
                if (offset + 1 > track->length)
                {
                    printf("ERROR: Not enough bytes for program/channel event\n");
                    play = false;
                    return;
                }
                offset += 1;
            }
            else
            {
                if (offset + 2 > track->length)
                {
                    printf("ERROR: Not enough bytes for 2-byte event\n");
                    play = false;
                    return;
                }
                offset += 2;
            }
            // sleep_ms(wait_time);
        }

        event_count++;
    }

    printf("MIDI playback finished. Events processed: %lu\n", event_count);
    play = false;
}

const char *Player::getNoteName(uint8_t note_value)
{
    if (note_value < 128)
        return note_names[note_value];

    return "NA";
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

void Player::resetPlayback(void)
{
    play = false;
    paused = false;
    pitch = 0;
    note_name = nullptr;

    resetFileSystem();
    reset_transmitter();
}

void Player::cleanupTrackData(MidiTrack *track)
{
    if (track && track->data)
    {
        free(track->data);
        track->data = nullptr;
        track->length = 0;
    }
}

void Player::resetFileSystem()
{
    // Close Files and Directory
    f_close(&fil);
    f_closedir(&dir);

    // Unmount and remount card
    f_unmount("0:");
    f_mount(&fs, "0:", 1);
}

uint32_t Player::delta_to_ms(uint32_t delta)
{
    return (delta * current_tempo) / (time_division * 1000);
}

#endif