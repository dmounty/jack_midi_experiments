#include "jack_midi_synth_sample_manager.h"


SampleManager::~SampleManager() {
  for (auto& sample: samples) {
    delete sample.second;
  }
}

SampleManager& SampleManager::get() {
  static SampleManager instance;
  return instance;
}

Sample* SampleManager::getSample(const char* filename) {
  auto this_sample = samples.find(filename);
  if (this_sample == samples.end()) {
    samples[filename] = new Sample(filename);
    return samples[filename];
  } else return this_sample->second;
}
