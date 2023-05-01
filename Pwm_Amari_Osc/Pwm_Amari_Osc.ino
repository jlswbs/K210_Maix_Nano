// Stereo Audio PWM - Amari oscillator example //

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

  float u = 0.0f;
  float s_u = 0.0f;
  float h_u = 0.0f;
  float c_uu = 1.5f;
  float d_u = 0.0f;
  float v = 0.0f;
  float s_v = 0.0f;
  float h_v = -1.0f;
  float c_vv = 0.0f;
  float d_v = 0.0f;
  float c_uv = -1.5f;
  float c_vu = 1.5f;
  float bt = 20.0f;
  float dt = 0.01f;
  float c_n = 0.1f;

float sigmoid(float x, float bt){ return 1.0f/(1.0f + expf(-x*bt)); }

int audio_callback(void *ctx){

  d_u = dt * (- u + s_u + h_u + c_uu * sigmoid(u, bt) + c_uv * sigmoid(v, bt) + c_n);
  u = u + d_u;
  d_v = dt * (- v + s_v + h_v + c_vv * sigmoid(v, bt) + c_vu * sigmoid(u, bt));
  v = v + d_v;
     
  double duty_l = 0.3f + (0.9f * u);
  double duty_r = 0.52f + (0.96f * v);

  pwm_set_frequency(PWM_DEVICE_1, PWM_CHANNEL_0, PWM_RATE, duty_l);
  pwm_set_frequency(PWM_DEVICE_1, PWM_CHANNEL_1, PWM_RATE, duty_r);

  return 0;
  
}

void setup(){

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