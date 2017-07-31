#ifndef JACK_MIDI_SYNTH_SAMPLE_MANAGER_H
#define JACK_MIDI_SYNTH_SAMPLE_MANAGER_H

#include <string>
#include <map>

#include "jack_midi_synth_sample.h"

class SampleManager {
  private:
    SampleManager() {};
    ~SampleManager();
    std::map<std::string, Sample*> samples;
  public:
    static SampleManager& get();
    Sample* getSample(const char*);
};

#endif // JACK_MIDI_SYNTH_SAMPLE_MANAGER_H
