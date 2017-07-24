#include "jack_midi_synth_oscillators.h"


void Oscillator::setMod(float new_mod) {
  mod = new_mod;
}


float Sine::getAmplitude(float phase_step) {
  offset += phase_step * tuning * phase;
  offset = fmod(offset, phase);
  return sin(offset);
}


float Pulse::getAmplitude(float phase_step) {
  offset += phase_step * tuning * phase;
  offset = fmod(offset, phase);
  return offset < phase/2.0 ? -1.0 : 1.0;
}


float Triangle::getAmplitude(float phase_step) {
  offset += phase_step * tuning * phase;
  offset = fmod(offset, phase);
  return offset < phase/2.0 ? (4.0 * offset / phase -1.0) : (3.0 - (4.0 * offset / phase));
}


float Saw::getAmplitude(float phase_step) {
  offset += phase_step * tuning * phase;
  offset = fmod(offset, phase);
  return (2.0 * offset/phase) - 1.0;
}


float ReverseSaw::getAmplitude(float phase_step) {
  offset += phase_step * tuning * phase;
  offset = fmod(offset, phase);
  return 1.0 - (2.0 * offset/phase);
}


float Noise::getAmplitude(float phase_step) {
  return distribution(generator);
}
