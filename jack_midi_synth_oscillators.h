#ifndef JACK_MIDI_SYNTH_OSCILLATORS_H
#define JACK_MIDI_SYNTH_OSCILLATORS_H

#include <random>
#include <cmath>


class Oscillator {
  protected:
    float phase;
    float offset;
    float mod;
  public:
    Oscillator(float init_phase=6.2831853) : phase(init_phase), offset(0.0) {}
    virtual float getAmplitude(float) = 0;
    virtual void setMod(float);
};


class PitchedOscillator : public Oscillator {
  protected:
    float tuning;
  public:
    PitchedOscillator(float tune) : Oscillator(), tuning(pow(2.0, tune)) {}
};


class Sine : public PitchedOscillator {
  public:
    Sine(float tune=0.0) : PitchedOscillator(tune) {}
    virtual float getAmplitude(float);
};


class Pulse : public PitchedOscillator {
  public:
    Pulse(float tune=0.0) : PitchedOscillator(tune) {}
    virtual float getAmplitude(float);
};


class Triangle : public PitchedOscillator {
  public:
    Triangle(float tune=0.0) : PitchedOscillator(tune) {}
    virtual float getAmplitude(float);
};


class Noise : public Oscillator {
  private:
    std::default_random_engine generator;
    std::uniform_real_distribution<float> distribution;
  public:
    Noise() : distribution(-1.0, 1.0) {}
    virtual float getAmplitude(float);
};

#endif // JACK_MIDI_SYNTH_OSCILLATORS_H
