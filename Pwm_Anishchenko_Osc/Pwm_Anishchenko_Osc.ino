// Stereo Audio PWM - Anishchenko-Astakhov chaotic oscillator example //

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
  float m = 1.42f;
  float g = 0.097f;
  float I = 0.0f;

int audio_callback(void *ctx){

  float nx = x;
  float ny = y;
  float nz = z;
        
  if (x > 0.0f) I = 1.0f;
  if (x <= 0.0f) I = 0.0f;
        
  x = nx + dt * (m * nx + ny - nx * nz);
  y = ny + dt * (- nx);
  z = nz + dt * (- (g * nz) + (g * I) * powf(nx, 2.0f));
     
  double duty_l = 0.4f + (0.1f * x);
  double duty_r = 0.36f + (0.11f * y);

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