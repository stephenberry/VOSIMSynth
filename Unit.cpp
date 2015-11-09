#include "Unit.h"
#include <array>

using namespace std;

namespace syn
{
  void Unit::modifyParameter(int portid, double val, MOD_ACTION action)
  {
    m_params[portid]->mod(val, action);
  }

  vector<string> Unit::getParameterNames() const
  {
    vector<string> pnames;
    for (int i = 0; i < m_params.size(); i++)
    {
      pnames.push_back(m_params[i]->getName());
    }
    return pnames;
  }

  Unit* Unit::clone() const
  {
    Unit* u = cloneImpl();
    u->m_name = m_name;
    u->m_Fs = m_Fs;
    u->m_output = m_output;
    u->m_parammap = m_parammap;      
    return u;
  }

  UnitParameter& Unit::addParam(string name, int id, PARAM_TYPE ptype,double min, double max, bool isHidden)
  {
    m_parammap[name] = id;
    if (m_params.size() <= id)
      m_params.resize(id + 1);
    m_params[id] = new UnitParameter(this,name, id, ptype, min, max, isHidden);
    return *m_params[id];
  }

  UnitParameter& Unit::addParam(string name, PARAM_TYPE ptype, double min, double max, bool isHidden)
  {
    return addParam(name, m_params.size(), ptype, min, max, isHidden);
  }

  UnitParameter& Unit::addEnumParam(string name, const vector<string> choice_names)
  {
    UnitParameter& param = addParam(name,ENUM_TYPE,0, choice_names.size(),false);
    for(int i=0;i<choice_names.size();i++){
      param.addValueName(i, choice_names[i]);
    }
    return param;
  }

  Unit::Unit(string name) :
    m_Fs(44100.0),
    m_output(0.0),
    m_name(name)
  {}

  Unit::~Unit()
  {
    // delete allocated parameters
    for (int i = 0; i < m_params.size(); i++)
    {
      delete m_params[i];
    }
  }

  void Unit::tick()
  {
    beginProcessing();
    process();
    finishProcessing();
    for (int i = 0; i < m_params.size(); i++)
    {
      m_params[i]->reset();
    }    
  }

  int Unit::getParamId(string name)
  {
    return m_parammap.at(name);
  }

}