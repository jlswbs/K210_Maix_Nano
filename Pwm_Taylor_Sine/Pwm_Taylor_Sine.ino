// Stereo Audio PWM - Taylor sine example //

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

  float delta = -PI;

float sintaylor(float angle){
  
  float a2 = angle * angle;
  float x = angle;

  float rv = x;

  x = x * (1.0f / 6.0f) * a2;
  rv -= x;

  x = x * (1.0f / 20.0f) * a2;
  rv += x;

  x = x * (1.0f / 42.0f) * a2;
  rv -= x;

  x = x * (1.0f / 72.0f) * a2;
  rv += x;

  x = x * (1.0f / 110.0f) * a2;
  rv -= x;

  return rv;

}

int audio_callback(void *ctx){
     
  double duty_l =  0.5f + (0.5f * sintaylor(delta));
  double duty_r =  duty_l;

  pwm_set_frequency(PWM_DEVICE_1, PWM_CHANNEL_0, PWM_RATE, duty_l);
  pwm_set_frequency(PWM_DEVICE_1, PWM_CHANNEL_1, PWM_RATE, duty_r);

  if (delta >= PI) delta = -PI;
  delta = delta + 0.01f;
 
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