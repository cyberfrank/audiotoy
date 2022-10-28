#include <stdint.h>

struct AudioDevice
{
    void Setup(void *context, int sampleRate, int outputBufferSize);
    void ClearBuffer();
    void FillBuffer(float *samples, uint32_t numSamples);
    uint32_t RemainingSamples();

    int samplesPerSecond;
    int bytesPerSample;
    int latencySampleCount;
    int secondaryBufferSize;
    uint64_t runningSampleIndex;
    void *globalSecondaryBuffer;
};