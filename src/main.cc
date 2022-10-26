#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <windows.h>

#include "audio_device.h"

#define PI 3.14159265359f

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LRESULT lResult = 0;
    switch (msg) {
        case WM_CLOSE: {
            PostQuitMessage(0);
            break;
        }
        default: 
            lResult = DefWindowProc(hwnd, msg, wParam, lParam);
            break;
    }
    return lResult;
}

static HWND Win32CreateWindow(int w, int h)
{
    WNDCLASSA windowClass = {};
    windowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    windowClass.lpfnWndProc = WndProc;
    windowClass.lpszClassName = "AudioToyWindowClass";

    if (!RegisterClassA(&windowClass)) {
        return 0;
    }

    HWND window = CreateWindowExA(
        0,
        windowClass.lpszClassName,
        "Audio Toy",
        WS_OVERLAPPEDWINDOW|WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT,
        w, h,
        0,
        0,
        0,
        0
    );

    return window;
}

int main()
{
    AudioDevice audioDeviceInst;
    bool running = true;

    HWND hwnd = Win32CreateWindow(800, 600);

    AudioOutput audioOut = {};
    audioOut.samplesPerSecond = 44100;
    audioOut.bytesPerSample = sizeof(int16_t) * 2;
    audioOut.latencySampleCount = 4096;
    
    AudioDevice *device = &audioDeviceInst;
    device->Setup(hwnd, &audioOut);
    device->ClearBuffer();
    
    uint64_t size = audioOut.samplesPerSecond * audioOut.bytesPerSample;
    int16_t *samples = (int16_t *)malloc(size);
    float t = 0;

    while (running) {
        MSG msg;
        while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                running = false;
            }
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }

        int samplesPerSecond = audioOut.samplesPerSecond;
        int sampleCount = device->RemainingSamples();

        // Test tone
        int toneHz = 800;
        float wavePeriod = (float)samplesPerSecond / toneHz;
        float volume = 3000;
        int16_t *p = samples;
        for (int i = 0; i < sampleCount; ++i) {
            float value = sinf(t) * volume;
            *p++ = (int16_t)value;
            *p++ = (int16_t)value;
            t += 2.0f * PI * 1.0f / wavePeriod;
        }

        device->FillBuffer(samples, sampleCount);
    }

    printf("Closing...\n");
    return 0;
}