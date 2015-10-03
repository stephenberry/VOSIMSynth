#ifndef __VOSIMSYNTH__
#define __VOSIMSYNTH__

#define NUM_VOICES 128

#include <cstdint>
#include "IPlug_include_in_plug_hdr.h"
#include "Oscillator.h"
#include "MIDIReceiver.h"
#include "VoiceManager.h"
#include "Filter.h"
#include "UI.h"

class VOSIMSynth : public IPlug {
public:
  VOSIMSynth(IPlugInstanceInfo instanceInfo);
  void sendToScope(double);
  void OnNoteOn(uint8_t pitch, uint8_t vel);
  void OnNoteOff(uint8_t pitch, uint8_t vel);
  ~VOSIMSynth() {
    delete mOscilloscope;
    delete LP4;
  };

  void Reset();
  void OnParamChange(int paramIdx);
  void ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames);
  void ProcessMidiMsg(IMidiMsg* pMsg);
private:
  void setGain(double gain) { mOutGain = gain; };
  MIDIReceiver mMIDIReceiver;
  VoiceManager mVoiceManager;
  Oscilloscope *mOscilloscope;
  Filter *LP4;
  double mOutGain = 1.0;
  double mLastOutput = 0.0;
};

#endif
