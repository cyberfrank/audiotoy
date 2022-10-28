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
    
    AudioDevice *device = &audioDeviceInst;
    device->Setup(hwnd, 96000, 4096);
    device->ClearBuffer();
    
    float *samples = (float *)malloc(device->samplesPerSecond * sizeof(float) * 2);
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

        int samplesPerSecond = device->samplesPerSecond;
        int sampleCount = device->RemainingSamples();

        // Test tone
        int toneHz = 440;
        float wavePeriod = (float)samplesPerSecond / toneHz;
        float volume = 0.5f;
        float *p = samples;
        for (int i = 0; i < sampleCount; ++i) {
            float value = sinf(t) * volume;
            *p++ = value;
            *p++ = value;
            t += 2.0f * PI * 1.0f / wavePeriod;
        }

        device->FillBuffer(samples, sampleCount);
    }

    printf("Closing...\n");
    return 0;
}