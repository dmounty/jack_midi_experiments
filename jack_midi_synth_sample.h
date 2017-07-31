#ifndef JACK_MIDI_SYNTH_SAMPLE_H
#define JACK_MIDI_SYNTH_SAMPLE_H

#include <vector>

class Sample {
  private:
    std::vector<float> audio;
    float pitch;
  public:
    Sample(const char*, float=261.2);
    float getAmplitude(int);
};

#endif // JACK_MIDI_SYNTH_SAMPLE_H
