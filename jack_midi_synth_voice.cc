#include "jack_midi_synth_voice.h"
#include "jack_midi_synth_envelopes.h"
#include "jack_midi_synth_oscillators.h"
#include "jack_midi_synth_filters.h"
#include "jack_midi_synth_events.h"

#include <cstring>


Voice::Voice(int note) {
  pitch = freq(note);
  trigger_frame = 0;
  velocity = 0.0;
  bend = 0.0;
  mod_wheel = 0.0;
  expression = 1.0;
  aftertouch = 0.0;
  sustain = 0.0;
  envelope = new LADSR(0.05, 0.2, 0.9, 0.7);
  osc_env_mixes.push_back(OscEnvMix(new Pulse(-2.0),     new LADSR(0.25, 0.2,  0.7, 0.6), 0.2)); // Sub 2
  osc_env_mixes.push_back(OscEnvMix(new Triangle(-1.0),  new LADSR(0.2,  0.15, 0.8, 0.5), 0.4)); // Sub 1
  osc_env_mixes.push_back(OscEnvMix(new Sine(0.0),       new LADSR(0.15, 0.1,  0.9, 0.4), 0.7)); // Main
  osc_env_mixes.push_back(OscEnvMix(new Saw(7.0/12.0),   new LADSR(0.1,  0.15, 0.8, 0.3), 0.3)); // Fifth
  osc_env_mixes.push_back(OscEnvMix(new ReverseSaw(1.0), new LADSR(0.05, 0.2,  0.7, 0.2), 0.1)); // Octave
  osc_env_mixes.push_back(OscEnvMix(new Noise(),         new LADSR(0.0,  0.25, 0.6, 0.1), 0.01)); // Noise
  filters.push_back(new Pass);
}

Voice::~Voice() {
  delete envelope;
  for (auto osc_env_mix: osc_env_mixes)  {
    delete osc_env_mix.oscillator;
    delete osc_env_mix.envelope;
  }
  for (auto filter: filters) delete filter;
}

bool Voice::isSounding() {
  if (envelope->isSounding()) {
    return true;
  }
  for (auto osc_env_mix: osc_env_mixes) {
    if (osc_env_mix.envelope->isSounding()) return true;
  }
  return false;
}

void Voice::triggerVoice(float new_velocity, int first_frame) {
  velocity = new_velocity;
  trigger_frame = first_frame;
  envelope->pushDown();
  for (auto osc_env_mix: osc_env_mixes) osc_env_mix.envelope->pushDown();
}

void Voice::releaseVoice() {
  envelope->liftUp();
  for (auto osc_env_mix: osc_env_mixes) osc_env_mix.envelope->liftUp();
}

void Voice::update(const FloatEvent& bend_event, const FloatEvent& mod_wheel_event, const FloatEvent& expression_event, const FloatEvent& aftertouch_event, const FloatEvent& sustain_event) {
  bend = bend_event.event;
  expression = expression_event.event;
  aftertouch = aftertouch_event.event;
  if (sustain_event.event != sustain) {
    sustain = sustain_event.event;
    bool pedal = sustain > 0.5;
    envelope->setPedal(pedal);
    for (auto osc_env_mix: osc_env_mixes) osc_env_mix.envelope->setPedal(pedal);
  }
  if (mod_wheel_event.event != mod_wheel) {
    mod_wheel = mod_wheel_event.event;
    for (auto osc_env_mix: osc_env_mixes) osc_env_mix.oscillator->setFloatParameter(PitchedOscillator::PARAMETER_PULSE_CENTRE, mod_wheel);
    //for (auto filter: filters) filter->setParameter(Pass::PARAMETER_CUTOFF, 1.0f - mod_wheel);
    //for (auto filter: filters) filter->setParameter(Pass::PARAMETER_RESONANCE, mod_wheel);
  }
}

void Voice::render(float* out, int global_frame, int length) {
  float freq = pow(2.0, bend) * pitch / sample_rate;
  float voice_channel[length];
  memset(voice_channel, 0, sizeof(voice_channel));
  for (auto osc_env_mix: osc_env_mixes) {
    for (int frame=0; frame < length; ++frame) {
      int frames_since_trigger = frame + global_frame - trigger_frame;
      float time_since_trigger = static_cast<float>(frames_since_trigger) / sample_rate;
      float voice_weight = expression * velocity * envelope->getWeight(time_since_trigger);
      voice_channel[frame] += voice_weight * osc_env_mix.mix * osc_env_mix.envelope->getWeight(time_since_trigger) * osc_env_mix.oscillator->getAmplitude(freq);
    }
  }
  for (auto filter: filters) {
    for (int frame=0; frame < length; ++frame) {
      voice_channel[frame] = filter->process(voice_channel[frame]);
    }
  }
  for (int frame=0; frame < length; ++frame) {
    out[frame] += voice_channel[frame];
  }
}

float Voice::freq(int note) {
  return pow(2.0f, ((note - 69.0) / 12.0)) * 440.0;
}

void Voice::setSampleRate(int rate) {
  sample_rate = rate;
}

void Voice::setBufferSize(int size) {
  buffer_size = size;
}
