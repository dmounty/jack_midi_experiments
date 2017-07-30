#ifndef JACK_MIDI_SYNTH_FILTERS_H
#define JACK_MIDI_SYNTH_FILTERS_H

#include <vector>

class Filter {
  protected:
    int sample_rate;
  public:
    Filter(const char* init_type) : type(init_type) {}
    virtual void process(float&) = 0;
    virtual void setParameter(int, float) = 0;
    virtual void setSampleRate(int new_sample_rate) { sample_rate = new_sample_rate; }
    const char* type;
};


class Pass : public Filter {
  public:
    enum FilterMode {
      FILTER_MODE_LOWPASS = 0,
      FILTER_MODE_HIGHPASS,
      FILTER_MODE_BANDPASS,
      FILTER_MODE_NOTCH,
      kNumFilterModes
    };
    enum Parameters {
      PARAMETER_CUTOFF = 0,
      PARAMETER_RESONANCE,
      kNumParameters
    };
  private:
    float cutoff;
    float resonance;
    FilterMode mode;
    float feedbackAmount;
    inline void calculateFeedbackAmount() { feedbackAmount = resonance + resonance/(1.0 - cutoff); }
    std::vector<float> buffer;
  public:
    Pass(FilterMode filter=FILTER_MODE_LOWPASS, int init_order=2) : mode(kNumFilterModes), buffer(init_order, 0.0), Filter("Pass") {
      setFilterMode(filter);
      setCutoff(0.99);
      setResonance(0.01);
      calculateFeedbackAmount();
    }
    virtual void process(float&) override;
    void setCutoff(float new_cutoff) { cutoff = new_cutoff; calculateFeedbackAmount(); }
    void setResonance(float new_resonance) { resonance = new_resonance; calculateFeedbackAmount(); }
    void setParameter(int, float) override;
    void setFilterMode(FilterMode);
};


class Delay : public Filter {
  public:
    enum Parameters {
      PARAMETER_DELAY = 0,
      PARAMETER_FEEDBACK,
      kNumParameters
    };
  private:
    float delay;
    float feedback;
    std::vector<float> buffer;
    int index;
  public:
    Delay(float init_delay=0.3, float init_feedback=0.6) : delay(init_delay), feedback(init_feedback), index(0), Filter("Delay") { }
    virtual void process(float&) override;
    void setParameter(int, float) override;
    void setSampleRate(int) override;
};

#endif // JACK_MIDI_SYNTH_FILTERS_H
