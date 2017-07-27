#ifndef JACK_MIDI_SYNTH_APP_H
#define JACK_MIDI_SYNTH_APP_H

class Voice;
struct FloatEvent;

#include <jack/types.h>

class JackApp {
  protected:
    jack_client_t *client;
    jack_nframes_t sample_rate;
    jack_nframes_t buffer_size;
  public:
    JackApp();
    ~JackApp();
    void run() const;
    static int static_srate(jack_nframes_t, void*);
    static int static_bsize(jack_nframes_t, void*);
    static void error(const char*);
    static void static_jack_shutdown(void*);
    static int static_process(jack_nframes_t, void*);
    virtual int srate(jack_nframes_t) {};
    virtual int bsize(jack_nframes_t) {};
    virtual int process(jack_nframes_t) {};
    virtual void jack_shutdown() {};
};

#endif  // JACK_MIDI_SYNTH_APP_H
