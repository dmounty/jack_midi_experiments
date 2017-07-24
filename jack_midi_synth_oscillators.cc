#include "jack_midi_synth_oscillators.h"


void Oscillator::setMod(float new_mod) {
  mod = new_mod;
}


float Sine::getAmplitude(float phase_step) {
  offset += phase_step * tuning * phase;
  offset = fmod(offset, phase);
  return sin(offset);
}


float Noise::getAmplitude(float phase_step) {
  return distribution(generator);
}
