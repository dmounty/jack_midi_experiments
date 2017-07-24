#include <iostream>
#include <sstream>

#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include <list>
#include <vector>
#include <cmath>
#include <random>
#include <utility>

#include <jack/jack.h>
#include <jack/midiport.h>


class Oscillator {
  protected:
    float phase;
    float offset;
    float mod;
  public:
    Oscillator(float init_phase=6.2831853) : phase(init_phase), offset(0.0) {}
    virtual float getAmplitude(float) = 0;
    virtual void setMod(float new_mod) { mod = new_mod; }
};


class PitchedOscillator : public Oscillator {
  protected:
    float tuning;
  public:
    PitchedOscillator(float tune) : Oscillator(), tuning(pow(2.0, tune)) {}
};


class Sine : public PitchedOscillator {
  public:
    Sine(float tune=0.0) : PitchedOscillator(tune) {}
    virtual float getAmplitude(float);
};

float Sine::getAmplitude(float phase_step) {
  offset += phase_step * tuning * phase;
  offset = fmod(offset, phase);
  return sin(offset);
}


class Noise : public Oscillator {
  private:
    std::default_random_engine generator;
    std::uniform_real_distribution<float> distribution;
  public:
    Noise() : distribution(-1.0, 1.0) {}
    virtual float getAmplitude(float);
};

float Noise::getAmplitude(float phase_step) {
  return distribution(generator);
}

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
    void pushDown() { down = true; sounding = true; }
    void liftUp() { down = false; }
    void setPedal(bool this_pedal) { pedal=this_pedal; }
    bool isSounding() { return sounding; }
    virtual float getWeight(float) = 0;
};

Envelope::Envelope(float init_attack, float init_decay, float init_delay=0.0) {
  attack = init_attack;
  decay = init_decay;
  delay = init_delay;
  down = false;
  sounding = false;
  pedal = false;
  up_time = 0.0;
  up_weight = 0.0;
}


class LADSR : public Envelope {
  private:
    float sustain;
    float release;
  public:
    LADSR(float, float, float, float, float);
    float getWeight(float);
};

LADSR::LADSR(float init_attack, float init_decay, float init_sustain, float init_release, float init_delay=0.0) : Envelope(init_attack, init_decay, init_delay) {
  sustain = init_sustain;
  release = init_release;
}

float LADSR::getWeight(float time) {
  float weight = 0.0;
  if (sounding) {
    if        (down && time < delay) {
      weight = 0.0;
      up_time = time;
      up_weight = weight;
    } else if (down && time < delay + attack) {
      weight = (time - delay)/attack;
      up_time = time;
      up_weight = weight;
    } else if (down && time < delay + attack + decay) {
      weight = 1.0 - (1.0 - sustain) * (time - (delay + attack)) / decay;
      up_time = time;
      up_weight = weight;
    } else if (down || pedal) {
      weight = sustain;
      up_time = time;
      up_weight = weight;
    } else {
      weight = up_weight +(-up_weight)*(time - up_time) / release;
      if (weight <= 0.0) sounding = false;
    }
  }
  return weight;
}


class LAD : public Envelope {
  public:
    LAD(float init_attack, float init_decay, float init_delay=0.0) : Envelope(init_attack, init_decay, init_delay) {}
    float getWeight(float);
};

float LAD::getWeight(float time) {
  float weight = 0.0;
  if (sounding) {
    if        (down && time < delay) {
      weight = 0.0;
      up_time = time;
      up_weight = weight;
    } else if (down && time < delay + attack) {
      weight = (time - delay)/attack;
      up_time = time;
      up_weight = weight;
    } else {
      weight = up_weight +(-up_weight)*(time - up_time) / decay;
      if (weight <= 0.0) sounding = false;
    }
  }
  return weight;
}

struct OscEnvMix {
  OscEnvMix(Oscillator* init_oscillator, Envelope* init_envelope, float init_mix) : oscillator(init_oscillator), envelope(init_envelope), mix(init_mix) {}
  Oscillator* oscillator;
  Envelope* envelope;
  float mix;
};

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
    Pass(FilterMode, int);
    float process(float);
    void setCutoff(float new_cutoff) { cutoff = new_cutoff; calculateFeedbackAmount(); }
    void setResonance(float new_resonance) { resonance = new_resonance; calculateFeedbackAmount(); }
    void setParameter(int, float);
    void setFilterMode(FilterMode);
};

Pass::Pass(FilterMode filter=FILTER_MODE_LOWPASS, int init_order=2) : mode(kNumFilterModes), buf(init_order, 0.0)
{
  setFilterMode(filter);
  setCutoff(0.99);
  setResonance(0.0);
  calculateFeedbackAmount();
}

void Pass::setParameter(int parameter, float value) {
  if (value < 0.01) value = 0.01;
  if (value > 0.99) value = 0.99;
  switch (parameter) {
    case PARAMETER_CUTOFF:
      setCutoff(value);
      break;
    case PARAMETER_RESONANCE:
      setResonance(value);
      break;
  }
}

