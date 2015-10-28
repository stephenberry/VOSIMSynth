#include "VoiceManager.h"
namespace syn
{
  Instrument* VoiceManager::createVoice(int note, int vel)
  {
    int vind = findIdleVoice();        
    m_voiceStack.push_back(vind);
    m_voiceMap[note].push_back(vind);
    m_allVoices[vind]->noteOn(note, vel);
    m_numVoices++;
    return m_allVoices[vind];
  }

  void VoiceManager::makeIdle()
  {
    m_numVoices--;
    int vind = m_voiceMap.begin()->second.back();
    m_voiceMap.begin()->second.pop_back();
    m_voiceStack.remove(vind);
    m_onDyingVoice.Emit(m_allVoices[vind]);
  }

  void VoiceManager::makeIdle(int vind)
  {
    m_numVoices--;
    m_voiceMap[m_allVoices[vind]->getNote()].remove(vind);
    m_voiceStack.remove(vind);
    m_onDyingVoice.Emit(m_allVoices[vind]);
  }

  int VoiceManager::findIdleVoice() const
  {
    int i = 0;
    while(i<m_allVoices.size())
    {
      if(!m_allVoices[i]->isActive()){
        return i;
      }
      i++;
    }
    return -1;
  }

  Instrument* VoiceManager::noteOn(uint8_t noteNumber, uint8_t velocity)
  {
    Instrument* v;
    if (m_numVoices == m_maxVoices)
    {
      int vind = getOldestVoiceInd();
      v = m_allVoices[vind];
      m_voiceMap[v->getNote()].remove(vind);
      m_voiceStack.remove(vind);
      v->noteOn(noteNumber, velocity);
      m_voiceStack.push_back(vind);
      m_voiceMap[noteNumber].push_back(vind);
    }
    else if(m_numVoices>=0)
    {
      v = createVoice(noteNumber, velocity);
    }    
    return v;
  }

  void VoiceManager::noteOff(uint8_t noteNumber, uint8_t velocity)
  {
    if (m_voiceMap.find(noteNumber) != m_voiceMap.end() && !m_voiceMap[noteNumber].empty())
    {
      for (VoiceList::iterator v = m_voiceMap[noteNumber].begin(); v != m_voiceMap[noteNumber].end(); v++)
      {
        m_allVoices[*v]->noteOff(noteNumber, velocity);
      }
    }
  }

  void VoiceManager::setFs(double fs)
  {
    m_instrument->setFs(fs);
    for (vector<Instrument*>::iterator v = m_allVoices.begin(); v != m_allVoices.end(); v++)
    {
      (*v)->setFs(fs);
    }
  }

  void VoiceManager::setMaxVoices(int max, Instrument* v)
  {
    if (max < 1)
      max = 1;
    m_maxVoices = max;
    while (m_numVoices > 1)
    {
      makeIdle();
    }
    while (m_allVoices.size()>max)
    {
      delete m_allVoices.back();
      m_allVoices.pop_back();
    }

    m_instrument = v;

    for (int i=0; i < m_allVoices.size(); i++)
    {
      delete m_allVoices[i];
      m_allVoices[i] = (Instrument*)m_instrument->clone();
    }

    while (m_allVoices.size() < max)
    {
      m_allVoices.push_back((Instrument*)m_instrument->clone());
    }
  }

  void VoiceManager::tick(double* buf, size_t nsamples)
  {
    double finalOutput = 0;
    mSampleCount++;
    for (VoiceList::iterator v = m_voiceStack.begin(); v != m_voiceStack.end();)
    {
      if (m_allVoices[*v]->isActive())
      {
        m_allVoices[*v]->tick(nsamples);
        const vector<double>& voiceOutput = m_allVoices[*v]->getLastOutputBuffer();
        for (int i = 0; i < nsamples; i++)
        {
          buf[i] += voiceOutput[i];
        }
        v++;
      }
      else
      {        
        int vind = *v;
        v++;
        makeIdle(vind);
      }
    }
  }

  void VoiceManager::modifyParameter(int uid, int pid, double val, MOD_ACTION action)
  {
    m_instrument->modifyParameter(uid, pid, val, action);
    for (int i=0;i<m_allVoices.size();i++)
    {
      m_allVoices[i]->modifyParameter(uid, pid, val, action);
    }
  }

  void VoiceManager::sendMIDICC(IMidiMsg& msg)
  {
    m_instrument->sendMIDICC(msg);
    for (int i = 0; i < m_allVoices.size(); i++)
    {
      m_allVoices[i]->sendMIDICC(msg);
    }
  }

  int VoiceManager::getLowestVoiceInd() const
  {
    if (m_numVoices)
    {
      VoiceMap::const_iterator it;
      for (it = m_voiceMap.begin(); it != m_voiceMap.end(); it++)
      {
        if (!it->second.empty() && m_allVoices[it->second.back()]->isActive())
        {
          return it->second.back();
        }
      }
    }
    return -1;
  }


  int VoiceManager::getNewestVoiceInd() const
  {
    if (m_numVoices){
      for (VoiceList::const_reverse_iterator it = m_voiceStack.rbegin(); it!=m_voiceStack.rend();it++)
      {
        if(m_allVoices[*it]->isActive())
          return *it;
      }
    }
    return -1;
  }

  int VoiceManager::getOldestVoiceInd() const
  {
    if (m_numVoices)
    {
      for (VoiceList::const_iterator it = m_voiceStack.begin(); it != m_voiceStack.end(); it++)
      {
        if (m_allVoices[*it]->isActive())
          return *it;
      }
    }
    return -1;
  }

  int VoiceManager::getHighestVoiceInd() const
  {
    if (m_numVoices)
    {
      VoiceMap::const_reverse_iterator it;
      for (it = m_voiceMap.rbegin(); it != m_voiceMap.rend(); it++)
      {
        if (!it->second.empty() && m_allVoices[it->second.back()]->isActive())
        {
          return it->second.back();
        }
      }
    }
    return -1;
  }
}