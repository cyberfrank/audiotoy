#include <stdint.h>

class AudioOutput
{
public:
    int samplesPerSecond;
    int bytesPerSample;
    uint64_t runningSampleIndex;
    int latencySampleCount;
};

class AudioDevice
{
public:
    void Setup(void *context, AudioOutput *audioOut);
    void ClearBuffer();
    void FillBuffer(int16_t *samples, uint32_t num_samples);
    uint32_t RemainingSamples();
private:
    AudioOutput *output;
    void *globalSecondaryBuffer;
};