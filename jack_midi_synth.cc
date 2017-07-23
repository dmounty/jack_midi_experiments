#include <iostream>
#include <sstream>

#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include <list>
#include <vector>
#include <cmath>

#include <jack/jack.h>
#include <jack/midiport.h>

struct Envelope {
  float attack;
  float decay;
  float sustain;
  float release;
};

class Voice {
  private:
    float pitch;
    float velocity;
    float bend;
    float mod_wheel;
    float expression;
    float aftertouch;
    Envelope envelope;
    float release_weight;
    bool sustain;
    int trigger_frame;
    float release_time;
    int sample_rate;
    bool sounding;
    bool down;
  public:
    Voice(int note) : pitch(freq(note)), sounding(false), trigger_frame(0), release_time(0.0), velocity(0.0), bend(0.0), mod_wheel(0.0), expression(1.0), aftertouch(0.0), sustain(false), release_weight(0.0) {
      envelope.attack = 0.2;
      envelope.decay = 0.1;
      envelope.sustain = 0.8;
      envelope.release = 0.5;
    }
    bool isSounding() { return sounding; }
    void trigger_voice(float, int);
    void release_voice() { down = false; }
    void update(float, float, float, float, float);
    void render(float*, int, int);
    float freq(int note) { return pow(2.0f, ((note - 69.0) / 12.0)) * 440.0; }
    void setSampleRate(int rate) { sample_rate = rate; }
};

void Voice::trigger_voice(float new_velocity, int first_frame) {
  velocity = new_velocity;
  trigger_frame = first_frame;
  down = true;
  sounding = true;
}

void Voice::update(float new_bend, float new_mod_wheel, float new_expression, float new_aftertouch, float new_sustain) {
  bend = new_bend;
  mod_wheel = new_mod_wheel;
  expression = new_expression;
  aftertouch = new_aftertouch;
  sustain = new_sustain > 0.5;
}

void Voice::render(float* out, int global_frame, int length) {
  float freq = pitch / sample_rate;
  for (int frame=0; frame < length; ++frame) {
    int frames_since_trigger = frame + global_frame - trigger_frame;
    float time_since_trigger = static_cast<float>(frames_since_trigger) / sample_rate;
    float weight = 0.0;
    if (sounding) {
      if (down && time_since_trigger < envelope.attack) {
        weight = (velocity - 0) * (time_since_trigger / envelope.attack);
        release_weight = weight;
        release_time = time_since_trigger;
      } else if (down && time_since_trigger < envelope.attack + envelope.decay) {
        weight = velocity + (velocity * envelope.sustain - velocity) * ((time_since_trigger - envelope.attack) / envelope.decay);
        release_weight = weight;
        release_time = time_since_trigger;
      } else if (down || sustain) {
        weight = velocity * envelope.sustain + aftertouch;
        release_weight = weight;
        release_time = time_since_trigger;
      } else {
        weight = release_weight + (0 - release_weight) * (time_since_trigger - release_time) / envelope.release;
        if (weight <= 0) {
          sounding = false;
          weight = 0;
        }
      }
      out[frame] += expression * weight * sin(frames_since_trigger * freq * 6.2831853);
    }
  }
}


class JackApp {
  private:
    jack_client_t *client;
    static std::vector<Voice> voices;
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
    JackApp() {
      jack_set_error_function(JackApp::error);
      client = jack_client_open("Midi Synth", JackNoStartServer, NULL);
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
      initialize_voices();
      connect_ports();
    }
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
std::vector<Voice> JackApp::voices;
int JackApp::global_frame = 0;
float JackApp::bend = 0.0;
float JackApp::mod_wheel = 0.0;
float JackApp::expression = 1.0;
float JackApp::aftertouch = 0.0;
float JackApp::sustain = 0.0;

void JackApp::initialize_voices() {
  for(int i=0;i<128;i++) {
    voices.push_back(Voice(i));
    voices.back().setSampleRate(sample_rate);
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
        voices[event.buffer[1]].trigger_voice(event.buffer[1] / 127.0, global_frame);
      } else if (operation == 8) {
        voices[event.buffer[1]].release_voice();
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
      }
    }
  }
  if (audio_output_ports.size() > 0) {
    auto out_port = audio_output_ports.front();
    auto out = reinterpret_cast<float*>(jack_port_get_buffer(out_port, nframes));
    memset(out, 0, nframes * 4);
    for (int note_num=0; note_num < voices.size() ; ++note_num) {
      voices[note_num].update(bend, mod_wheel, expression, aftertouch, sustain);
      if (voices[note_num].isSounding()) {
        voices[note_num].render(out, global_frame, nframes);
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

        //elif operation == 14:
        //    (_, value) = struct.unpack('<BH', indata)
        //    bend = (value - 16384.0) / 16384.0

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
