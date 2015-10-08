
#ifndef __VOICEMANAGER__
#define __VOICEMANAGER__

#define MOD_FS_RAT 0
#include "Oscillator.h"
#include "Filter.h"
#include <cmath>
#include <cstdint>
#include <string>
#include <list>
#include <vector>
#include <map>
#include <unordered_map>
#include <functional>

class Voice : public DSPComponent<double> {
public:
  uint8_t mNote;
  double mVelocity;
  VOSIM mOsc[3];
  Envelope mVFEnv[3];
  Envelope mAmpEnv;
  Oscillator mLFOPitch;
  Filter *m_LP4;
  void trigger(uint8_t noteNumber, uint8_t velocity);
  void release();
  void setAudioFs(double fs);
  void setModFs(double fs);
  void updateParams();
  bool isActive();
  bool isSynced();
  double process(const double input = 0);
  int getSamplesPerPeriod() const;
  Voice() :
    DSPComponent() {
    m_LP4 = new Filter(AA_FILTER_X, AA_FILTER_Y, AA_FILTER_SIZE + 1, AA_FILTER_SIZE);
    mNote = 0;
    mOsc[0].m_pGain.set(0);
    mOsc[1].m_pGain.set(0);
    mOsc[2].m_pGain.set(0);    
    mVFEnv[0].connectOutputTo(&mOsc[0].mpPulsePitch, &Modifiable<double>::mod);
    mVFEnv[1].connectOutputTo(&mOsc[1].mpPulsePitch, &Modifiable<double>::mod);
    mVFEnv[2].connectOutputTo(&mOsc[2].mpPulsePitch, &Modifiable<double>::mod);
    mAmpEnv.connectOutputTo(&m_LP4->m_pGain, &Modifiable<double>::scale);
    mLFOPitch.connectOutputTo(&mOsc[0].m_pPitch, &Modifiable<double>::mod);
    mLFOPitch.connectOutputTo(&mOsc[1].m_pPitch, &Modifiable<double>::mod);
    mLFOPitch.connectOutputTo(&mOsc[2].m_pPitch, &Modifiable<double>::mod);
    // connect sum of oscillators to LP4 filter
    // connect LP4 filter to voice output
    updateParams();
  };
  Voice(const Voice& v) :
    Voice() {
    mNote = v.mNote;
    m_LP4 = new Filter(AA_FILTER_X, AA_FILTER_Y, AA_FILTER_SIZE + 1, AA_FILTER_SIZE);
  }
  ~Voice() {
    delete m_LP4; 
  }
};


class VoiceManager {
private:
  uint32_t mSampleCount;
  uint8_t m_numVoices;
public:
  map<int, list<Voice*> > m_voiceMap;
  list<Voice*> m_activeVoiceStack;
  list<Voice*> m_idleVoiceStack;
  vector<Voice> m_voices;  
  Voice& TriggerNote(uint8_t noteNumber, uint8_t velocity);
  void ReleaseNote(uint8_t noteNumber, uint8_t velocity);
  Voice& getLowestVoice() const;
  Voice& getNewestVoice() const;
  Voice& getOldestVoice() const;
  Voice& getHighestVoice() const;
  void setFs(double fs);
  void setNumVoices(int numVoices);
  int getNumVoices() const { return m_numVoices; };
  int getNumActiveVoices() const { return m_activeVoiceStack.size(); };
  double process(double input = 0);

  VoiceManager() :
    m_numVoices(1),
    mSampleCount(0) {
    setNumVoices(m_numVoices);
  };
  ~VoiceManager() {}
};

#endif
