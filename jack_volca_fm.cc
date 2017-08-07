#include <iostream>

#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include <list>

#include <jack/jack.h>
#include <jack/midiport.h>


class JackApp {
  private:
    jack_client_t *client;
    static jack_nframes_t sample_rate;
    static std::list<jack_port_t*> midi_input_ports;
    static std::list<jack_port_t*> midi_output_ports;
  public:
    JackApp() {
      jack_set_error_function(JackApp::error);
      client = jack_client_open("VolcaFM", JackNoStartServer, NULL);
      sample_rate = jack_get_sample_rate(client);
      jack_set_process_callback(client, JackApp::process, 0);
      jack_set_sample_rate_callback(client, JackApp::srate, 0);
      jack_on_shutdown(client, JackApp::jack_shutdown, 0);
      add_ports();
    }
    ~JackApp() {
      jack_client_close(client);
    }
    void activate() {
      if(jack_activate(client)) {
        std::cerr << "Unable to activate client" << std::endl;
        exit(1);
      }
      connect_ports();
    }
    void add_ports();
    void connect_ports();
    void run() {
      for(;;) sleep(1);
    }
    static int srate(jack_nframes_t nframes, void *arg) {
      sample_rate = nframes;
      return 0;
    }
    static void error(const char *desc) {
      std::cerr << "JACK error: " << desc << std::endl;
    }
    static void jack_shutdown(void *arg) {
      exit(1);
    }
    static int process(jack_nframes_t, void*);
};

jack_nframes_t JackApp::sample_rate = 0;
std::list<jack_port_t*> JackApp::midi_input_ports;
std::list<jack_port_t*> JackApp::midi_output_ports;

int JackApp::process(jack_nframes_t nframes, void *arg) {
  if (midi_input_ports.size() > 0 && midi_output_ports.size() >0) {
    auto in_port = midi_input_ports.front();
    auto in = jack_port_get_buffer(in_port, nframes);
    auto out_port = midi_output_ports.front();
    auto out = jack_port_get_buffer(out_port, nframes);
    jack_midi_clear_buffer(out);
    for (int i=0;i < jack_midi_get_event_count(in); ++i) {
      jack_midi_event_t event;
      jack_midi_event_get(&event, in, i);
      int operation = 0;
      int channel = 0;
      if (event.size > 0) {
        operation = event.buffer[0] >> 4;
        channel = event.buffer[0] & 0xF;
      }
      if (operation == 9) {
        char message[3];
        message[0] = (0xB << 4) | channel;
        message[1] = 41;
        message[2] = event.buffer[2];
        jack_midi_data_t* data = jack_midi_event_reserve(out, event.time, 3);
        memcpy(data, message, event.size);
      }
      jack_midi_data_t* data = jack_midi_event_reserve(out, event.time, event.size);
      memcpy(data, event.buffer, event.size);
    }
  }
  return 0;
}

void JackApp::add_ports() {
  midi_input_ports.push_back(jack_port_register(client, "in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0));
  midi_output_ports.push_back(jack_port_register(client, "out", JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0));
}

void JackApp::connect_ports() {
}

int main(int argc, char *argv[]) {
  JackApp my_app;
  my_app.activate();
  my_app.run();
  return 0;
}