void Pass::setFilterMode(Pass::FilterMode new_mode) {
  if (new_mode >= 0 && new_mode < kNumFilterModes) mode = new_mode;
}

float Pass::process(float value) {
  float new_value = value;
  buf[0] += cutoff * (value - buf[0] + feedbackAmount * (buf[0] - buf[1]));
  for (int i=1; i < buf.size(); ++i) buf[i] += cutoff * (buf[i-1] - buf[i]);
  switch (mode) {
    case FILTER_MODE_LOWPASS:
      new_value = buf[buf.size()-1];
      break;
    case FILTER_MODE_HIGHPASS:
      new_value = value - buf[buf.size()-1];
      break;
    case FILTER_MODE_BANDPASS:
      new_value = buf[0] - buf[buf.size()-1];
      break;
  }
  return new_value;
}

class Voice {
  private:
    float pitch;
    float velocity;
    float bend;
    float mod_wheel;
    float expression;
    float sustain;
    float aftertouch;
    std::list<Filter*> filters;
    Envelope* envelope;
    std::list<OscEnvMix> osc_env_mixes;
    int trigger_frame;
    int sample_rate;
  public:
    Voice(int);
    ~Voice();
    bool isSounding();
    void triggerVoice(float, int);
    void releaseVoice();
    void update(float, float, float, float, float);
    void render(float*, int, int);
    float freq(int note) { return pow(2.0f, ((note - 69.0) / 12.0)) * 440.0; }
    void setSampleRate(int rate) { sample_rate = rate; }
};

Voice::Voice(int note) {
  pitch = freq(note);
  trigger_frame = 0;
  velocity = 0.0;
  bend = 0.0;
  mod_wheel = 0.0;
  expression = 1.0;
  aftertouch = 0.0;
  sustain = 0.0;
  envelope = new LADSR(0.2, 0.1, 0.8, 0.5);
  osc_env_mixes.push_back(OscEnvMix(new Sine(-2.0), new LAD(0.05, 0.5), 0.7)); // Sub
  osc_env_mixes.push_back(OscEnvMix(new Sine(0.0), new LADSR(0.1, 0.2, 0.8, 0.5), 0.6)); // Main
  osc_env_mixes.push_back(OscEnvMix(new Sine(7.0/12.0), new LADSR(0.1, 0.2, 0.7, 0.5), 0.5)); // Fifth
  osc_env_mixes.push_back(OscEnvMix(new Sine(1.0), new LADSR(0.0, 0.3, 0.6, 0.5), 0.4)); // Octave
  osc_env_mixes.push_back(OscEnvMix(new Noise(), new LAD(0.1, 1.0), 0.01)); // Sub
  filters.push_back(new Pass);
}

Voice::~Voice() {
  delete envelope;
  for (auto osc_env_mix: osc_env_mixes)  {
    delete osc_env_mix.oscillator;
    delete osc_env_mix.envelope;
  }
  for (auto filter: filters) delete filter;
}

bool Voice::isSounding() {
  if (envelope->isSounding()) {
    return true;
  }
  for (auto osc_env_mix: osc_env_mixes) {
    if (osc_env_mix.envelope->isSounding()) return true;
  }
  return false;
}

void Voice::triggerVoice(float new_velocity, int first_frame) {
  velocity = new_velocity;
  trigger_frame = first_frame;
  envelope->pushDown();
  for (auto osc_env_mix: osc_env_mixes) osc_env_mix.envelope->pushDown();
}

void Voice::releaseVoice() {
  envelope->liftUp();
  for (auto osc_env_mix: osc_env_mixes) osc_env_mix.envelope->liftUp();
}

void Voice::update(float new_bend, float new_mod_wheel, float new_expression, float new_aftertouch, float new_sustain) {
  bend = new_bend;
  expression = new_expression;
  aftertouch = new_aftertouch;
  if (new_sustain != sustain) {
    sustain = new_sustain;
    bool pedal = sustain > 0.5;
    envelope->setPedal(pedal);
    for (auto osc_env_mix: osc_env_mixes) osc_env_mix.envelope->setPedal(pedal);
  }
  if (new_mod_wheel != mod_wheel) {
    mod_wheel = new_mod_wheel;
    for (auto filter: filters) filter->setParameter(Pass::PARAMETER_CUTOFF, 1.0f - mod_wheel);
    for (auto filter: filters) filter->setParameter(Pass::PARAMETER_RESONANCE, mod_wheel);
  }
}

