// Stereo Audio PWM template //

#include <stdio.h>
#include <syslog.h>
#include <timer.h>
#include <pwm.h>
#include <plic.h>
#include <sysctl.h>
#include <fpioa.h>

#define SAMPLE_RATE 44100
#define TIMER_RATE  1000000000/SAMPLE_RATE
#define PWM_RATE    1000000
#define AUDIO_PIN_L 6
#define AUDIO_PIN_R 8

uint8_t out_l, out_r;

int audio_callback(void *ctx){
     
  double duty_l =  out_l / 256.0f;
  double duty_r =  out_r / 256.0f;  

  pwm_set_frequency(PWM_DEVICE_1, PWM_CHANNEL_0, PWM_RATE, duty_l);
  pwm_set_frequency(PWM_DEVICE_1, PWM_CHANNEL_1, PWM_RATE, duty_r);

  out_l++;
  out_r++;
 
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