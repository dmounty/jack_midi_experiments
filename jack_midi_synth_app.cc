#include <iostream>
#include <unistd.h>

#include <jack/jack.h>

#include "jack_midi_synth_app.h"


JackApp::JackApp() : sample_rate (0), buffer_size (0) {
  jack_set_error_function(JackApp::error);
  client = jack_client_open("Midi Synth", JackNoStartServer, NULL);
  if (!client) {
    std::cerr << "Unable to create Jack client" << std::endl;
    exit(1);
  }
  sample_rate = jack_get_sample_rate(client);
  buffer_size = jack_get_buffer_size(client);
  jack_set_process_callback(client, JackApp::static_process, this);
  jack_set_sample_rate_callback(client, JackApp::static_srate, this);
  jack_set_buffer_size_callback(client, JackApp::static_bsize, this);
  jack_on_shutdown(client, JackApp::static_jack_shutdown, this);
}

JackApp::~JackApp() {
  jack_client_close(client);
}

int JackApp::static_process(jack_nframes_t nframes, void *arg) {
  JackApp* o = reinterpret_cast<JackApp*>(arg);
  return o->process(nframes);
}

void JackApp::run() {
  for(;;) sleep(1);
}

int JackApp::static_srate(jack_nframes_t nframes, void *arg) {
  JackApp* o = reinterpret_cast<JackApp*>(arg);
  return o->srate(nframes);
}

int JackApp::static_bsize(jack_nframes_t nframes, void *arg) {
  JackApp* o = reinterpret_cast<JackApp*>(arg);
  return o->bsize(nframes);
}

void JackApp::error(const char *desc) {
  std::cerr << "JACK error: " << desc << std::endl;
}

void JackApp::static_jack_shutdown(void *arg) {
  JackApp* o = reinterpret_cast<JackApp*>(arg);
  o->jack_shutdown();
}