void Voice::render(float* out, int global_frame, int length) {
  float freq = pow(2.0, bend) * pitch / sample_rate;
  float voice_channel[length];
  memset(voice_channel, 0, sizeof(voice_channel));
  for (auto osc_env_mix: osc_env_mixes) {
    for (int frame=0; frame < length; ++frame) {
      int frames_since_trigger = frame + global_frame - trigger_frame;
      float time_since_trigger = static_cast<float>(frames_since_trigger) / sample_rate;
      float voice_weight = expression * velocity * envelope->getWeight(time_since_trigger);
      voice_channel[frame] += voice_weight * osc_env_mix.mix * osc_env_mix.envelope->getWeight(time_since_trigger) * osc_env_mix.oscillator->getAmplitude(freq);
    }
  }
  for (auto filter: filters) {
    for (int frame=0; frame < length; ++frame) {
      voice_channel[frame] = filter->process(voice_channel[frame]);
    }
  }
  for (int frame=0; frame < length; ++frame) {
    out[frame] += voice_channel[frame];
  }
}


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
    void run() {
      for(;;) sleep(1);
    }
    static void initialize_voices();
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
std::list<jack_port_t*> JackApp::audio_output_ports;
std::vector<Voice*> JackApp::voices;
int JackApp::global_frame = 0;
float JackApp::bend = 0.0;
float JackApp::mod_wheel = 0.0;
float JackApp::expression = 1.0;
float JackApp::aftertouch = 0.0;
float JackApp::sustain = 0.0;

JackApp::JackApp() {
  jack_set_error_function(JackApp::error);
  client = jack_client_open("Midi Synth", JackNoStartServer, NULL);
  sample_rate = jack_get_sample_rate(client);
  jack_set_process_callback(client, JackApp::process, 0);
  jack_set_sample_rate_callback(client, JackApp::srate, 0);
  jack_on_shutdown(client, JackApp::jack_shutdown, 0);
  add_ports();
}

JackApp::~JackApp() {
  jack_client_close(client);
  for (auto voice: voices) delete voice;
}

void JackApp::activate() {
  if(jack_activate(client)) {
    std::cerr << "Unable to activate client" << std::endl;
    exit(1);
  }
  initialize_voices();
  connect_ports();
}

void JackApp::initialize_voices() {
  for(int i=0;i<128;i++) {
    voices.push_back(new Voice(i));
    voices.back()->setSampleRate(sample_rate);
  }
}

int JackApp::process(jack_nframes_t nframes, void *arg) {
  if (midi_input_ports.size() > 0) {
    auto in_port = midi_input_ports.front();
    auto in = jack_port_get_buffer(in_port, nframes);
    for (int i=0; i < jack_midi_get_event_count(in); ++i) {
      jack_midi_event_t event;
      jack_midi_event_get(&event, in, i);
      int operation = 0;
      int channel = 0;
      if (event.size > 0) {
        operation = event.buffer[0] >> 4;
        channel = event.buffer[0] & 0xF;
      }
      if (operation == 9) {
        voices[event.buffer[1]]->triggerVoice(event.buffer[1] / 127.0, global_frame);
      } else if (operation == 8) {
        voices[event.buffer[1]]->releaseVoice();
      } else if (operation == 11) {
        if (event.buffer[1] == 1) {
          mod_wheel = event.buffer[2] / 127.0;
        } else if (event.buffer[1] == 11) {
          expression = event.buffer[2] / 127.0;
        } else if (event.buffer[1] == 64) {
          sustain = (event.buffer[2] /127);
        }
      } else if (operation == 12) {
      } else if (operation == 13) {
        aftertouch = event.buffer[1] / 127.0;
      } else if (operation == 14) {
        bend = *reinterpret_cast<short*>(event.buffer + 1) / 16384.0 - 1.0;
      }
    }
  }
  if (audio_output_ports.size() > 0) {
    auto out_port = audio_output_ports.front();
    auto out = reinterpret_cast<float*>(jack_port_get_buffer(out_port, nframes));
    memset(out, 0, nframes * 4);
    for (int note_num=0; note_num < voices.size() ; ++note_num) {
      voices[note_num]->update(bend, mod_wheel, expression, aftertouch, sustain);
      if (voices[note_num]->isSounding()) {
        voices[note_num]->render(out, global_frame, nframes);
      }
    }
    for (int frame=0; frame < nframes; ++frame) {
      out[frame] = atan(out[frame]) / 1.5707963;
    }
  }
  aftertouch -= (nframes / sample_rate) * 2.0;
  global_frame += nframes;
  return 0;
}

void JackApp::add_ports() {
  midi_input_ports.push_back(jack_port_register(client, "midi_input", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0));
  audio_output_ports.push_back(jack_port_register(client, "audio_output", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0));
}

void JackApp::connect_ports() {
  const char **ports;

  //Get an array of all physical output ports
  if((ports = jack_get_ports(client, NULL, JACK_DEFAULT_AUDIO_TYPE, JackPortIsPhysical|JackPortIsTerminal|JackPortIsInput)) == NULL) {
    std::cerr << "Cannot find any physical playback ports" << std::endl;
  }

  // Connect our output port to all physical output ports (NULL terminated array)
  int i=0;
  while(ports[i] != NULL){
    if(jack_connect(client, jack_port_name(audio_output_ports.front()), ports[i])) {
      std::cerr << "cannot connect output ports" << std::endl;
    }
    i++;
  }

  jack_free(ports);
}

int main(int argc, char *argv[]) {
  JackApp my_app;
  my_app.activate();
  my_app.run();
  return 0;
}
