// Stereo Audio PWM - FM synthesizer music //

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

const int STATE_ATTACK = 0;
const int STATE_DECAY = 1;
const int STATE_SUSTAIN = 2;
const int STATE_RELEASE = 3;
const int STATE_SLEEP = 4;
const int WAVE_BUFFER_BITS = 8;
const int INT_BITS = 32;
const int FIXED_BITS = 14;
const int FIXED_BITS_ENV = 8;
const int WAVE_ADDR_SHIFT = (INT_BITS - WAVE_BUFFER_BITS);
const int WAVE_ADDR_SHIFT_M = (WAVE_ADDR_SHIFT - FIXED_BITS);
const int FIXED_SCALE = (1 << FIXED_BITS);
const int FIXED_SCALE_M1 = (FIXED_SCALE - 1);
const int WAVE_BUFFER_SIZE = (1 << WAVE_BUFFER_BITS);
const int WAVE_BUFFER_SIZE_M1 = (WAVE_BUFFER_SIZE - 1);
const int OPS = 4;
const int CHANNELS = 4;
const int OSCS = (OPS * CHANNELS);
const int TEMPO = 8192;
const int SEQ_LENGTH = 16;
const int SAMPLE_US = 40;
const int ADD_RATE = 6;
const int DEL_RATE = 12;
const int OCT_MIN = 6;
const int OCT_WIDTH = 2;

const int CONST03 = (1 << FIXED_BITS << FIXED_BITS_ENV);
const int CONST04 = (FIXED_SCALE >> 1);
const int VOLUME = (CONST04 / CHANNELS);
const int REVERB_VOLUME = (int)(VOLUME * 1.9);
const int REVERB_BUFFER_SIZE_L = 0x4000;
const int REVERB_BUFFER_SIZE_R = 0x2000;
const int REVERB_LENGTH_L = (int)(REVERB_BUFFER_SIZE_L * 0.99);
const int REVERB_LENGTH_R = (int)(REVERB_BUFFER_SIZE_R * 0.91);
const int REVERB_DECAY = (int)(FIXED_SCALE * 0.7);

typedef struct
{
  int envelopeLevelA;
  int envelopeLevelS;
  int envelopeDiffA;
  int envelopeDiffD;
  int envelopeDiffR;
  int modPatch0;
  int modPatch1;
  int modLevel0;
  int modLevel1;
  int levelL;
  int levelR;
  int levelRev;

  int state;
  int count;
  int currentLevel;
  int pitch;
  int velocity;
  int mod0;
  int mod1;
  int outData;
  int outWaveL;
  int outWaveR;
  int outRevL;
  int outRevR;
  boolean mixOut;
  boolean noteOn;
  boolean noteOnSave;
  boolean goToggle;
  boolean outDoneToggle;
} params_t;

typedef struct
{
  unsigned char note;
  unsigned char oct;
} seqData_t;

short waveData[] = {
  0,402,803,1205,1605,2005,2403,2800,
  3196,3589,3980,4369,4755,5139,5519,5896,
  6269,6639,7004,7365,7722,8075,8422,8764,
  9101,9433,9759,10079,10393,10700,11002,11296,
  11584,11865,12139,12405,12664,12915,13158,13394,
  13621,13841,14052,14254,14448,14633,14810,14977,
  15135,15285,15425,15556,15677,15789,15892,15984,
  16068,16141,16205,16259,16304,16338,16363,16378,
  16383,16378,16363,16338,16304,16259,16205,16141,
  16068,15984,15892,15789,15677,15556,15425,15285,
  15135,14977,14810,14633,14448,14254,14052,13841,
  13621,13394,13158,12915,12664,12405,12139,11865,
  11584,11296,11002,10700,10393,10079,9759,9433,
  9101,8764,8422,8075,7722,7365,7004,6639,
  6269,5896,5519,5139,4755,4369,3980,3589,
  3196,2800,2403,2005,1605,1205,803,402,
  0,-403,-804,-1206,-1606,-2006,-2404,-2801,
  -3197,-3590,-3981,-4370,-4756,-5140,-5520,-5897,
  -6270,-6640,-7005,-7366,-7723,-8076,-8423,-8765,
  -9102,-9434,-9760,-10080,-10394,-10701,-11003,-11297,
  -11585,-11866,-12140,-12406,-12665,-12916,-13159,-13395,
  -13622,-13842,-14053,-14255,-14449,-14634,-14811,-14978,
  -15136,-15286,-15426,-15557,-15678,-15790,-15893,-15985,
  -16069,-16142,-16206,-16260,-16305,-16339,-16364,-16379,
  -16383,-16379,-16364,-16339,-16305,-16260,-16206,-16142,
  -16069,-15985,-15893,-15790,-15678,-15557,-15426,-15286,
  -15136,-14978,-14811,-14634,-14449,-14255,-14053,-13842,
  -13622,-13395,-13159,-12916,-12665,-12406,-12140,-11866,
  -11585,-11297,-11003,-10701,-10394,-10080,-9760,-9434,
  -9102,-8765,-8423,-8076,-7723,-7366,-7005,-6640,
  -6270,-5897,-5520,-5140,-4756,-4370,-3981,-3590,
  -3197,-2801,-2404,-2006,-1606,-1206,-804,-403,
};

