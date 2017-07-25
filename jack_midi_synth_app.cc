#include <list>
#include <iostream>
#include <cstring>
#include <cmath>
#include <unistd.h>

#include <jack/jack.h>
#include <jack/midiport.h>

#include "jack_midi_synth_voice.h"
#include "jack_midi_synth_app.h"

jack_nframes_t JackApp::sample_rate = 0;
std::list<jack_port_t*> JackApp::midi_input_ports;
std::list<jack_port_t*> JackApp::audio_output_ports;
std::vector<Voice*> JackApp::voices;
int JackApp::global_frame = 0;
float JackApp::bend = 0.0;
float JackApp::mod_wheel = 0.0;
float JackApp::expression = 1.0;
float JackApp::aftertouch = 0.0;
float JackApp::sustain = 0.0;


JackApp::JackApp() {
  jack_set_error_function(JackApp::error);
  client = jack_client_open("Midi Synth", JackNoStartServer, NULL);
  sample_rate = jack_get_sample_rate(client);
  jack_set_process_callback(client, JackApp::process, 0);
  jack_set_sample_rate_callback(client, JackApp::srate, 0);
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
  }
}

int JackApp::process(jack_nframes_t nframes, void *arg) {
  if (midi_input_ports.size() > 0) {
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
        voices[event.buffer[1]]->triggerVoice(event.buffer[1] / 127.0, global_frame);
      } else if (operation == 8) {
        voices[event.buffer[1]]->releaseVoice();
      } else if (operation == 11) {
        if (event.buffer[1] == 1) {
          mod_wheel = event.buffer[2] / 127.0;
        } else if (event.buffer[1] == 11) {
          expression = event.buffer[2] / 127.0;
        } else if (event.buffer[1] == 64) {
          sustain = (event.buffer[2] /127);
        }
      } else if (operation == 12) {
      } else if (operation == 13) {
        aftertouch = event.buffer[1] / 127.0;
      } else if (operation == 14) {
        bend = *reinterpret_cast<short*>(event.buffer + 1) / 16384.0 - 1.0;
      }
    }
  }
  if (audio_output_ports.size() > 0) {
    auto out_port = audio_output_ports.front();
    auto out = reinterpret_cast<float*>(jack_port_get_buffer(out_port, nframes));
    memset(out, 0, nframes * 4);
    for (int note_num=0; note_num < voices.size() ; ++note_num) {
      voices[note_num]->update(bend, mod_wheel, expression, aftertouch, sustain);
      if (voices[note_num]->isSounding()) {
        voices[note_num]->render(out, global_frame, nframes);
      }
    }
    for (int frame=0; frame < nframes; ++frame) {
      out[frame] = tanh(out[frame]) / 1.5707963;
    }
  }
  aftertouch -= (nframes / sample_rate) * 2.0;
  global_frame += nframes;
  return 0;
}

void JackApp::add_ports() {
  midi_input_ports.push_back(jack_port_register(client, "midi_input", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0));
  audio_output_ports.push_back(jack_port_register(client, "audio_output", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0));
}

void JackApp::connect_ports() {
  const char **ports;

  //Get an array of all physical output ports
  if((ports = jack_get_ports(client, NULL, JACK_DEFAULT_AUDIO_TYPE, JackPortIsPhysical|JackPortIsTerminal|JackPortIsInput)) == NULL) {
    std::cerr << "Cannot find any physical playback ports" << std::endl;
  }

  // Connect our output port to all physical output ports (NULL terminated array)
  int i=0;
  while(ports[i] != NULL){
    if(jack_connect(client, jack_port_name(audio_output_ports.front()), ports[i])) {
      std::cerr << "cannot connect output ports" << std::endl;
    }
    i++;
  }

  jack_free(ports);
}

void JackApp::run() {
  for(;;) sleep(1);
}

int JackApp::srate(jack_nframes_t nframes, void *arg) {
  sample_rate = nframes;
  return 0;
}

void JackApp::error(const char *desc) {
  std::cerr << "JACK error: " << desc << std::endl;
}

void JackApp::jack_shutdown(void *arg) {
  exit(1);
}
