#ifndef JACK_MIDI_SYNTH_VOICE_H
#define JACK_MIDI_SYNTH_VOICE_H

class Envelope;
class Oscillator;
class Filter;

#include <list>

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
    float bend;
    float mod_wheel;
    float expression;
    float sustain;
    float aftertouch;
    std::list<Filter*> filters;
    Envelope* envelope;
    std::list<OscEnvMix> osc_env_mixes;
    int trigger_frame;
    int sample_rate;
  public:
    Voice(int);
    ~Voice();
    bool isSounding();
    void triggerVoice(float, int);
    void releaseVoice();
    void update(float, float, float, float, float);
    void render(float*, int, int);
    float freq(int);
    void setSampleRate(int);
};

#endif // JACK_MIDI_SYNTH_VOICE_H
