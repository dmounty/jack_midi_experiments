#include <iostream>
#include <sstream>

#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <map>

#include <jack/jack.h>
#include <jack/midiport.h>


class JackApp {
  private:
    jack_client_t *client;
    static int last_port;
    static std::map<int,jack_port_t*> note_port_map;
    static int port_count;
    static jack_nframes_t sample_rate;
    static std::vector<jack_port_t*> midi_input_ports;
    static std::vector<jack_port_t*> midi_output_ports;
  public:
    JackApp(int init_port_count) {
      port_count = init_port_count;
      jack_set_error_function(JackApp::error);
      client = jack_client_open("Midi Stripe", JackNoStartServer, NULL);
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
int JackApp::port_count = 0;
int JackApp::last_port = 0;
std::map<int,jack_port_t*> JackApp::note_port_map;
std::vector<jack_port_t*> JackApp::midi_input_ports;
std::vector<jack_port_t*> JackApp::midi_output_ports;

int JackApp::process(jack_nframes_t nframes, void *arg) {
  if (midi_input_ports.size() > 0 && midi_output_ports.size() >0) {
    auto in_port = midi_input_ports.front();
    auto in = jack_port_get_buffer(in_port, nframes);
    for (auto out_port: midi_output_ports) {
      auto out = jack_port_get_buffer(out_port, nframes);
      jack_midi_clear_buffer(out);
    }
    for (int i=0;i < jack_midi_get_event_count(in); ++i) {
      jack_midi_event_t event;
      jack_midi_event_get(&event, in, i);
      int operation = 0;
      int channel = 0;
      if (event.size > 0) {
        operation = event.buffer[0] >> 4;
        channel = event.buffer[0] & 0xF;
      }
      if (operation == 8 || operation == 9) {
        int note = event.buffer[1];
        jack_port_t *out_port;
        if (operation == 9) {
          last_port = (last_port + 1) % port_count;
          out_port = midi_output_ports[last_port];
          note_port_map[note] = out_port;
        } else {
          out_port = note_port_map[note];
          note_port_map.erase(note);
        }
        auto out = jack_port_get_buffer(out_port, nframes);
        jack_midi_data_t* data = jack_midi_event_reserve(out, event.time, event.size);
        memcpy(data, event.buffer, event.size);
      } else {
        for (auto out_port: midi_output_ports) {
          auto out = jack_port_get_buffer(out_port, nframes);
          jack_midi_data_t* data = jack_midi_event_reserve(out, event.time, event.size);
          memcpy(data, event.buffer, event.size);
        }
      }
    }
  }
  return 0;
}

void JackApp::add_ports() {
  midi_input_ports.push_back(jack_port_register(client, "midi_input", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0));
  for (int i=0;i < port_count; i++) {
    std::stringstream name;
    name << "midi_output_" << i;
    midi_output_ports.push_back(jack_port_register(client, name.str().c_str(), JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0));
  }
}

void JackApp::connect_ports() {
  const char **ports;
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "usage: " << argv[0] << " <number_of_outputs>" << std::endl;
    return 1;
  }
  JackApp my_app(atoi(argv[1]));
  my_app.activate();
  my_app.run();
  return 0;
}
