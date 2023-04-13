// Stereo Audio PWM - DDS sine example //

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

#define TABLE_BITS  8
#define TABLE_SIZE  (1 << TABLE_BITS)

  uint32_t phase = 0;
  float sinelut[TABLE_SIZE+1];

float lut_sinf(uint32_t phase){
  
	uint32_t quad_phase;
	int table_loc;
	float x, y[2];

  union {
		float f;
		uint32_t i;
	} out;  

	quad_phase = (phase & 0x40000000) ? ~phase : phase;
	table_loc = (quad_phase & 0x3FFFFFFF) >> (30 - TABLE_BITS);
	y[0] = sinelut[table_loc];
	y[1] = sinelut[table_loc + 1];
	x = (quad_phase & ((1 << (30 - TABLE_BITS)) - 1)) / (float) (1 << (30 - TABLE_BITS));
	out.f = y[0] + ((y[1] - y[0]) * x);
	out.i |= phase & 0x80000000;
	return 0.5f + (0.5f * out.f);

}

int audio_callback(void *ctx){
     
  double duty_l =  lut_sinf(phase);
  double duty_r =  duty_l;

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

  for (int i = 0; i < TABLE_SIZE+1; i++) sinelut[i] = sinf(M_PI_2 * (i / (float) TABLE_SIZE));
    
}

void loop(){

  phase = phase + 500000;
  
}