#ifndef JACK_MIDI_SYNTH_VOICE_H
#define JACK_MIDI_SYNTH_VOICE_H

class Envelope;
class Oscillator;
class Filter;

struct FloatEvent;

#include <list>
#include <vector>

#include "jack_midi_synth_envelopes.h"

struct OscEnvMix {
  OscEnvMix(Oscillator* init_oscillator, Envelope* init_envelope, float init_mix) : oscillator(init_oscillator), envelope(init_envelope), mix(init_mix) {}
  Oscillator* oscillator;
  Envelope* envelope;
  float mix;
};

class Voice {
  private:
    float pitch;
    float velocity;
    const std::vector<float>* bend;
    const std::vector<float>* mod_wheel;
    const std::vector<float>* expression;
    const std::vector<float>* sustain;
    const std::vector<float>* aftertouch;
    std::list<Filter*> filters;
    Envelope* envelope;
    std::list<OscEnvMix> osc_env_mixes;
    int trigger_frame;
    int sample_rate;
    int buffer_size;
  public:
    Voice(int);
    ~Voice();
    bool isSounding();
    void triggerVoice(float, int);
    void releaseVoice();
    void update(const std::vector<float>*, const std::vector<float>*, const std::vector<float>*, const std::vector<float>*, const std::vector<float>*);
    void render(float*, int, int);
    float freq(int) const;
    void setSampleRate(int);
    void setBufferSize(int);
};

#endif // JACK_MIDI_SYNTH_VOICE_H
