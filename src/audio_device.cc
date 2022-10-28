#include "audio_device.h"
#include <windows.h>
#include <dsound.h>

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);

void AudioDevice::Setup(void *context, int sampleRate, int outputBufferSize)
{
    samplesPerSecond = sampleRate;
    bytesPerSample = sizeof(int32_t) * 2;
    latencySampleCount = outputBufferSize;
    secondaryBufferSize = samplesPerSecond * bytesPerSample;

    HMODULE directSoundLib = LoadLibraryA("dsound.dll");
    if (!directSoundLib)
        return;

    direct_sound_create *DirectSoundCreate = (direct_sound_create *)
        GetProcAddress(directSoundLib, "DirectSoundCreate");
    
    LPDIRECTSOUND directSound;

    if (DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &directSound, 0))) {

        WAVEFORMATEX waveFormat = {};
        waveFormat.wFormatTag = WAVE_FORMAT_PCM;
        waveFormat.nChannels = 2;
        waveFormat.nSamplesPerSec = samplesPerSecond;
        waveFormat.wBitsPerSample = 32;
        waveFormat.nBlockAlign = (waveFormat.nChannels * waveFormat.wBitsPerSample) / 8;
        waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
        waveFormat.cbSize = 0;

        HWND hwnd = (HWND)context;
        if (SUCCEEDED(directSound->SetCooperativeLevel(hwnd, DSSCL_PRIORITY))) {
            DSBUFFERDESC bufferDesc = {};
            bufferDesc.dwSize = sizeof(bufferDesc);
            bufferDesc.dwFlags = DSBCAPS_PRIMARYBUFFER;
            bufferDesc.lpwfxFormat = &waveFormat;
            
            directSound->CreateSoundBuffer(&bufferDesc, &((LPDIRECTSOUNDBUFFER)primaryBuffer), 0);
        }

        DSBUFFERDESC bufferDesc = {};
        bufferDesc.dwSize = sizeof(bufferDesc);
        bufferDesc.dwFlags = DSBCAPS_GETCURRENTPOSITION2;
        bufferDesc.dwBufferBytes = secondaryBufferSize;
        bufferDesc.lpwfxFormat = &waveFormat;

        directSound->CreateSoundBuffer(&bufferDesc, &((LPDIRECTSOUNDBUFFER)secondaryBuffer), 0);

        auto buffer = (LPDIRECTSOUNDBUFFER)secondaryBuffer;
        buffer->Play(0, 0, DSBPLAY_LOOPING);
    }
}

void AudioDevice::ClearBuffer()
{
    VOID *region1;
    DWORD region1Size;
    VOID *region2;
    DWORD region2Size;

    auto buffer = (LPDIRECTSOUNDBUFFER)secondaryBuffer;
    if (SUCCEEDED(buffer->Lock(0, secondaryBufferSize, 
        &region1, &region1Size, &region2, &region2Size, 0)))
    {
        uint8_t *dst = (uint8_t *)region1;
        for (uint64_t i = 0; i < region1Size; ++i) {
            *dst++ = 0;
        }

        dst = (uint8_t *)region2;
        for (uint64_t i = 0; i < region2Size; ++i) {
            *dst++ = 0;
        }

        buffer->Unlock(region1, region1Size, region2, region2Size);
    }
}

void AudioDevice::FillBuffer(float *samples, uint32_t numSamples)
{
    VOID *region1;
    DWORD region1Size;
    VOID *region2;
    DWORD region2Size;

    DWORD offset = (runningSampleIndex * bytesPerSample) % secondaryBufferSize;
    DWORD bytes = numSamples * bytesPerSample;

    auto buffer = (LPDIRECTSOUNDBUFFER)secondaryBuffer;
    if (SUCCEEDED(buffer->Lock(offset, bytes,
        &region1, &region1Size, &region2, &region2Size, 0)))
    {
        uint64_t region1SampleCount = region1Size / bytesPerSample;
        int32_t *dst = (int32_t *)region1;
        float *src = (float *)samples;
        for (uint64_t i = 0; i < region1SampleCount; ++i) {
            *dst++ = (int32_t)(*src++ * INT32_MAX);
            *dst++ = (int32_t)(*src++ * INT32_MAX);
        }

        uint64_t region2SampleCount = region2Size / bytesPerSample;
        dst = (int32_t *)region2;
        for (uint64_t i = 0; i < region2SampleCount; ++i) {
            *dst++ = (int32_t)(*src++ * INT32_MAX);
            *dst++ = (int32_t)(*src++ * INT32_MAX);
        }

        runningSampleIndex += region1SampleCount;
        runningSampleIndex += region2SampleCount;

        buffer->Unlock(region1, region1Size, region2, region2Size);
    }
}

uint32_t AudioDevice::RemainingSamples()
{
    uint32_t bytes = 0;
    DWORD playCursor = 0;
    DWORD writeCursor = 0;
    
    auto buffer = (LPDIRECTSOUNDBUFFER)secondaryBuffer;
    if (SUCCEEDED(buffer->GetCurrentPosition(&playCursor, &writeCursor))) {
        DWORD offset = (runningSampleIndex * bytesPerSample) % secondaryBufferSize;
        DWORD target = (playCursor + (latencySampleCount * bytesPerSample)) % secondaryBufferSize;

        if (offset > target) {
            bytes = secondaryBufferSize - offset;
            bytes += target;
        } else {
            bytes = target - offset;
        }
    }

    return bytes / bytesPerSample;
}
