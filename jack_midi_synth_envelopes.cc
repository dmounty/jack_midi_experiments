#include "jack_midi_synth_envelopes.h"


void Envelope::pushDown() {
  down = true;
  sounding = true;
}

void Envelope::liftUp() {
  down = false;
}

void Envelope::setPedal(bool this_pedal) {
  pedal=this_pedal;
}

bool Envelope::isSounding() {
  return sounding;
}

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
      weight = up_weight + (-up_weight)*(time - up_time) / decay;
      if (weight <= 0.0) sounding = false;
    }
  }
  return weight;
}
