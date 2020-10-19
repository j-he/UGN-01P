#include "daisysp.h"
#include "daisy_patch.h"
#include <string>

using namespace daisy;
using namespace daisysp;

DaisyPatch patch;
Metro int_clk;

bool inputs[4];
long lfsr_bit = 0;
float current = 0;
bool isInverted[4];
int tapPosition[4];
int t1, t2, t3, t4;
Parameter tapPos1, tapPos2, tapPos3, tapPos4;
float int_clkFreq;
int steps = 15;
int len = 15;

long lfsr = 0xACE1;
long lockedLFSR = 0;

//menu logic
int menuPos; //6 positions
bool inSubMenu; //only on gate types
int cursorPos[3]; //x positions for the OLED cursor
bool edit;
bool isExtClk;
bool isLocked;
bool setLock;

int mod(int x, int m) { return (x % m + m) % m; }

void NextSamples();

void ProcessControls();
void ProcessOled();
void ProcessOutputs();

static void AudioCallBack(float **in, float **out, size_t size)
{
    float sig = 0;
    int t = 0;
    float r = 0.0;
    ProcessControls();

    //if (patch.gate_input[0].Trig())
    for (size_t i = 0; i < size; i+= 2)
    {
      if (isExtClk) {
        // gate in interrupt will handle
      } else if (int_clk.Process()){
        NextSamples();
        for (int i = 0; i < 4; i++) {
          // t = lfsr >> tapPosition[i] & 0x0001 ? 1.0 : 0.0;
          t = lfsr >> tapPosition[i] & 0x0001;
        }
      }
      for (size_t chn = 0; chn < 4; chn++)
      {
          int ti = (in[chn][i]+1.0f) * 0.5f * 0xACE1;
          r = ti & t ? 1.0 : 0.0;
          sig = 0.5 * r; //* (lfsr >> tapPosition[i] & 0x0001);
          out[chn][i] = sig;
      }
    }
    //if (int_clk.Process())
    //{
    //  lfsr_bit = ((lfsr >> 0) ^ (lfsr >> t1) ^ (lfsr >> t2) ^ (lfsr >> t3) ^ (lfsr >> t4) ) & 0x0001;
    //  lfsr = (lfsr >> 1) | (lfsr_bit << 15);
    //  current = lfsr_bit;
    //}
}

void InitCursorPos()
{
    cursorPos[0] = 5; //Clock-source
    cursorPos[1] = 23; // Tempo
    cursorPos[2] = 48; //len
    // cursorPos[3] = 75; //Steps
    // cursorPos[4] = 92;
    // cursorPos[5] = 117;
}

void InitTapPos()
{
  tapPosition[0] = 1;
  tapPosition[1] = 7;
  tapPosition[2] = 11;
  tapPosition[3] = 15;
}

int main(void)
{
    patch.Init(); // Initialize hardware (daisy seed, and patch)
    float samplerate = patch.AudioSampleRate();
    int_clkFreq = 10.f;
    edit = false;
    isExtClk = false;
    menuPos = 0;
    isLocked = false;
    setLock = false;

    InitCursorPos();
    InitTapPos();

    tapPos1.Init(patch.controls[patch.CTRL_1], 1.0f, 15.0f, Parameter::LINEAR);
    tapPos2.Init(patch.controls[patch.CTRL_2], 1.0f, 15.0f, Parameter::LINEAR);
    tapPos3.Init(patch.controls[patch.CTRL_3], 1.0f, 15.0f, Parameter::LINEAR);
    tapPos4.Init(patch.controls[patch.CTRL_4], 1.0f, 15.0f, Parameter::LINEAR);

    int_clk.Init(int_clkFreq, samplerate);

    patch.StartAdc();
    patch.StartAudio(AudioCallBack);
    while(1)
    {
      ProcessControls();
      ProcessOutputs();
      ProcessOled();
    }
}

void ProcessIncrement(int increment)
{
    if (edit) {
      if(menuPos == 0) {
        int_clkFreq += increment;
        int_clk.SetFreq(int_clkFreq);
      } else if (menuPos == 2) {
          len += increment;
      }
    } else {
      menuPos = (menuPos + increment) % 5;
    }
}

