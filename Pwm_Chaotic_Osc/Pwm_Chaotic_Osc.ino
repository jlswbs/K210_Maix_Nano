// Stereo Audio PWM - Chaotic oscillator example //

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

  float x = 1.0f;
  float y = 1.0f;
  float z = 1.0f;
  float dt = 0.005f;
  float a = 0.6f;
  float b = 30.0f;
  float c = 0.00000004f;
  float d = 0.13f;

int audio_callback(void *ctx){

  float nx = x;
  float ny = y;
  float nz = z;
    
  x = nx + dt * ny;
  y = ny + dt * (a * ny - nx - nz);
  z = nz + dt * ((b + ny - c * (expf(nz - 1.0f))) / d);
     
  double duty_l = 0.44f + (0.006f * x);
  double duty_r = 0.5f + (0.006f * y);

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