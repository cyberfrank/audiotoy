#include "audio_device.h"
#include <windows.h>
#include <dsound.h>

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);

void AudioDevice::Setup(void *context, AudioOutput *audioOut)
{
    this->output = audioOut;

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
        waveFormat.nSamplesPerSec = output->samplesPerSecond;
        waveFormat.wBitsPerSample = 16;
        waveFormat.nBlockAlign = (waveFormat.nChannels * waveFormat.wBitsPerSample) / 8;
        waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
        waveFormat.cbSize = 0;

        HWND hwnd = (HWND)context;
        if (SUCCEEDED(directSound->SetCooperativeLevel(hwnd, DSSCL_PRIORITY))) {
            DSBUFFERDESC bufferDesc = {};
            bufferDesc.dwSize = sizeof(bufferDesc);
            bufferDesc.dwFlags = DSBCAPS_PRIMARYBUFFER;

            LPDIRECTSOUNDBUFFER primaryBuffer;
            directSound->CreateSoundBuffer(&bufferDesc, &primaryBuffer, 0);
            primaryBuffer->SetFormat(&waveFormat);
        }

        DSBUFFERDESC bufferDesc = {};
        bufferDesc.dwSize = sizeof(bufferDesc);
        bufferDesc.dwFlags = DSBCAPS_GETCURRENTPOSITION2;
        bufferDesc.dwBufferBytes = output->samplesPerSecond * output->bytesPerSample;
        bufferDesc.lpwfxFormat = &waveFormat;
        
        globalSecondaryBuffer = malloc(sizeof(LPDIRECTSOUNDBUFFER));
        directSound->CreateSoundBuffer(&bufferDesc, (LPDIRECTSOUNDBUFFER *)globalSecondaryBuffer, 0);

        auto secondaryBuffer = *(LPDIRECTSOUNDBUFFER *)globalSecondaryBuffer;
        secondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);
    }
}

void AudioDevice::ClearBuffer()
{
    VOID *region1;
    DWORD region1Size;
    VOID *region2;
    DWORD region2Size;

    DWORD outputBufferSize = output->samplesPerSecond * output->bytesPerSample;

    auto secondaryBuffer = *(LPDIRECTSOUNDBUFFER *)globalSecondaryBuffer;
    if (SUCCEEDED(secondaryBuffer->Lock(0, outputBufferSize, 
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

        secondaryBuffer->Unlock(region1, region1Size, region2, region2Size);
    }
}

void AudioDevice::FillBuffer(int16_t *samples, uint32_t num_samples)
{
    VOID *region1;
    DWORD region1Size;
    VOID *region2;
    DWORD region2Size;

    DWORD outputBufferSize = output->samplesPerSecond * output->bytesPerSample;
    DWORD offset = (output->runningSampleIndex * output->bytesPerSample) % outputBufferSize;
    DWORD bytes = num_samples * output->bytesPerSample;

    auto secondaryBuffer = *(LPDIRECTSOUNDBUFFER *)globalSecondaryBuffer;
    if (SUCCEEDED(secondaryBuffer->Lock(offset, bytes,
        &region1, &region1Size, &region2, &region2Size, 0)))
    {
        uint64_t region1SampleCount = region1Size / output->bytesPerSample;
        int16_t *dst = (int16_t *)region1;
        int16_t *src = (int16_t *)samples;
        for (uint64_t i = 0; i < region1SampleCount; ++i) {
            *dst++ = *src++;
            *dst++ = *src++;
            ++output->runningSampleIndex;
        }

        uint64_t region2SampleCount = region2Size / output->bytesPerSample;
        dst = (int16_t *)region2;
        for (uint64_t i = 0; i < region2SampleCount; ++i) {
            *dst++ = *src++;
            *dst++ = *src++;
            ++output->runningSampleIndex;
        }

        secondaryBuffer->Unlock(region1, region1Size, region2, region2Size);
    }
}

uint32_t AudioDevice::RemainingSamples()
{
    uint32_t bytes = 0;
    auto secondaryBuffer = *(LPDIRECTSOUNDBUFFER *)globalSecondaryBuffer;
    
    DWORD outputBufferSize = output->samplesPerSecond * output->bytesPerSample;
    DWORD playCursor = 0;
    DWORD writeCursor = 0;
    if (SUCCEEDED(secondaryBuffer->GetCurrentPosition(&playCursor, &writeCursor))) {
        DWORD offset = (output->runningSampleIndex * output->bytesPerSample) % outputBufferSize;
        DWORD target = (playCursor + (output->latencySampleCount * output->bytesPerSample)) % outputBufferSize;

        if (offset > target) {
            bytes = outputBufferSize - offset;
            bytes += target;
        } else {
            bytes = target - offset;
        }
    }

    return bytes / output->bytesPerSample;
}
