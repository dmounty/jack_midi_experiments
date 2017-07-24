#include "jack_midi_synth_voice.h"
#include "jack_midi_synth_envelopes.h"
#include "jack_midi_synth_oscillators.h"
#include "jack_midi_synth_filters.h"

#include <list>
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
  envelope = new LADSR(0.2, 0.1, 0.8, 0.5);
  osc_env_mixes.push_back(OscEnvMix(new Pulse(-2.0), new LAD(0.05, 0.5), 0.4)); // Sub 2
  osc_env_mixes.push_back(OscEnvMix(new Sine(-1.0), new LAD(0.2, 1.0), 0.5)); // Sub 1
  osc_env_mixes.push_back(OscEnvMix(new Triangle(0.0), new LADSR(0.1, 0.2, 0.8, 0.5), 0.6)); // Main
  osc_env_mixes.push_back(OscEnvMix(new Saw(7.0/12.0), new LADSR(0.1, 0.2, 0.7, 0.5), 0.5)); // Fifth
  osc_env_mixes.push_back(OscEnvMix(new ReverseSaw(1.0), new LADSR(0.0, 0.3, 0.6, 0.5), 0.4)); // Octave
  osc_env_mixes.push_back(OscEnvMix(new Noise(), new LAD(0.1, 1.0), 0.01)); // Noise
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

void Voice::update(float new_bend, float new_mod_wheel, float new_expression, float new_aftertouch, float new_sustain) {
  bend = new_bend;
  expression = new_expression;
  aftertouch = new_aftertouch;
  if (new_sustain != sustain) {
    sustain = new_sustain;
    bool pedal = sustain > 0.5;
    envelope->setPedal(pedal);
    for (auto osc_env_mix: osc_env_mixes) osc_env_mix.envelope->setPedal(pedal);
  }
  if (new_mod_wheel != mod_wheel) {
    mod_wheel = new_mod_wheel;
    for (auto filter: filters) filter->setParameter(Pass::PARAMETER_CUTOFF, 1.0f - mod_wheel);
    for (auto filter: filters) filter->setParameter(Pass::PARAMETER_RESONANCE, mod_wheel);
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
