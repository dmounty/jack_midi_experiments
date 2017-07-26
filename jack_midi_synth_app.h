#ifndef JACK_MIDI_SYNTH_APP_H
#define JACK_MIDI_SYNTH_APP_H

class Voice;
struct FloatEvent;

#include <jack/types.h>

#include <vector>
#include <list>

class JackApp {
  private:
    jack_client_t *client;
    std::vector<Voice*> voices;
    jack_nframes_t sample_rate;
    jack_nframes_t buffer_size;
    std::list<jack_port_t*> midi_input_ports;
    std::list<jack_port_t*> audio_output_ports;
    int global_frame;
    std::list<FloatEvent> bend_events;
    std::list<FloatEvent> mod_wheel_events;
    std::list<FloatEvent> expression_events;
    std::list<FloatEvent> aftertouch_events;
    std::list<FloatEvent> sustain_events;
    std::vector<float> bend;
    std::vector<float> mod_wheel;
    std::vector<float> expression;
    std::vector<float> aftertouch;
    std::vector<float> sustain;
  public:
    JackApp();
    ~JackApp();
    void activate();
    void add_ports();
    void connect_ports();
    void run();
    void initialize_voices();
    static int srate(jack_nframes_t, void*);
    static int bsize(jack_nframes_t, void*);
    static void error(const char*);
    static void jack_shutdown(void*);
    static int process(jack_nframes_t, void*);
    void cycleEventList(std::list<FloatEvent>&);
    void interpolateEvents(const std::list<FloatEvent>&, std::vector<float>&);
};

#endif  // JACK_MIDI_SYNTH_APP_H
