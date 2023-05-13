// Stereo Audio PWM - Chromosome oscillations in mitosis example //

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

  float x = 38.0f;
  float y = 11.0f;
  float dt = 0.007f;
  float N = 100.0f;
  float ku = 1.65f;
  float ns = 11.5f;
  float neta = 5.2f;
  float V0 = 2.38f / 60.0f;
  float f = 2.0f;

int audio_callback(void *ctx){

  float nx = x;
  float ny = y;
  float kbr = 266.0f / powf(y, 2.0f);
    
  x = nx + dt * (kbr * (N - nx) - ku * expf(f * (ns+neta) / (nx+neta)) * nx);
  y = ny + dt * (V0 * (nx - ns) / (nx + neta));
     
  double duty_l = 0.07f + (0.015f * x);
  double duty_r = -1.3f + (0.165f * y);

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