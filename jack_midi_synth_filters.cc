#include "jack_midi_synth_filters.h"


void Pass::setParameter(int parameter, float value) {
  if (value < 0.01) value = 0.01;
  if (value > 0.99) value = 0.99;
  switch (parameter) {
    case PARAMETER_CUTOFF:
      setCutoff(value);
      break;
    case PARAMETER_RESONANCE:
      setResonance(value);
      break;
  }
}

void Pass::setFilterMode(Pass::FilterMode new_mode) {
  if (new_mode >= 0 && new_mode < kNumFilterModes) mode = new_mode;
}

float Pass::process(float value) {
  float new_value = value;
  buf[0] += cutoff * (value - buf[0] + feedbackAmount * (buf[0] - buf[1]));
  for (int i=1; i < buf.size(); ++i) buf[i] += cutoff * (buf[i-1] - buf[i]);
  switch (mode) {
    case FILTER_MODE_LOWPASS:
      new_value = buf[buf.size()-1];
      break;
    case FILTER_MODE_HIGHPASS:
      new_value = value - buf[buf.size()-1];
      break;
    case FILTER_MODE_BANDPASS:
      new_value = buf[0] - buf[buf.size()-1];
      break;
    case FILTER_MODE_NOTCH:
      new_value = value - buf[0] + buf[buf.size()-1];
      break;
  }
  return new_value;
}
