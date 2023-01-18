#include "audio_buffer.hpp"
#include <iostream>
#include <sndfile.hh>

AudioBuffer::AudioBuffer(std::string file_name) {
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
}