void ProcessRisingEdge()
{
    if(patch.encoder.RisingEdge()) {
      if (menuPos == 0 || menuPos == 2)
      {
        edit = !edit;
      } else if (menuPos == 1) {
        isExtClk = !isExtClk;
      }
    }
}

void ProcessEncoder()
{
    ProcessIncrement(patch.encoder.Increment());

    if (patch.encoder.RisingEdge())
    {
        ProcessRisingEdge();
    }

    if (patch.encoder.TimeHeldMs() > 1000) {
        // isLocked = !isLocked;
        setLock = true;
    }
    if (patch.encoder.FallingEdge() && setLock){
      isLocked = !isLocked;
      setLock = false;
      if (isLocked) {
        lockedLFSR = lfsr;
      }
    }
}

void ProcessGates()
{
    for (int i = 0 ; i < 2; i ++)
    {
      // check gate_input and do something
      if (i == 1) {
        if (patch.gate_input[i].Trig() && isExtClk)
        {
          // advance clock and step forward;
          NextSamples();
        }
      } else {
        if(patch.gate_input[i].Trig()){
          lfsr = rand() % 0xACE1;
        }
      }

    }
}

void ProcessControls()
{
    patch.UpdateAnalogControls();
    patch.DebounceControls();

    ProcessEncoder();

    tapPosition[0] = tapPos1.Process();
    tapPosition[1] = tapPos2.Process();
    tapPosition[2] = tapPos3.Process();
    tapPosition[3] = tapPos4.Process();
    // ProcessGates();
}

void ProcessOutputs()
{
  dsy_dac_write(DSY_DAC_CHN1, lfsr >> 4);
  dsy_dac_write(DSY_DAC_CHN2, ~(lfsr >> 4));
  dsy_gpio_write(&patch.gate_output, lfsr_bit);
    // dsy_dac_write(DSY_DAC_CHN2, gates[1].Process(inputs[2], inputs[3]) * 4095);
}

void ProcessOled()
{
    // oled width 110
    // oled height 64

    // draw OLED here
    patch.display.Fill(false);

    std::string str;
    char* cstr = &str[0];

    patch.display.SetCursor(0,0);
    str = "UGN-01P";
    patch.display.WriteString(cstr, Font_6x8, true);

    patch.display.SetCursor(90, 0);
    str = edit ? "o" : " ";
    patch.display.WriteString(cstr, Font_6x8, true);

    for (int j = 0; j < 4; j++)
    {
      str = "v";
      patch.display.SetCursor(7*tapPosition[j], 15);
      patch.display.WriteString(cstr, Font_6x8, true);
    }

    for (int i = 0; i < 16; i++)
    {
      int state = (int) lfsr >> i & 0x0001;
      if (state > 0) {
        str = "+";
      } else {
        str = "-";
      }
      patch.display.SetCursor(7*i, 25);
      patch.display.WriteString(cstr, Font_6x8, true);
    }

    if (menuPos == 0) {
      str = "T";
    } else if (menuPos == 1){
      if (isExtClk) {
        str = "E";
      } else {
        str = "I";
      }
    } else if (menuPos == 2){
      str = "S";
    } else {
      // str = "o";
    }

    //Cursor
    patch.display.SetCursor(cursorPos[menuPos], 45);
    patch.display.WriteString(cstr, Font_6x8, true);

    if (isLocked){
      patch.display.SetCursor(70, 45);
      str = "L";
      patch.display.WriteString(cstr, Font_6x8, true);
    }

    patch.display.SetCursor(92, 45);
    std::string numberAsString = std::to_string(len+1);
    str = numberAsString;
    patch.display.WriteString(cstr, Font_6x8, true);

    patch.display.Update();
}

void NextSamples()
{
  if (lfsr <= 0) {
    lfsr = rand() % 0xACE1;
  }
  lfsr_bit = ((lfsr >> 0) ^ (lfsr >> tapPosition[0]) ^ (lfsr >> tapPosition[1]) ^ (lfsr >> tapPosition[2]) ^ (lfsr >> tapPosition[3]) ) & 0x0001;
  lfsr = (lfsr >> 1) | (lfsr_bit << 15);
  current = lfsr_bit;
  if (isLocked) {
    steps ++;
    if (steps > len) {
      lfsr = lockedLFSR;
      steps = 0;
    }
  }
}
