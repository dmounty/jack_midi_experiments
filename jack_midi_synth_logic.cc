#include <iostream>
#include <cstring>
#include <cmath>

#include <jack/jack.h>
#include <jack/midiport.h>

#include "jack_midi_synth_voice.h"
#include "jack_midi_synth_logic.h"
#include "jack_midi_synth_events.h"


JackSynth::JackSynth() : JackApp(), global_frame (0), bend_events ({FloatEvent(0, 0.0)}), mod_wheel_events ({FloatEvent(0, 0.0)}), expression_events ({FloatEvent(0, 1.0)}), aftertouch_events ({FloatEvent(0, 0.0)}), sustain_events ({FloatEvent(0, 0.0)}) {
  add_ports();
}

JackSynth::~JackSynth() {
  for (auto voice: voices) delete voice;
}

void JackSynth::activate() {
  if (jack_activate(client)) {
    std::cerr << "Unable to activate client" << std::endl;
    exit(1);
  }
  initialize_voices();
  connect_ports();
}

void JackSynth::initialize_voices() {
  for(int i=0;i<128;i++) {
    voices.push_back(new Voice(i));
    voices.back()->setSampleRate(sample_rate);
    voices.back()->setBufferSize(buffer_size);
  }
}

void JackSynth::cycleEventList(std::list<FloatEvent>& event_list) const {
  FloatEvent last_event = event_list.back();
  last_event.frame -= buffer_size;
  event_list.clear();
  event_list.push_back(last_event);
}

void JackSynth::interpolateEvents(const std::list<FloatEvent>& event_list, std::vector<float>& values) const {
  if (event_list.size() == 1) {
    for (auto& value: values) value = event_list.back().value;
  } else {
    FloatEvent previous_event = event_list.front();
    bool first = true;
    int offset = 0;
    for (const auto& event: event_list) {
      if (first) { first = false; continue; }
      for (int i=offset; i < values.size() ; ++i) {
        if (previous_event.frame < i && i <= event.frame) {
          values[i] = previous_event.value + (i - previous_event.frame) * (event.value - previous_event.value) / (event.frame - previous_event.frame);
        } else {
          previous_event = event;
          values[i] = event.value;
          offset = i;
          break;
        }
      }
    }
    for (int i=offset; i < values.size(); ++i) values[i] = previous_event.value;
  }
}

void JackSynth::bendToFreq() {
  for (int i=0; i < bend.size(); ++i) bend_freq[i] = pow(2.0, bend[i]);
}

int JackSynth::process(jack_nframes_t nframes) {
  if (midi_input_ports.size() > 0) {
    cycleEventList(bend_events);
    cycleEventList(mod_wheel_events);
    cycleEventList(expression_events);
    cycleEventList(aftertouch_events);
    cycleEventList(sustain_events);
    auto in_port = midi_input_ports.front();
    auto in = jack_port_get_buffer(in_port, nframes);
    for (int i=0; i < jack_midi_get_event_count(in); ++i) {
      jack_midi_event_t event;
      jack_midi_event_get(&event, in, i);
      int operation = 0;
      int channel = 0;
      if (event.size > 0) {
        operation = event.buffer[0] >> 4;
        channel = event.buffer[0] & 0xF;
      }
      if (operation == 9) {
        voices[event.buffer[1]]->triggerVoice(event.buffer[2] / 127.0, global_frame + event.time);
      } else if (operation == 8) {
        voices[event.buffer[1]]->releaseVoice();
      } else if (operation == 11) {
        if (event.buffer[1] == 1) {
          mod_wheel_events.push_back(FloatEvent(event.time, event.buffer[2] / 127.0));
        } else if (event.buffer[1] == 11) {
          expression_events.push_back(FloatEvent(event.time, event.buffer[2] / 127.0));
        } else if (event.buffer[1] == 64) {
          sustain_events.push_back(FloatEvent(event.time, event.buffer[2] /127));
        }
      } else if (operation == 12) {
      } else if (operation == 13) {
        aftertouch_events.push_back(FloatEvent(event.time, event.buffer[1] / 127.0));
      } else if (operation == 14) {
        bend_events.push_back(FloatEvent(event.time, *reinterpret_cast<short*>(event.buffer + 1) / 16384.0 - 1.0));
      }
    }
  }
  interpolateEvents(bend_events, bend);
  interpolateEvents(mod_wheel_events, mod_wheel);
  interpolateEvents(expression_events, expression);
  interpolateEvents(aftertouch_events, aftertouch);
  interpolateEvents(sustain_events, sustain);
  bendToFreq();
  if (audio_output_ports.size() > 0) {
    auto out_port = audio_output_ports.front();
    auto out = reinterpret_cast<float*>(jack_port_get_buffer(out_port, nframes));
    memset(out, 0, nframes * 4);
    for (int note_num=0; note_num < voices.size() ; ++note_num) {
      voices[note_num]->update(&bend, &bend_freq, &mod_wheel, &expression, &aftertouch, &sustain);
      if (voices[note_num]->isSounding()) voices[note_num]->render(out, global_frame, nframes);
    }
    for (int frame=0; frame < nframes; ++frame) out[frame] = tanh(out[frame]) / 1.5707963;
  }
  global_frame += nframes;
  return 0;
}

void JackSynth::add_ports() {
  midi_input_ports.push_back(jack_port_register(client, "midi_input", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0));
  audio_output_ports.push_back(jack_port_register(client, "audio_output", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0));
}

void JackSynth::connect_ports() {
  const char **ports;

  if((ports = jack_get_ports(client, NULL, JACK_DEFAULT_AUDIO_TYPE, JackPortIsPhysical|JackPortIsTerminal|JackPortIsInput)) == NULL) {
    std::cerr << "Cannot find any physical playback ports" << std::endl;
  }

  for (int i=0; ports[i]; ++i) {
    if(jack_connect(client, jack_port_name(audio_output_ports.front()), ports[i])) {
      std::cerr << "cannot connect output ports" << std::endl;
    }
  }

  jack_free(ports);
}

int JackSynth::srate(jack_nframes_t nframes) {
  for (auto voice: voices) voice->setSampleRate(nframes);
  return 0;
}

int JackSynth::bsize(jack_nframes_t nframes) {
  buffer_size = nframes;
  bend.resize(nframes);
  bend_freq.resize(nframes);
  mod_wheel.resize(nframes);
  expression.resize(nframes);
  aftertouch.resize(nframes);
  sustain.resize(nframes);
  for (auto voice: voices) voice->setBufferSize(nframes);
  return 0;
}

void JackSynth::jack_shutdown() {
  exit(1);
}
