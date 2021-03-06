#include "jack_midi_synth_voice.h"
#include "jack_midi_synth_envelopes.h"
#include "jack_midi_synth_oscillators.h"
#include "jack_midi_synth_filters.h"
#include "jack_midi_synth_events.h"

#include <cstring>
#include <cmath>


Voice::Voice(int note) {
  pitch = freq(note);
  trigger_frame = 0;
  velocity = 0.0;
  envelope = new LADSR(0.06, 0.25, 0.9, 1.5, 0.01);
  osc_env_mixes.push_back(OscEnvMix(new Audio("test.wav"), new LADSR(0.1, 0.5, 0.9, 3.0), 0.8));            // Sample
  osc_env_mixes.push_back(OscEnvMix(new Sine(2.0),         new LADSR(0.06, 0.15, 0.8,  1.0, 0.015), 0.2));  // Sub
  osc_env_mixes.push_back(OscEnvMix(new Triangle(-1.0),    new LADSR(0.06, 0.2,  0.65, 0.9, 0.015), 0.1));  // Sub fifth
  osc_env_mixes.push_back(OscEnvMix(new Triangle(0.0),     new LADSR(0.05, 0.25, 0.5,  0.8, 0.02),  0.7));  // Main
  osc_env_mixes.push_back(OscEnvMix(new Sine(7.0/12.0),    new LADSR(0.04, 0.2,  0.7,  0.7, 0.02),  0.3));  // Fifth
  osc_env_mixes.push_back(OscEnvMix(new Sine(1.0),         new LADSR(0.03, 0.15, 0.4,  0.6, 0.02),  0.4));  // Octave
  osc_env_mixes.push_back(OscEnvMix(new Pulse(2.0),        new LADSR(0.02, 0.1,  0.3,  0.5, 0.02),  0.05)); // Octave 2
  filters.push_back(new Pass);
  filters.push_back(new Delay(0.1, 0.7));
}

Voice::~Voice() {
  delete envelope;
  for (auto& osc_env_mix: osc_env_mixes)  {
    delete osc_env_mix.oscillator;
    delete osc_env_mix.envelope;
  }
  for (auto& filter: filters) delete filter;
}

bool Voice::isSounding() {
  if (envelope->isSounding()) {
    return true;
  }
  for (auto& osc_env_mix: osc_env_mixes) {
    if (osc_env_mix.envelope->isSounding()) return true;
  }
  return false;
}

void Voice::triggerVoice(float new_velocity, int first_frame) {
  velocity = new_velocity;
  trigger_frame = first_frame;
  envelope->pushDown();
  for (auto& osc_env_mix: osc_env_mixes) {
    osc_env_mix.envelope->pushDown();
    osc_env_mix.oscillator->reset();
  }
}

void Voice::releaseVoice() {
  envelope->liftUp();
  for (auto& osc_env_mix: osc_env_mixes) osc_env_mix.envelope->liftUp();
}

void Voice::update(const std::vector<float>* new_bend, const std::vector<float>* new_bend_freq, const std::vector<float>* new_mod_wheel, const std::vector<float>* new_expression, const std::vector<float>* new_aftertouch, const std::vector<float>* new_sustain) {
  bend = new_bend;
  bend_freq = new_bend_freq;
  mod_wheel = new_mod_wheel;
  expression = new_expression;
  aftertouch = new_aftertouch;
  sustain = new_sustain;
  bool pedal = (*sustain)[sustain->size()/2];
  envelope->setPedal(pedal);
  for (auto& osc_env_mix: osc_env_mixes) osc_env_mix.envelope->setPedal(pedal);
  for (auto& osc_env_mix: osc_env_mixes) osc_env_mix.oscillator->setFloatParameter(PitchedOscillator::PARAMETER_PULSE_CENTRE, 0.5 + (*mod_wheel)[mod_wheel->size()/2]*0.5);
  for (auto filter: filters) {
    if (strcmp(filter->type, "Pass") == 0) filter->setParameter(Pass::PARAMETER_CUTOFF, 1.0f - (*aftertouch)[aftertouch->size()/2]);
    if (strcmp(filter->type, "Pass") == 0) filter->setParameter(Pass::PARAMETER_RESONANCE, (*aftertouch)[aftertouch->size()/2]);
  }
}

void Voice::render(float* out, int global_frame, int length) {
  float raw_freq = pitch / sample_rate;
  float voice_channel[length];
  memset(voice_channel, 0, sizeof(voice_channel));
  for (auto& osc_env_mix: osc_env_mixes) {
    for (int frame=0; frame < length; ++frame) {
      float freq = (*bend_freq)[frame] * raw_freq;
      int frames_since_trigger = frame + global_frame - trigger_frame;
      float time_since_trigger = static_cast<float>(frames_since_trigger) / sample_rate;
      float voice_weight = (*expression)[frame] * velocity * envelope->getWeight(time_since_trigger);
      voice_channel[frame] += voice_weight * (osc_env_mix.mix * (1.0 + (*aftertouch)[frame])) * osc_env_mix.envelope->getWeight(time_since_trigger) * osc_env_mix.oscillator->getAmplitude(freq);
    }
  }
  for (auto& filter: filters) {
    for (int frame=0; frame < length; ++frame) {
      filter->process(voice_channel[frame]);
    }
  }
  for (int frame=0; frame < length; ++frame) {
    out[frame] += tanh(voice_channel[frame]);
  }
}

float Voice::freq(int note) const {
  return pow(2.0f, ((note - 69.0) / 12.0)) * 440.0;
}

void Voice::setSampleRate(int rate) {
  sample_rate = rate;
  for (auto& filter: filters) filter->setSampleRate(rate);
}

void Voice::setBufferSize(int size) {
  buffer_size = size;
}
