#include "audio_buffer.hpp"
#include <chrono>
#include <iostream>
#include <sndfile.hh>

AudioBuffer::AudioBuffer(std::string file_name) {
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

    // Open file.
    SndfileHandle file(file_name);
    if (!file)
        return;

    samplerate = file.samplerate();
    num_channels = file.channels();
    format = file.format();
    constexpr size_t kFramesPerRead = 10240;
    size_t frames_read;
    do {
        // Make room for more samples.
        samples.resize((num_frames + kFramesPerRead) * num_channels);

        // Read frames to vector.
        frames_read = file.readf(&samples[num_frames * num_channels], kFramesPerRead);
        num_frames += frames_read;
    } while (frames_read == kFramesPerRead);

    // Trim end of vector.
    samples.resize(num_frames * num_channels);

    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    std::cerr << file_name << " decoded in "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << " ms"
              << std::endl;
}

void AudioBuffer::SaveTo(float start, float end, const std::string& filename) {
    // assert(start < end);
    SndfileHandle file(filename, SFM_WRITE, format, num_channels, samplerate);
    size_t number_of_frames = (end - start) * samplerate;
    float* begin = samples.data() + static_cast<size_t>(start * num_channels * samplerate);
    file.writef(begin, number_of_frames);
}