int scaleTable[] = {
  153791,162936,172624,182889,193764,205286,217493,230426,
  244128,258644,274024,290319,307582,325872,345249,365779,
};

const int CHORD_LENGTH = 5;
const int CHORDS = 3;
unsigned char chordData[CHORDS][8] = {
  {0,2,4,7,9,0,0,0},
  {0,2,4,7,9,0,0,0},
  {0,2,4,7,9,0,0,0}
};
unsigned char progressionData[CHORDS][2] = {
  {0, 3},
  {0, 3},
  {0, 3}
};
unsigned char bassData[CHORDS] = {4, 2, 1};
unsigned char bassPattern[SEQ_LENGTH] = {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
unsigned char toneData[CHANNELS][OPS];

params_t params[OSCS];
seqData_t seqData[OSCS][SEQ_LENGTH];
bool activeCh[OSCS];

short reverbBufferL[REVERB_BUFFER_SIZE_L];
short reverbBufferR[REVERB_BUFFER_SIZE_R];

int counter;
int seqCounter;
int barCounter;
int deleteCounter;
int chord;
int note;
int sc;
int reverbAddrL;
int reverbAddrR;
int outL;
int outR;

void render()
{
  int mixL = 0;
  int mixR = 0;
  int mixRevL = 0;
  int mixRevR = 0;

  for (int i = 0; i < OSCS; i++)
  {
    if (activeCh[i] == true)
    {
      // envelope generator
      switch (params[i].state)
      {
        case STATE_SLEEP:
        {
          break;
        }

        case STATE_ATTACK:
        {
          params[i].currentLevel += params[i].envelopeDiffA;
          if (params[i].currentLevel > params[i].envelopeLevelA)
          {
            params[i].currentLevel = params[i].envelopeLevelA;
            params[i].state = STATE_DECAY;
          }
          break;
        }

        case STATE_DECAY:
        {
          params[i].currentLevel += params[i].envelopeDiffD;
          if (params[i].currentLevel < params[i].envelopeLevelS)
          {
            params[i].currentLevel = params[i].envelopeLevelS;
            params[i].state = STATE_SUSTAIN;
          }
          break;
        }

        case STATE_SUSTAIN:
        {
          break;
        }

        case STATE_RELEASE:
        {
          params[i].currentLevel += params[i].envelopeDiffR;
          if (params[i].currentLevel < 0)
          {
            params[i].currentLevel = 0;
            activeCh[i] = false;
            params[i].state = STATE_SLEEP;
          }
          break;
        }
      }

      params[i].mod0 = params[params[i].modPatch0].outData;
      params[i].mod1 = params[params[i].modPatch1].outData;

      int waveAddr = (unsigned int)(params[i].count +
                                    (params[i].mod0 * params[i].modLevel0) +
                                    (params[i].mod1 * params[i].modLevel1)) >> WAVE_ADDR_SHIFT_M;

      // fetch wave data
      int waveAddrF = waveAddr >> FIXED_BITS;
      int waveAddrR = (waveAddrF + 1) & WAVE_BUFFER_SIZE_M1;    
      int oscOutF = waveData[waveAddrF];
      int oscOutR = waveData[waveAddrR];
      int waveAddrM = waveAddr & FIXED_SCALE_M1;
      int oscOut = ((oscOutF * (FIXED_SCALE - waveAddrM)) >> FIXED_BITS) +
        ((oscOutR * waveAddrM) >> FIXED_BITS);
      params[i].outData = (oscOut * (params[i].currentLevel >> FIXED_BITS_ENV)) >> FIXED_BITS;
      params[i].count += params[i].pitch;

      // mix
      if (params[i].mixOut == 0)
      {
        params[i].outWaveL = 0;
        params[i].outWaveR = 0;
        params[i].outRevL = 0;
        params[i].outRevR = 0;
      }
      else
      {
        params[i].outWaveL = (params[i].outData * params[i].levelL) >> FIXED_BITS;
        params[i].outWaveR = (params[i].outData * params[i].levelR) >> FIXED_BITS;
        params[i].outRevL = (params[i].outWaveL * params[i].levelRev) >> FIXED_BITS;
        params[i].outRevR = (params[i].outWaveR * params[i].levelRev) >> FIXED_BITS;
      }

      mixL += params[i].outWaveL;
      mixR += params[i].outWaveR;
      mixRevL += params[i].outRevL;
      mixRevR += params[i].outRevR;
    }
  }

  // reverb
  int reverbL = ((int)reverbBufferR[reverbAddrR] * REVERB_DECAY) >> FIXED_BITS;
  int reverbR = ((int)reverbBufferL[reverbAddrL] * REVERB_DECAY) >> FIXED_BITS;
  reverbL += mixRevR;
  reverbR += mixRevL;
  reverbBufferL[reverbAddrL] = reverbL;
  reverbBufferR[reverbAddrR] = reverbR;
  reverbAddrL++;
  if (reverbAddrL > REVERB_LENGTH_L)
  {
    reverbAddrL = 0;
  }
  reverbAddrR++;
  if (reverbAddrR > REVERB_LENGTH_R)
  {
    reverbAddrR = 0;
  }
  outL = 32768 + (mixL + reverbBufferL[reverbAddrL]);
  outR = 32768 + (mixR + reverbBufferR[reverbAddrR]);
}

int audio_callback(void *ctx){

  counter++;
  if (counter > TEMPO)
  {
    counter = 0;
    if (seqCounter >= SEQ_LENGTH)
    {
      seqCounter = 0;
      if ((barCounter & 3) == 0)
      {
        // tone change
        for (int i = 0; i < CHANNELS; i++)
        {
          int osc = i * OPS;
          params[osc].modLevel0 = FIXED_SCALE * random(5);
          params[osc + 1].modLevel0 = FIXED_SCALE * random(4);
          for (int j = 0; j < OPS; j++)
          {
            osc = i * OPS + j;
            params[osc].envelopeDiffA = CONST03 >> (random(10) + 3);
            params[osc].envelopeDiffD = -CONST03 >> (random(9) + 13);
            toneData[i][j] = random(3) - 1;
          }
        }
        // chord change
        chord = random(progressionData[chord][1]) + progressionData[chord][0];
        // delete rate change
        deleteCounter = random(DEL_RATE);
      }
      // delete note
      for (int i = 0; i < deleteCounter; i++)
      {
        seqData[random(CHANNELS)][random(SEQ_LENGTH)].oct = 0;
      }
      // add note
      for (int i = 0; i < ADD_RATE; i++)
      {
        int ch = random(CHANNELS);
        int beat = random(SEQ_LENGTH);
        int beat_prev = beat - 1;
        if (beat_prev < 0)
        {
          beat_prev += SEQ_LENGTH;
        }
        seqData[ch][beat].note = random(CHORD_LENGTH);
        seqData[ch][beat].oct = random(OCT_WIDTH) + OCT_MIN;
        seqData[ch][beat_prev].oct = 0;
      }
      // add bass
      for (int i = 0; i < SEQ_LENGTH; i++)
      {
        if (bassPattern[i] == 1)
        {
          seqData[0][i].note = bassData[chord];
          seqData[0][i].oct = OCT_MIN;
        }
      }
      barCounter++;
    }
    // note on/off
    for (int i = 0; i < CHANNELS; i++)
    {
      if (seqData[i][seqCounter].oct != 0)
      {
        int n = chordData[chord][seqData[i][seqCounter].note];
        for (int j = 0; j < OPS; j++)
        {
          int osc = i * OPS + j;
          params[osc].pitch = scaleTable[n] << (seqData[i][seqCounter].oct + toneData[i][j]);
          params[osc].state = STATE_ATTACK;
          params[osc].noteOn = true;
          activeCh[osc] = true;
        }
      }
      else
      {
        for (int j = 0; j < OPS; j++)
        {
          int osc = i * OPS + j;
          if (params[osc].noteOn == true)
          {
            params[osc].state = STATE_RELEASE;
            params[osc].noteOn = false;
          }
        }
      }
    }
    seqCounter++;
  }

  render();
       
  double duty_l =  outL / 65536.0f;
  double duty_r =  outR / 65536.0f;

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

  counter = 0;
  seqCounter = 0;
  barCounter = 0;
  deleteCounter = 0;
  chord = 0;
  note = 0;
  sc = 0;
  reverbAddrL = 0;
  reverbAddrR = 0;

  for (int i = 0; i < OSCS; i++)
  {
    activeCh[i] = false;
    params[i].state = STATE_SLEEP;
    params[i].envelopeLevelA = CONST03;
    params[i].envelopeLevelS = 0;
    params[i].envelopeDiffA = CONST03 >> (random(9) + 4);
    params[i].envelopeDiffD = (0 - CONST03) >> (random(9) + 13);
    params[i].envelopeDiffR = (0 - CONST03) >> 13;
    params[i].levelL = VOLUME;
    params[i].levelR = VOLUME;
    params[i].levelRev = REVERB_VOLUME;
    params[i].mixOut = true;
    params[i].modLevel0 = FIXED_SCALE * random(5);
    params[i].modPatch0 = i;
    params[i].modPatch1 = i;
    params[i].modLevel1 = 0;
  }
  for (int i = 0; i < CHANNELS; i++)
  {
    int osc = i * OPS;
    params[osc].modPatch0 = osc + 1;
    params[osc + 1].modPatch0 = osc + 1;
    params[osc + 1].modLevel0 = 0;
    params[osc + 1].mixOut = false;
  }
  params[4].levelL = VOLUME >> 1;
  params[4].levelR = VOLUME;
  params[6].levelL = VOLUME;
  params[6].levelR = VOLUME >> 1;

  for (int i = 0; i < CHANNELS; i++)
  {
    for (int j = 0; j < OPS; j++)
    {
      toneData[i][j] = 0;
    }
  }

  for (int i = 0; i < 3; i++)
  {
    bassPattern[random(12)] = 1;
  }
  
    
}

void loop(){


  
}