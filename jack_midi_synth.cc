#include "jack_midi_synth_logic.h"

int main(int argc, char *argv[]) {
  JackSynth my_app;
  my_app.activate();
  my_app.run();
  return 0;
}
