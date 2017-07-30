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

void Pass::process(float& value) {
  buffer[0] += cutoff * (value - buffer[0] + feedbackAmount * (buffer[0] - buffer[1]));
  for (int i=1; i < buffer.size(); ++i) buffer[i] += cutoff * (buffer[i-1] - buffer[i]);
  switch (mode) {
    case FILTER_MODE_LOWPASS:
      value = buffer[buffer.size()-1];
      break;
    case FILTER_MODE_HIGHPASS:
      value = value - buffer[buffer.size()-1];
      break;
    case FILTER_MODE_BANDPASS:
      value = buffer[0] - buffer[buffer.size()-1];
      break;
    case FILTER_MODE_NOTCH:
      value = value - buffer[0] + buffer[buffer.size()-1];
      break;
  }
}

void Delay::process(float& value) {
  index %= buffer.size();
  value += buffer[index] * feedback;
  buffer[index++] = value;
}

void Delay::setParameter(int parameter, float value) {
  if (value < 0.00005) value = 0.00005;
  int new_size = static_cast<int>(value * sample_rate);
  switch (parameter) {
    case PARAMETER_DELAY:
      delay = value;
      if (buffer.size() != new_size) buffer.resize(new_size);
      break;
    case PARAMETER_FEEDBACK:
      if (value > 1.0) value = 1.0;
      feedback = value;
      break;
  }
}

void Delay::setSampleRate(int rate) {
  Filter::setSampleRate(rate);
  buffer.resize(sample_rate * delay);
}
