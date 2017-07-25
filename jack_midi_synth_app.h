#ifndef JACK_MIDI_SYNTH_APP_H
#define JACK_MIDI_SYNTH_APP_H

class Voice;

#include <jack/types.h>

#include <vector>
#include <list>

class JackApp {
  private:
    jack_client_t *client;
    static std::vector<Voice*> voices;
    static jack_nframes_t sample_rate;
    static std::list<jack_port_t*> midi_input_ports;
    static std::list<jack_port_t*> audio_output_ports;
    static int global_frame;
    static float bend;
    static float mod_wheel;
    static float expression;
    static float aftertouch;
    static float sustain;
  public:
    JackApp();
    ~JackApp();
    void activate();
    void add_ports();
    void connect_ports();
    void run();
    static void initialize_voices();
    static int srate(jack_nframes_t, void*);
    static void error(const char*);
    static void jack_shutdown(void*);
    static int process(jack_nframes_t, void*);
};

#endif  // JACK_MIDI_SYNTH_APP_H
