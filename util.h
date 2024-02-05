#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>
#include <chrono>
#include <pico/stdlib.h>

int map(int v, int a1, int a2, int b1, int b2)
{
    return b1 + (v - a1) * (b2 - b1) / (a2 - a1);
}

unsigned long millis()
{
    auto now = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
}

#endif