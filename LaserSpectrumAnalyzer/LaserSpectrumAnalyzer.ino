/* 
This is a Laser Spectum Analyzer, done by Florian Link (at) gmx.de

Note: It works best (without sparkling dots) with my patch on the Audio library,
so that the FFT is only calculated when it is being read.

You can get the patched version here:
https://github.com/florianlink/Audio


It is based on the following:
 
Led matrix array Spectrum Analyser
22/11/2015 Nick Metcalfe
Takes a mono analog RCA audio input (nominal 1v pp) and presents a 64-band
real-time colour spectrum analyser display with peak metering.

Built with Teensy 3.2 controller
https://www.pjrc.com/store/teensy32.html
and the Teensy Audio Library
http://www.pjrc.com/teensy/td_libs_Audio.html

Portions of code based on:

Logarithmic band calculations http://code.compartmental.net/2007/03/21/fft-averages/

Copyright (c) 2015 Nick Metcalfe  valves@alphalink.com.au
Copyright (c) 2016 Florian Link
All right reserved.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.
This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.
You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

// Enable this if you have my patched version of the Audio library
// (get patched version here: https://github.com/florianlink/Audio)
//#define PATCHED_AUDIO

#include <analyze_fft1024.h>
#include <input_adc.h>

AudioInputAnalog         adc1(A1);        //xy=144,119
AudioAnalyzeFFT1024      fft1024_1;       //xy=498,64
AudioConnection          patchCord1(adc1, fft1024_1);

const int m_amountOfRows = 256;
const int m_amountOfColumns = 90; //128;  //Number of outputs per row
unsigned int logGen1024[m_amountOfColumns][2] = {0}; //Linear1024 -> Log64 mapping
float m_bandVal[m_amountOfColumns] = {0.0};      //band FP value
const unsigned int logAmpSteps = 256;             //How many steps in the log amp converter
int logAmpOffset[logAmpSteps] = {0};             //Linear64 -> Log16 mapping

unsigned char m_bands[m_amountOfColumns] = {0};  //band pixel height
unsigned char m_peaks[m_amountOfColumns] = {0};  //peak pixel height
float m_peakVal[m_amountOfColumns] = {0.0};      //peak FP value

//------------------------------------------------------------------------------------------------
// Logarithmic band calculations
// http://code.compartmental.net/2007/03/21/fft-averages/

//Changing the gain changes the overall height of the bars in the display 
//by a proportionate amount. Note that increasing the gain for low-level 
//signals also increases background noise to a point where it may start to 
//show along the bottom of the display. m_shift can hide this.
const float m_gain = 340.0;                       //Input gain

//Shifts the bars down to conceal the zero-point as well as any additional 
//background noise. If random data shows at the very bottom of the display 
//when no input is present, increase this value by steps of 0.1 to just past 
//the point where it disappears. This should be 0.1 in a good quiet design.
const float m_shift = 1.;                       //Shift bars down to mask background noise

//Controls how fast each spectrum bar shrinks back down. Larger numbers are faster.
float m_decay = 1.0;                      //Speed of band decay

//Enable showing the red peak trace. Turn it off for a pure spectrum display.
const bool m_showPeak = true;                    //Show peaks

//How many 20ms cycles the peak line holds on the display before resetting. As the
//peak timer is restarted by peaks being nudged up, this should remain short or 
//the peak display tends to get stuck.
const int m_peakCounter = 5;

//How many pixels of spectrum need to show on the display for the peak line to appear.
//This hides the peak when no input is present in order to blank and save the display.
const int m_peakDisplayThreshold = 12;           //Minimum number of pixels under peak for it to show

//The noise gate mutes the input when the peak is below m_peakDisplayThreshold. This
//can be used to conceal narrow band background noise.
const bool m_noiseGate = true;

const int sampleRate = 16384;
const int timeSize = 1024;    //FFT points
const int bandShift = 2;      //First two bands contain 50hz and display switching noise. Hide them..

//Shape of spread of frequency steps within the passband. Shifts the middle part left
//or right for visual balance. 0.05 for a hard logarithmic response curve to 0.40 for 
//a much more linear curve on two displays.
const float logScale = 0.14;  //Scale the logarithm curve deeper or shallower

int freqToIndex(int freq);

//Calculate a logarithmic set of band ranges
void calcBands(void) {
  int bandOffset;     //Bring us back toward zero for larger values of logScale
  for (int i = 0; i < m_amountOfColumns; i++)
  {
    int lowFreq = (int)((sampleRate/2) / (float)pow(logScale + 1, m_amountOfColumns - i)) - 1;
    int hiFreq = (int)((sampleRate/2) / (float)pow(logScale + 1, m_amountOfColumns - 1 - i)) - 1;
    int lowBound = freqToIndex(lowFreq);
    int hiBound = freqToIndex(hiFreq);
    if (i == 0) bandOffset = lowBound;
    lowBound -= bandOffset;
    hiBound -= bandOffset + 1;
    if (lowBound < i + bandShift) lowBound = i + bandShift;
    if (hiBound < i + bandShift) hiBound = i + bandShift;
    if (lowBound > hiBound) lowBound = hiBound;
    if (i == m_amountOfColumns - 1) hiBound = 511;
    logGen1024[i][0] = lowBound;
    logGen1024[i][1] = hiBound;
#ifdef SERIAL_DEBUG
    Serial.print(i);
    Serial.print(" - Bounds:");
    Serial.print(lowBound);
    Serial.print(", ");
    Serial.println(hiBound);
#endif
  }
}

//Determine the FFT sample bandwidth
float getBandWidth()
{
  return (2.0/(float)timeSize) * (sampleRate / 2.0);
}

//Convert a frequency to a FFT bin index 
int freqToIndex(int freq)
{
  // special case: freq is lower than the bandwidth of spectrum[0]
  if ( freq < getBandWidth()/2 ) return 0;
  // special case: freq is within the bandwidth of spectrum[512]
  if ( freq > sampleRate/2 - getBandWidth()/2 ) return (timeSize / 2) - 1;
  // all other cases
  float fraction = (float)freq/(float) sampleRate;
  int i = (int)(timeSize * fraction);
  return i;
}

//Calculate a logarithmic amplitude lookup table
void calcAmplitudes() {
  for (int i = 0; i < logAmpSteps; i++)
  {  
    float db = 1.0 - ((float) i / (float)logAmpSteps);
    db = (1.0 - (db * db)) * (m_amountOfRows + 1);
    if (db < 0) logAmpOffset[i] = -1;    
    else logAmpOffset[i] = (int)db;
#ifdef SERIAL_DEBUG
    Serial.print(i);
    Serial.print(" - Amp:");
    Serial.println(logAmpOffset[i]);
#endif
  }
}

int maxBeatValue = 0;

void updateFFT() {
  static int peakCount = 0;      //Peak delay counter
  int barValue, barPeak;         //current values for bar and peak
  float maxPeak = 0;             //Sum of all peak values in frame
  static bool drawPeak = true;   //Show peak on display
  //interrupts();
  // some time to gather the FFT
  //hold(6000);
  if (fft1024_1.available())
  {
    for (int band = 0; band < m_amountOfColumns; band++) {
      //Get FFT data
      float fval = fft1024_1.read(logGen1024[band][0], logGen1024[band][1]);
      fval = fval * m_gain - m_shift;
      if (fval > logAmpSteps) fval = logAmpSteps;            //don't saturate the band

      //process bands bar value
      if (m_bandVal[band] > 0) m_bandVal[band] -= m_decay;   //decay current band
      if ((drawPeak || !m_noiseGate) && fval > m_bandVal[band]) m_bandVal[band] = fval; //set to current value if it's greater
      barValue = (int)m_bandVal[band];                       //reduce to a pixel location
      if (barValue > logAmpSteps - 1) barValue = logAmpSteps - 1; //apply limits
      if (barValue < 0) barValue = 0;
      barValue = logAmpOffset[barValue];

      //process peak bar value
      if (drawPeak || !m_noiseGate) fval = m_bandVal[band] + 0.1; //examine band data transposed slightly higher
      else fval += 0.1;
      if (fval > m_peakVal[band]) {                //if value is greater than stored data
        m_peakVal[band] = fval;                    //update stored data
        peakCount = m_peakCounter;                 //Start the peak display counter
      }
      barPeak = (int)m_peakVal[band];              //extract the pixel location of peak
      if (barPeak > logAmpSteps - 1) barPeak = logAmpSteps - 1; //apply limits
      if (barPeak < 0.0) barPeak = 0.0;
      barPeak = logAmpOffset[barPeak];
      maxPeak += barPeak;                          //sum up all the peaks

      m_bands[band] = barValue;
      m_peaks[band] = barPeak;
    }
      
    //Peak counter timeout
    if (peakCount > 0) {                                      //if the peak conter is active
      if (--peakCount == 0) {                                 //and decrementing it deactivates it
        for (int band = 0; band < m_amountOfColumns; band++) {  //clear the peak values
          m_peakVal[band] = 0;
        }
      }
    }
  maxBeatValue = 0;
  for (int i = m_amountOfColumns * 6 / 10;i<m_amountOfColumns;i++)
  {
    if (m_bands[i]>maxBeatValue) {
      maxBeatValue = m_bands[i]; 
    }
  }
  }
  //noInterrupts();
}

void initAudio()
{
  AudioMemory(12);
  calcBands();
  calcAmplitudes();
#ifdef PATCHED_AUDIO
  fft1024_1.calculateOnRequestOnly(true);
#endif
}

// --- End of FFT/AUDIO ----------------------------------------------------------------------------------

#include "Laser.h"
#include "Drawing.h"

// Create laser instance (with laser pointer connected to digital pin 5)
Laser laser(5);

void setup()
{  
  laser.init();
  initAudio();
}

void fftLoop()
{
  int rotate = 0;
  int count = 0;
  while (1)
  { 
    updateFFT();
    count++;
    laser.setScale(1.);
    laser.setOffset(0,0);

    if (count % 2000 > 1000) 
    { 
      // circle analyzer   
      m_decay = 0.15;
      int i = 0;
      float firstX;
      float firstY;
      i=20;
      for (int r = 0;r<=360;r+=8,i++)
      {    
        float d = ((float)m_bands[i]*3 + maxBeatValue * 0.15) /320.;
        float x = SIN((r + rotate) % 360) * d + 2048;
        float y = COS((r + rotate) % 360) *d + 2048;
        laser.sendto(x,y);
        if (r == 0)   {    
          laser.on();
          firstX = x;
          firstY = y;
        }
      }
      laser.sendto(firstX,firstY);
      rotate += 1;
      laser.off();
    } else 
    {
      // normal analyzer
      m_decay = 0.5;
      long step = 4096/m_amountOfColumns;
      long pos = 0;
      laser.sendto(0,0);
      laser.on(); 
      for (int i = 0;i<m_amountOfColumns;i++)
      {
        laser.sendto(pos, m_bands[i] * 30);  
        pos += step;
      }
      laser.off();
      if (count % 2000 > 500) 
      {
        laser.sendto(0,0);
        laser.on();
        pos = 0; 
        for (int i = 0;i<m_amountOfColumns;i++)
        {
          laser.sendto(pos, m_peaks[i] * 30);  
          pos += step;
        }
        laser.off();
      }
    }
  }
}

void laserShow()
{
  String str = "LASER";
  int w = Drawing::stringAdvance(str);
  int count = 360/4;
  int angle = 0;
  laser.setScale(0.5);
  for (int i = 0;i<count;i++) {
    Matrix3 world;
    laser.setEnable3D(true);
    world = Matrix3::rotateX(angle % 360);
    laser.setMatrix(world);
    laser.setZDist(4000);
    laser.setOffset(1024,1024 + 600);
    Drawing::drawString(str,-w/2,-500, 1);
    world = Matrix3::rotateY(angle % 360);
    laser.setMatrix(world);
    laser.setOffset(1024,1024 - 600);
    Drawing::drawString("SHOW",-w/2,-500, 1);
    angle += 8;
  }
  laser.setEnable3D(false);
}

void loop() {
  laserShow();
  fftLoop();
}

