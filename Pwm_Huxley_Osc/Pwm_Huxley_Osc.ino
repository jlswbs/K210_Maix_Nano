// Stereo Audio PWM - Hodgkin–Huxley neuron model example //

#include <stdio.h>
#include <syslog.h>
#include <timer.h>
#include <pwm.h>
#include <plic.h>
#include <sysctl.h>
#include <fpioa.h>

#define SAMPLE_RATE 44100
#define TIMER_RATE  1000000000/SAMPLE_RATE
#define PWM_RATE    10*SAMPLE_RATE
#define AUDIO_PIN_L 6
#define AUDIO_PIN_R 8

float mapf(float x, float in_min, float in_max, float out_min, float out_max){ return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min; }

  float iInj = 30.0f;
  float tstep = 0.03f;
  float gBarK = 36.0f;
  float gBarNa = 120.0f;
  float gM = 0.3f;
  float eK = -77.0f;
  float eNa = 50.0f;
  float vRest = -54.4f;
  float v = -65.00f;
  float n = 0.3177f;
  float m = 0.0529f;
  float h = 0.5961f;

float model(){

  float alphan = 0.01f * (v + 55.0f) / (1.0f - expf(-(v + 55.0f) / 10.0f));
  float betan = 0.125f * expf(-(v + 65.0f) / 80.0f);
  float alpham = 0.1f * (v + 40.0f) / (1.0f - expf(-(v + 40.0f) / 10.0f));
  float betam = 4.0f * expf(-(v + 65.0f) / 18.0f);
  float alphah = 0.07f * expf(-(v + 65.0f) / 20.0f);
  float betah = 1.0f / (1.0f + expf(-(v + 35.0f) / 10.0f));
  float dn = (alphan * (1.0f - n)) - (betan * n);
  float dm = alpham * (1.0f - m) - betam * m;
  float dh = alphah * (1.0f - h) - betah * h;
  float dv = -gBarNa * powf(m, 3.0f) * h * (v - eNa) - gBarK * powf(n, 4.0f) * (v - eK) - gM * (v - vRest) + iInj;

  n += (dn * tstep);
  m += (dm * tstep);
  h += (dh * tstep);
  v += (dv * tstep);

  return v;
  
}

int audio_callback(void *ctx){

  double duty_l = mapf(model(), -80.0f, 35.0f, 0.0f, 1.0f);
  double duty_r = duty_l;

  pwm_set_frequency(PWM_DEVICE_1, PWM_CHANNEL_0, PWM_RATE, duty_l);
  pwm_set_frequency(PWM_DEVICE_1, PWM_CHANNEL_1, PWM_RATE, duty_r);

  return 0;
  
}

void setup(){

  srand(read_cycle());
  fpioa_set_function(AUDIO_PIN_L, FUNC_TIMER1_TOGGLE1);
  fpioa_set_function(AUDIO_PIN_R, FUNC_TIMER1_TOGGLE2);
  plic_init();
  sysctl_enable_irq();
  timer_init(TIMER_DEVICE_0);
  timer_set_interval(TIMER_DEVICE_0, TIMER_CHANNEL_0, TIMER_RATE);
  timer_irq_register(TIMER_DEVICE_0, TIMER_CHANNEL_0, 0, 1, audio_callback, NULL);
  timer_set_enable(TIMER_DEVICE_0, TIMER_CHANNEL_0, 1);
  pwm_init(PWM_DEVICE_1);
  pwm_set_enable(PWM_DEVICE_1, PWM_CHANNEL_0, 1);
  pwm_set_enable(PWM_DEVICE_1, PWM_CHANNEL_1, 1);
    
}

void loop(){ 

  
}