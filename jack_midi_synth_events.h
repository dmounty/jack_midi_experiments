#ifndef JACK_MIDI_SYNTH_EVENTS_H
#define JACK_MIDI_SYNTH_EVENTS_H

struct BoolEvent {
  BoolEvent(int init_frame, bool init_event) : frame(init_frame), value(init_event) {}
  int frame;
  bool value;
};

struct IntEvent {
  IntEvent(int init_frame, int init_event) : frame(init_frame), value(init_event) {}
  int frame;
  int value;
};

struct FloatEvent {
  FloatEvent(int init_frame, float init_event) : frame(init_frame), value(init_event) {}
  int frame;
  float value;
};

#endif // JACK_MIDI_SYNTH_EVENTS_H
