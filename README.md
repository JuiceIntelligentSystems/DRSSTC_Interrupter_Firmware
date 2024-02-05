The firmware for the Dual-Resonant-Solid-State-Tesla-Coil inturrupter board. It controls the tesla coil via a pwm signal generated at the fiber optic transmitter given the duty cycle and frequency potentiometers and midi files loaded onto teh SD Card.
It uses the NO_OS_FATFS library found at: https://github.com/carlk3/no-OS-FatFS-SD-SPI-RPi-Pico by carlk3. 

FEATURES
-
- Fiber Optic Transmitter
- 4x20 LCD
- SEL and SCROLL Buttons for user input
- FREQ and DUTY_CYCLE Pots for manual control
- Micro SD Card for loading midi music files

USAGE
- 
- When turned on the screen goes directly to pwm mode where the user may adjust the pots to control the pwm
- If in the pwm screen and the user presses the SEL button, then the sd card menu shows and you can select the midi file that you want to play
- While playing, if the user presses the SEL button, the music pauses and when the user presses SCROLL the player quits and the output is turned off
- The music frequency is between 32Hz and 1kHz
- The control frequency is between 15Hz and 1kHz
