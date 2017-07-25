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

jack_nframes_t JackApp::sample_rate = 0;
jack_nframes_t JackApp::buffer_size = 0;
std::list<jack_port_t*> JackApp::midi_input_ports;
std::list<jack_port_t*> JackApp::audio_output_ports;
std::vector<Voice*> JackApp::voices;
int JackApp::global_frame = 0;
std::list<FloatEvent> JackApp::bend_events = {FloatEvent(0, 0.0)};
std::list<FloatEvent> JackApp::mod_wheel_events = {FloatEvent(0, 0.0)};
std::list<FloatEvent> JackApp::expression_events = {FloatEvent(0, 1.0)};
std::list<FloatEvent> JackApp::aftertouch_events = {FloatEvent(0, 0.0)};
std::list<FloatEvent> JackApp::sustain_events = {FloatEvent(0, 0.0)};


JackApp::JackApp() {
  jack_set_error_function(JackApp::error);
  client = jack_client_open("Midi Synth", JackNoStartServer, NULL);
  sample_rate = jack_get_sample_rate(client);
  jack_set_process_callback(client, JackApp::process, 0);
  jack_set_sample_rate_callback(client, JackApp::srate, 0);
  jack_set_buffer_size_callback(client, JackApp::bsize, 0);
  jack_on_shutdown(client, JackApp::jack_shutdown, 0);
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

int JackApp::process(jack_nframes_t nframes, void *arg) {
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
  if (audio_output_ports.size() > 0) {
    auto out_port = audio_output_ports.front();
    auto out = reinterpret_cast<float*>(jack_port_get_buffer(out_port, nframes));
    memset(out, 0, nframes * 4);
    for (int note_num=0; note_num < voices.size() ; ++note_num) {
      voices[note_num]->update(bend_events.back(), mod_wheel_events.back(), expression_events.back(), aftertouch_events.back(), sustain_events.back());
      if (voices[note_num]->isSounding()) voices[note_num]->render(out, global_frame, nframes);
    }
    for (int frame=0; frame < nframes; ++frame) out[frame] = tanh(out[frame]) / 1.5707963;
  }
  global_frame += nframes;
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

  for (int i=0;ports[i];++i) {
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
  sample_rate = nframes;
  for (auto voice: voices) voice->setSampleRate(sample_rate);
  return 0;
}

int JackApp::bsize(jack_nframes_t nframes, void *arg) {
  buffer_size = nframes;
  for (auto voice: voices) voice->setBufferSize(buffer_size);
  return 0;
}

void JackApp::error(const char *desc) {
  std::cerr << "JACK error: " << desc << std::endl;
}

void JackApp::jack_shutdown(void *arg) {
  exit(1);
}
