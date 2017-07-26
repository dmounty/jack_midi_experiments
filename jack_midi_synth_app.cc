#include <list>
#include <iostream>
#include <cstring>
#include <cmath>
#include <unistd.h>

#include <jack/jack.h>
#include <jack/midiport.h>

#include "jack_midi_synth_voice.h"
#include "jack_midi_synth_app.h"
#include "jack_midi_synth_events.h"



#include <functional>

JackApp::JackApp() : sample_rate (0), buffer_size (0), global_frame (0), bend_events ({FloatEvent(0, 0.0)}), mod_wheel_events ({FloatEvent(0, 0.0)}), expression_events ({FloatEvent(0, 1.0)}), aftertouch_events ({FloatEvent(0, 0.0)}), sustain_events ({FloatEvent(0, 0.0)}) {
  jack_set_error_function(JackApp::error);
  client = jack_client_open("Midi Synth", JackNoStartServer, NULL);
  if (!client) {
    std::cerr << "Unable to create Jack client" << std::endl;
    exit(1);
  }
  sample_rate = jack_get_sample_rate(client);
  buffer_size = jack_get_buffer_size(client);
  jack_set_process_callback(client, JackApp::process, this);
  jack_set_sample_rate_callback(client, JackApp::srate, this);
  jack_set_buffer_size_callback(client, JackApp::bsize, this);
  jack_on_shutdown(client, JackApp::jack_shutdown, this);
  add_ports();
}

JackApp::~JackApp() {
  jack_client_close(client);
  for (auto voice: voices) delete voice;
}

void JackApp::activate() {
  if(jack_activate(client)) {
    std::cerr << "Unable to activate client" << std::endl;
    exit(1);
  }
  initialize_voices();
  connect_ports();
}

void JackApp::initialize_voices() {
  for(int i=0;i<128;i++) {
    voices.push_back(new Voice(i));
    voices.back()->setSampleRate(sample_rate);
    voices.back()->setBufferSize(buffer_size);
  }
}

void JackApp::cycleEventList(std::list<FloatEvent>& event_list) {
  FloatEvent last_event = event_list.back();
  last_event.frame -= buffer_size;
  event_list.clear();
  event_list.push_back(last_event);
}

void JackApp::interpolateEvents(const std::list<FloatEvent>& event_list, std::vector<float>& values) {
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

int JackApp::process(jack_nframes_t nframes, void *arg) {
  JackApp* o = reinterpret_cast<JackApp*>(arg);
  if (o->midi_input_ports.size() > 0) {
    o->cycleEventList(o->bend_events);
    o->cycleEventList(o->mod_wheel_events);
    o->cycleEventList(o->expression_events);
    o->cycleEventList(o->aftertouch_events);
    o->cycleEventList(o->sustain_events);
    auto in_port = o->midi_input_ports.front();
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
        o->voices[event.buffer[1]]->triggerVoice(event.buffer[2] / 127.0, o->global_frame + event.time);
      } else if (operation == 8) {
        o->voices[event.buffer[1]]->releaseVoice();
      } else if (operation == 11) {
        if (event.buffer[1] == 1) {
          o->mod_wheel_events.push_back(FloatEvent(event.time, event.buffer[2] / 127.0));
        } else if (event.buffer[1] == 11) {
          o->expression_events.push_back(FloatEvent(event.time, event.buffer[2] / 127.0));
        } else if (event.buffer[1] == 64) {
          o->sustain_events.push_back(FloatEvent(event.time, event.buffer[2] /127));
        }
      } else if (operation == 12) {
      } else if (operation == 13) {
        o->aftertouch_events.push_back(FloatEvent(event.time, event.buffer[1] / 127.0));
      } else if (operation == 14) {
        o->bend_events.push_back(FloatEvent(event.time, *reinterpret_cast<short*>(event.buffer + 1) / 16384.0 - 1.0));
      }
    }
  }
  o->interpolateEvents(o->bend_events, o->bend);
  o->interpolateEvents(o->mod_wheel_events, o->mod_wheel);
  o->interpolateEvents(o->expression_events, o->expression);
  o->interpolateEvents(o->aftertouch_events, o->aftertouch);
  o->interpolateEvents(o->sustain_events, o->sustain);
  if (o->audio_output_ports.size() > 0) {
    auto out_port = o->audio_output_ports.front();
    auto out = reinterpret_cast<float*>(jack_port_get_buffer(out_port, nframes));
    memset(out, 0, nframes * 4);
    for (int note_num=0; note_num < o->voices.size() ; ++note_num) {
      o->voices[note_num]->update(&o->bend, &o->mod_wheel, &o->expression, &o->aftertouch, &o->sustain);
      if (o->voices[note_num]->isSounding()) o->voices[note_num]->render(out, o->global_frame, nframes);
    }
    for (int frame=0; frame < nframes; ++frame) out[frame] = tanh(out[frame]) / 1.5707963;
  }
  o->global_frame += nframes;
  return 0;
}

void JackApp::add_ports() {
  midi_input_ports.push_back(jack_port_register(client, "midi_input", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0));
  audio_output_ports.push_back(jack_port_register(client, "audio_output", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0));
}

void JackApp::connect_ports() {
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

void JackApp::run() {
  for(;;) sleep(1);
}

int JackApp::srate(jack_nframes_t nframes, void *arg) {
  JackApp* o = reinterpret_cast<JackApp*>(arg);
  o->sample_rate = nframes;
  for (auto voice: o->voices) voice->setSampleRate(nframes);
  return 0;
}

int JackApp::bsize(jack_nframes_t nframes, void *arg) {
  JackApp* o = reinterpret_cast<JackApp*>(arg);
  o->buffer_size = nframes;
  o->bend.resize(nframes);
  o->mod_wheel.resize(nframes);
  o->expression.resize(nframes);
  o->aftertouch.resize(nframes);
  o->sustain.resize(nframes);
  for (auto voice: o->voices) voice->setBufferSize(nframes);
  return 0;
}

void JackApp::error(const char *desc) {
  std::cerr << "JACK error: " << desc << std::endl;
}

void JackApp::jack_shutdown(void *arg) {
  exit(1);
}
