// Stereo Audio PWM - two Karplus-Strong + LP filter + drive //

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

#define SIZE 1024
#define OFFSET 64

  float out1 = 0.0f;
  float last1 = 0.0f;
  float curr1 = 0.0f;
  float tmp1 = 0.0f;
  float delaymem1[SIZE];
  int locat1 = 0;
  int bound1 = SIZE;
  float LPF_Beta1 = 0.0f;

  float out2 = 0.0f;
  float last2 = 0.0f;
  float curr2 = 0.0f;
  float tmp2 = 0.0f;
  float delaymem2[SIZE];
  int locat2 = 0;
  int bound2 = SIZE;
  float LPF_Beta2 = 0.0f;
  float m_drive = 0.0f;

float Drive(float in, float m_drive){

  if (m_drive == 1.0f) return in;
  if (in >= 1.0f) return 1.0f;
  if (in > 0.0f) return (1.0f - powf(1.0f - in, m_drive));
  if (in >= -1.0f) return (powf(1.0f + in, m_drive) - 1.0f);
  else return -1.0f;
  
}

float randomf(float minf, float maxf) {return minf + random(1UL << 31)*(maxf - minf) / (1UL << 31);}

int audio_callback(void *ctx){
     
  double duty_l = Drive((out1+out2)/2.0f, m_drive);
  double duty_r = duty_l;

  pwm_set_frequency(PWM_DEVICE_1, PWM_CHANNEL_0, PWM_RATE, duty_l);
  pwm_set_frequency(PWM_DEVICE_1, PWM_CHANNEL_1, PWM_RATE, duty_r);

  delaymem1[locat1++] = out1;
  if (locat1 >= bound1) locat1 = 0;
  curr1 = delaymem1[locat1];
  tmp1 = 0.5f * (last1 + curr1);
  last1 = curr1;
  out1 = out1 - (LPF_Beta1 * (out1 - tmp1));
     
  delaymem2[locat2++] = out2;
  if (locat2 >= bound2) locat2 = 0;
  curr2 = delaymem2[locat2];
  tmp2 = 0.5f * (last2 + curr2);
  last2 = curr2;
  out2 = out2 - (LPF_Beta2 * (out2 - tmp2));  

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

  for (int i = 0; i < SIZE; i++) delaymem1[i] = randomf(0.000f, 0.999f);
  bound1 = random(OFFSET, SIZE);
  LPF_Beta1 = randomf(0.009f, 0.999f);

  delay(120);

  for (int i = 0; i < SIZE; i++) delaymem2[i] = randomf(0.000f, 0.999f);
  bound2 = random(OFFSET, SIZE);
  LPF_Beta2 = randomf(0.009f, 0.999f);
  m_drive = randomf(0.199f, 1.999f);
  
  delay(240);
  
}