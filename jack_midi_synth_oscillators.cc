#include "jack_midi_synth_oscillators.h"


float PitchedOscillator::advanceOffset(float phase_step) {
  offset += phase_step * tuning;
  offset = fmod(offset, 1.0);
  return pulseWidthModulate(offset);
}


float PitchedOscillator::pulseWidthModulate(float offset) {
  if (offset < pulse_centre) return 0.5 * offset / pulse_centre;
  else return 0.5 + (0.5 * (offset - pulse_centre) / (1.0 - pulse_centre));
}


float Sine::getAmplitude(float phase_step) {
  return sin(advanceOffset(phase_step) * 6.2831853);
}


void PitchedOscillator::setFloatParameter(int parameter, float value) {
  if (value < 0.01) value = 0.01;
  if (value > 0.99) value = 0.99;
  switch (parameter) {
    case PARAMETER_PULSE_CENTRE:
      pulse_centre = value;
      break;
  }
}


float Pulse::getAmplitude(float phase_step) {
  return advanceOffset(phase_step) < 0.5 ? -1.0 : 1.0;
}


float Triangle::getAmplitude(float phase_step) {
  float local_offset = advanceOffset(phase_step);
  return local_offset < 0.5 ? (4.0 * local_offset - 1.0) : (3.0 - (4.0 * local_offset));
}


float Saw::getAmplitude(float phase_step) {
  return (2.0 * advanceOffset(phase_step)) - 1.0;
}


float ReverseSaw::getAmplitude(float phase_step) {
  return 1.0 - (2.0 * advanceOffset(phase_step));
}


float Noise::getAmplitude(float phase_step) {
  return distribution(generator);
}
