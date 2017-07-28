#include "jack_midi_synth_envelopes.h"


void Envelope::pushDown() {
  down = true;
  sounding = true;
}

void Envelope::liftUp() {
  down = false;
}

void Envelope::setPedal(bool this_pedal) {
  pedal = this_pedal;
}

bool Envelope::isSounding() {
  return sounding;
}

float linear_interpolate(float x_start, float y_start, float x_end, float y_end, float x) {
  float x_diff = x_end - x_start;
  float y_diff = y_end - y_start;
  if (x_diff > 0) {
    return y_start + (x - x_start) * y_diff / x_diff;
  } else {
    return 0.5 * (y_start + y_end);
  }
}


float LADSR::getWeight(float time) {
  float weight = 0.0;
  if (sounding) {
    if        (down && time < delay) {
      weight = 0.0;
    } else if (down && time < delay + attack) {
      weight = linear_interpolate(delay, 0.0, delay + attack, 1.0, time);
    } else if (down && time < delay + attack + decay) {
      weight = linear_interpolate(delay + attack, 1.0, delay + attack + decay, sustain, time);
    } else if (down || (pedal && !in_release)) {
      weight = sustain;
    } else {
      in_release = true;
      weight = linear_interpolate(up_time, up_weight, up_time + release, 0.0, time);
      if (weight <= 0.0) sounding = false;
      return weight;
    }
  }
  up_time = time;
  up_weight = weight;
  return weight;
}


void LADSR::pushDown() {
  Envelope::pushDown();
  in_release = false;
}


float LAD::getWeight(float time) {
  float weight = 0.0;
  if (sounding) {
    if        (down && time < delay) {
      weight = 0.0;
    } else if (down && time < delay + attack) {
      weight = linear_interpolate(delay, 0.0, delay + attack, 1.0, time);
    } else {
      weight = linear_interpolate(up_time, up_weight, up_time + decay, 0.0, time);
      if (weight <= 0.0) sounding = false;
      return weight;
    }
  }
  up_time = time;
  up_weight = weight;
  return weight;
}


DL4R4::DL4R4(float L1, float R1, float L2, float R2, float L3, float R3, float L4, float R4, float init_delay) : L{L1, L2, L3, L4}, R{R1, R2, R3, R4}, delay(init_delay), in_release(false) {
  if (L4 > 0.0) sounding = true;
  else sounding = false;
}

float DL4R4::getWeight(float time) {
  float weight = 0.0;
  if (sounding) {
    if (down && time < delay) {
      weight = L[3];
    } else if (down && time < delay + R[0]) {
      weight = linear_interpolate(delay, L[3], delay + R[0], L[0], time);
    } else if (down && time < delay + R[0] + R[1]) {
      weight = linear_interpolate(delay + R[0], L[0], delay + R[0] + R[1], L[1], time);
    } else if (down && time < delay + R[0] + R[1] + R[2]) {
      weight = linear_interpolate(delay + R[0] + R[1], L[1], delay + R[0] + R[1] + R[2], L[2], time);
    } else if (down || (pedal && !in_release)) {
      weight = L[2];
    } else {
      weight = linear_interpolate(up_time, up_weight, up_time + R[3], L[3], time);
      if (weight <= L[3]) weight = L[3];
      if (weight <= 0.0) sounding = false;
      return weight;
    }
  }
  up_time = time;
  up_weight = weight;
  return weight;
}


void DL4R4::pushDown() {
  in_release = false;
  down = true;
  sounding = true;
}
