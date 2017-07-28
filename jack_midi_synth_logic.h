#ifndef JACK_MIDI_SYNTH_LOGIC_H
#define JACK_MIDI_SYNTH_LOGIC_H

#include <vector>
#include <list>

#include <jack/types.h>

#include "jack_midi_synth_app.h"

class Voice;
struct FloatEvent;

class JackSynth : public JackApp {
  private:
    std::vector<Voice*> voices;
    std::list<jack_port_t*> midi_input_ports;
    std::list<jack_port_t*> audio_output_ports;
    int global_frame;
    std::list<FloatEvent> bend_events;
    std::list<FloatEvent> mod_wheel_events;
    std::list<FloatEvent> expression_events;
    std::list<FloatEvent> aftertouch_events;
    std::list<FloatEvent> sustain_events;
    std::vector<float> bend;
    std::vector<float> bend_freq;
    std::vector<float> mod_wheel;
    std::vector<float> expression;
    std::vector<float> aftertouch;
    std::vector<float> sustain;
  public:
    JackSynth();
    ~JackSynth();
    void activate();
    void add_ports();
    void connect_ports();
    void initialize_voices();
    virtual int srate(jack_nframes_t) override;
    virtual int bsize(jack_nframes_t) override;
    virtual void jack_shutdown() override;
    virtual int process(jack_nframes_t) override;
    void cycleEventList(std::list<FloatEvent>&) const;
    void interpolateEvents(const std::list<FloatEvent>&, std::vector<float>&) const;
    void bendToFreq();
};

#endif  // JACK_MIDI_SYNTH_LOGIC_H
