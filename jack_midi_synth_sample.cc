#include <iostream>

#include "jack_midi_synth_sample.h"

#include <sndfile.h>

Sample::Sample(const char* filename, float init_pitch) : pitch(init_pitch) {
  SF_INFO sfinfo;
  SNDFILE *sound_file = sf_open(filename, SFM_READ, &sfinfo);
  if (int error=sf_error(sound_file)) {
    std::cerr << sf_error_number(error) << std::endl;
  } else {
    audio.resize(sfinfo.frames);
    int items = sfinfo.frames * sfinfo.channels;
    std::vector<float> all_channels(items);
    sf_read_float(sound_file, all_channels.data(), items);
    for (int i=0; i < audio.size() ; ++i) {
      for (int j=0; j < sfinfo.channels; ++j) {
        audio[i] += all_channels[i * sfinfo.channels + j] / sfinfo.channels;
      }
    }
  }
}

float Sample::getAmplitude(int sample) {
  if (audio.size()) {
    sample %= audio.size();
    return audio[sample];
  }
  return 0;
}
