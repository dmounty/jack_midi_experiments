#ifndef JACK_MIDI_SYNTH_ENVELOPES_H
#define JACK_MIDI_SYNTH_ENVELOPES_H


class Envelope {
  protected:
    float attack;
    float decay;
    float delay;
    bool down;
    bool sounding;
    bool pedal;
    float up_time;
    float up_weight;
  public:
    Envelope(float, float, float);
    virtual void pushDown();
    void liftUp();
    void setPedal(bool);
    bool isSounding();
    virtual float getWeight(float) = 0;
};


class LADSR : public Envelope {
  private:
    float sustain;
    float release;
    bool in_release;
  public:
    LADSR(float init_attack, float init_decay, float init_sustain, float init_release, float init_delay=0.0) : Envelope(init_attack, init_decay, init_delay), in_release(false) {
      sustain = init_sustain;
      release = init_release;
    }
    virtual float getWeight(float) override;
    virtual void pushDown() override;
};


class LAD : public Envelope {
  public:
    LAD(float init_attack, float init_decay, float init_delay=0.0) : Envelope(init_attack, init_decay, init_delay) {}
    float getWeight(float) override;
};

#endif // JACK_MIDI_SYNTH_ENVELOPES_H
