#ifndef JACK_MIDI_SYNTH_FILTERS_H
#define JACK_MIDI_SYNTH_FILTERS_H

#include <vector>

class Filter {
  public:
    virtual float process(float) = 0;
    virtual void setParameter(int, float) = 0;
};


class Pass : public Filter {
  public:
    enum FilterMode {
      FILTER_MODE_LOWPASS = 0,
      FILTER_MODE_HIGHPASS,
      FILTER_MODE_BANDPASS,
      kNumFilterModes
    };
    enum Parameters {
      PARAMETER_CUTOFF = 0,
      PARAMETER_RESONANCE,
      kNumParameters
    };
  private:
    double cutoff;
    double resonance;
    FilterMode mode;
    double feedbackAmount;
    inline void calculateFeedbackAmount() { feedbackAmount = resonance + resonance/(1.0 - cutoff); }
    std::vector<float> buf;
  public:
    Pass(FilterMode filter=FILTER_MODE_LOWPASS, int init_order=2) : mode(kNumFilterModes), buf(init_order, 0.0) {
      setFilterMode(filter);
      setCutoff(0.6);
      setResonance(0.4);
      calculateFeedbackAmount();
    }
    float process(float);
    void setCutoff(float new_cutoff) { cutoff = new_cutoff; calculateFeedbackAmount(); }
    void setResonance(float new_resonance) { resonance = new_resonance; calculateFeedbackAmount(); }
    void setParameter(int, float);
    void setFilterMode(FilterMode);
};

#endif // JACK_MIDI_SYNTH_FILTERS_H
