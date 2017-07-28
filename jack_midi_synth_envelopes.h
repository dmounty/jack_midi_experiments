#ifndef JACK_MIDI_SYNTH_ENVELOPES_H
#define JACK_MIDI_SYNTH_ENVELOPES_H


class Envelope {
  protected:
    bool down;
    bool sounding;
    bool pedal;
    float up_time;
    float up_weight;
  public:
    Envelope() : down(false), sounding(false), pedal(false), up_time(0.0), up_weight(0.0) {}
    virtual void pushDown();
    void liftUp();
    void setPedal(bool);
    bool isSounding();
    virtual float getWeight(float) = 0;
};


class LAD : public Envelope {
  protected:
    float attack;
    float decay;
    float delay;
  public:
    LAD(float init_attack, float init_decay, float init_delay=0.0) : attack(init_attack), decay(init_decay), delay(init_delay) {}
    virtual float getWeight(float) override;
};


class LADSR : public LAD {
  private:
    float sustain;
    float release;
    bool in_release;
  public:
    LADSR(float init_attack, float init_decay, float init_sustain, float init_release, float init_delay=0.0) : LAD(init_attack, init_decay, init_delay), in_release(false), sustain(init_sustain), release(init_release) {
      sustain = init_sustain;
      release = init_release;
    }
    virtual float getWeight(float) override;
    virtual void pushDown() override;
};


class DL4R4 : public Envelope {
  private:
    float L[4];
    float R[4];
    float in_release;
    float delay;
  public:
    DL4R4(float, float, float, float, float, float, float, float, float);
    float getWeight(float) override;
    virtual void pushDown() override;
};

#endif // JACK_MIDI_SYNTH_ENVELOPES_H
