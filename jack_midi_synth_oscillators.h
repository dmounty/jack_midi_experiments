#ifndef JACK_MIDI_SYNTH_OSCILLATORS_H
#define JACK_MIDI_SYNTH_OSCILLATORS_H

#include <random>
#include <cmath>

#include "jack_midi_synth_sample.h"


class Oscillator {
  protected:
    float offset;
  public:
    Oscillator(const char* init_type) : offset(0.0), type(init_type) {}
    virtual float getAmplitude(float) = 0;
    virtual void setFloatParameter(int, float) {}
    virtual void setIntParameter(int, int) {}
    virtual void setBoolParameter(int, bool) {}
    virtual void reset() { offset = 0.0; }
    const char* type;
};


class PitchedOscillator : public Oscillator {
  public:
    enum Parameters {
      PARAMETER_PULSE_CENTRE = 0,
      kNumParameters
    };
  protected:
    float tuning;
    float pulse_centre;
  public:
    PitchedOscillator(float tune, const char* init_type) : Oscillator(init_type), pulse_centre(0.5), tuning(pow(2.0, tune)) {}
    virtual float advanceOffset(float);
    virtual float pulseWidthModulate(float);
    virtual void setFloatParameter(int, float);
};


class Sine : public PitchedOscillator {
  public:
    Sine(float tune=0.0) : PitchedOscillator(tune, "Sine") {}
    virtual float getAmplitude(float) override;
};


class Pulse : public PitchedOscillator {
  public:
    Pulse(float tune=0.0) : PitchedOscillator(tune, "Pulse") {}
    virtual float getAmplitude(float) override;
};


class Triangle : public PitchedOscillator {
  public:
    Triangle(float tune=0.0) : PitchedOscillator(tune, "Triangle") {}
    virtual float getAmplitude(float) override;
};


class Saw : public PitchedOscillator {
  public:
    Saw(float tune=0.0) : PitchedOscillator(tune, "Saw") {}
    virtual float getAmplitude(float) override;
};


class ReverseSaw : public PitchedOscillator {
  public:
    ReverseSaw(float tune=0.0) : PitchedOscillator(tune, "ReverseSaw") {}
    virtual float getAmplitude(float) override;
};


class Noise : public Oscillator {
  private:
    std::default_random_engine generator;
    std::uniform_real_distribution<float> distribution;
  public:
    Noise() : distribution(-1.0, 1.0), Oscillator("Noise") {}
    virtual float getAmplitude(float) override;
};

class Audio : public Oscillator {
  private:
    Sample* audio;
    int sample;
  public:
    Audio(const char*, float=261.2);
    virtual float getAmplitude(float) override;
    virtual void reset() override;
};

#endif // JACK_MIDI_SYNTH_OSCILLATORS_H
