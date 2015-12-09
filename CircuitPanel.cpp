#include "CircuitPanel.h"
#include "UI.h"
#include "VOSIMSynth.h"
#include "mutex.h"

namespace syn
{
  UnitControl::UnitControl(IPlugBase* pPlug, VoiceManager* vm, Unit* unit, int x, int y, int size) :
    m_size(size),
    m_x(x),
    m_y(y),
    m_unit(unit),
    m_nParams(unit->getParameterNames().size()),
    m_is_sink(false),
    IControl(pPlug, { x,y,x + size,y + size })
  {
    for (int i = 0; i < m_nParams; i++)
    {
      m_portLabels.push_back(ITextSlider(pPlug, vm, IRECT{ 0,0,0,0 }, unit->getParent().getUnitId(unit), i));
    }
    m_ports.resize(m_nParams);
    resize(size);
  }

  void UnitControl::OnMouseDblClick(int x, int y, IMouseMod* pMod)
  {
    int selectedParam = getSelectedParam(x,y);
    if(selectedParam>=0)
    {
      m_portLabels[selectedParam].OnMouseDblClick(x,y,pMod);
    }
  }

  UnitControl::~UnitControl()
  {}

  void UnitControl::move(int newx, int newy)
  {
    if (newx < 0 || newy < 0) return;
    m_x = newx;
    m_y = newy;
    mRECT.L = newx;
    mRECT.T = newy;
    mRECT.R = newx + m_size;
    mRECT.B = newy + m_size;
    SetTargetArea(mRECT);
    resize(m_size);
  }

  int UnitControl::getMinSize() const
  {
    return 10*m_nParams+30;
  }

  void UnitControl::resize(int newsize)
  {
    const vector<string>& paramNames = m_unit->getParameterNames();
    if (newsize <= getMinSize()) newsize = getMinSize();
    m_size = newsize;
    mRECT.R = m_x + m_size;
    mRECT.B = m_y + m_size;

    int portY = m_y + 20;
    int rowsize = (newsize - 20) / (double)m_nParams;
    for (int i = 0; i < paramNames.size(); i++)
    {
      IRECT port_label_irect{ m_x + 30 ,portY, m_x + newsize,portY + 10 };
      m_portLabels[i].setRect(port_label_irect);
      m_ports[i].add_rect = IRECT{ m_x, portY, m_x + 10, portY + 10 };
      m_ports[i].scale_rect = IRECT{ m_x + 12, portY, m_x + 22, portY + 10 };
      portY += rowsize;
    }
    SetTargetArea(mRECT);
  }

  bool UnitControl::Draw(IGraphics* pGraphics)
  {
    Instrument* instr = static_cast<Instrument*>(&m_unit->getParent());
    // Local text palette
    IText textfmt{ 12, &COLOR_BLACK,"Helvetica",IText::kStyleNormal,IText::kAlignNear,0,IText::kQualityClearType };
    IText textfmtwhite{ 12, &COLOR_WHITE,"Helvetica",IText::kStyleNormal,IText::kAlignNear,0,IText::kQualityClearType };
    IText centertextfmt{ 12, &COLOR_BLACK,"Helvetica",IText::kStyleNormal,IText::kAlignCenter,0,IText::kQualityClearType };
    // Local color palette
    IColor bg_color = m_is_sink ? globalPalette[1] : globalPalette[2];
    IColor addport_color{ 150,0,255,0 };
    IColor mulport_color{ 150,255,0,0 };

    if (instr->isPrimarySource(instr->getUnitId(m_unit)))
    {
      IRECT paddedoutline = mRECT.GetPadded(5);
      pGraphics->DrawRect(&COLOR_WHITE, &paddedoutline);
    }

    pGraphics->FillIRect(&bg_color, &mRECT);

    VOSIMSynth* vs = static_cast<VOSIMSynth*>(mPlug);
    // If this unit is the oscilloscope input
    if (vs->m_Oscilloscope->getInputId() == m_unit->getParent().getUnitId(m_unit))
    {
      IRECT input_badge_irect { mRECT.R - 10,mRECT.B - 10,mRECT.R,mRECT.B };
      pGraphics->DrawIText(&textfmt, "I", &input_badge_irect);
    }
    if (vs->m_Oscilloscope->getTriggerId() == m_unit->getParent().getUnitId(m_unit))
    {
      IRECT trigger_badge_irect{ mRECT.R - 20,mRECT.B - 10,mRECT.R - 10,mRECT.B };
      pGraphics->DrawIText(&textfmt, "T", &trigger_badge_irect);
    }

    vector<string> paramNames = m_unit->getParameterNames();
    char strbuf[256];
    sprintf(strbuf, "%s", m_unit->getName().c_str());
    IRECT titleTextRect{ m_x,m_y,m_x + m_size,m_y + 10 };
    pGraphics->DrawIText(&centertextfmt, strbuf, &titleTextRect, true);
    pGraphics->DrawIText(&centertextfmt, strbuf, &titleTextRect);
    if(titleTextRect.W()+10>m_size)
    {
      resize(titleTextRect.W()+10);
    }
    for (int i = 0; i < paramNames.size(); i++)
    {
      pGraphics->FillCircle(&addport_color, m_ports[i].add_rect.MW(), m_ports[i].add_rect.MH(), m_ports[i].add_rect.W() / 2, 0, true);
      pGraphics->DrawIText(&centertextfmt, "+", &m_ports[i].add_rect);
      pGraphics->FillCircle(&mulport_color, m_ports[i].scale_rect.MW(), m_ports[i].scale_rect.MH(), m_ports[i].scale_rect.W() / 2, 0, true);
      pGraphics->DrawIText(&centertextfmt, "x", &m_ports[i].scale_rect);
      sprintf(strbuf, "%s", paramNames[i].c_str());
      if (!m_unit->getParam(i).isHidden())
      {
        m_portLabels[i].Draw(pGraphics);
        pGraphics->DrawIText(&textfmtwhite, strbuf, &m_portLabels[i].getRect());
      }
      else
      {
        pGraphics->DrawIText(&textfmt, strbuf, &m_portLabels[i].getRect());
      }
    }
    return true;
  }

  NDPoint<2, int> UnitControl::getPos() const
  {
    return NDPoint<2, int>(m_x, m_y);
  }

  NDPoint<2, int> UnitControl::getPortPos(SelectedPort& port)
  {
    int portid = port.paramid;
    if (port.modaction == ADD)
    {
      return NDPoint<2, int>(m_ports[portid].add_rect.L + m_ports[portid].add_rect.W() / 2, m_ports[portid].add_rect.T + m_ports[portid].add_rect.H() / 2);
    }
    else if (port.modaction == SCALE)
    {
      return NDPoint<2, int>(m_ports[portid].scale_rect.L + m_ports[portid].scale_rect.W() / 2, m_ports[portid].scale_rect.T + m_ports[portid].scale_rect.H() / 2);
    }
    else
    {
      return NDPoint<2, int>(0, 0);
    }
  }

  NDPoint<2, int> UnitControl::getOutputPos() const
  {
    return NDPoint<2, int>(m_x + m_size, m_y + m_size / 2);
  }

  Unit* UnitControl::getUnit() const
  {
    return m_unit;
  }

  SelectedPort UnitControl::getSelectedPort(int x, int y)
  {
    SelectedPort selectedPort = { -1,SET };
    for (int i = 0; i < m_ports.size(); i++)
    {
      if (m_ports[i].add_rect.Contains(x, y))
      {
        selectedPort.paramid = i;
        selectedPort.modaction = ADD;
      }
      else if (m_ports[i].scale_rect.Contains(x, y))
      {
        selectedPort.paramid = i;
        selectedPort.modaction = SCALE;
      }
    }
    return selectedPort;
  }

  int UnitControl::getSelectedParam(int x, int y)
  {
    int selectedParam = -1;
    for (int i = 0; i < m_portLabels.size(); i++)
    {
      if (m_unit->getParam(i).isHidden())
      {
        continue;
      }
      if (m_portLabels[i].getRect().Contains(x, y))
      {
        selectedParam = i;
      }
    }
    return selectedParam;
  }

  void CircuitPanel::updateInstrument() const
  {
    IPlugBase::IMutexLock lock(mPlug);
    m_vm->setMaxVoices(m_vm->getMaxVoices(), m_vm->getProtoInstrument());
  }

  void CircuitPanel::deleteUnit(int unitctrlid)
  {
    WDL_MutexLock guilock(&mPlug->GetGUI()->mMutex);
    // Delete unit from instrument
    Unit* unit = m_unitControls[unitctrlid]->getUnit();
    int unitid = unit->getParent().getUnitId(unit);
    VOSIMSynth* vs = static_cast<VOSIMSynth*>(mPlug);
    Instrument* instr = m_vm->getProtoInstrument();
    instr->removeUnit(instr->getUnitId(unit));
    // Delete unit controller
    UnitControl* unitctrl = m_unitControls[unitctrlid];
    m_unitControls.erase(unitctrlid);
    delete unitctrl;
    updateInstrument();
  }

  void CircuitPanel::setSink(int unitctrlid)
  {
    Instrument* instr = m_vm->getProtoInstrument();
    Unit* unit = m_unitControls[unitctrlid]->getUnit();
    instr->setSinkId(instr->getUnitId(unit));
    for(pair<int,UnitControl*> ctrlpair : m_unitControls)
    {
      if(ctrlpair.second->m_is_sink)
      {
        ctrlpair.second->m_is_sink = false;
      }
    }
    m_unitControls[unitctrlid]->m_is_sink = true;
    updateInstrument();
  }

  void CircuitPanel::OnMouseDown(int x, int y, IMouseMod* pMod)
  {
    if (pMod->L) m_isMouseDown = 1;
    else if (pMod->R) m_isMouseDown = 2;
    m_lastMousePos = NDPoint<2, int>(x, y);
    m_lastClickPos = m_lastMousePos;
    m_lastSelectedUnit = getSelectedUnit(x, y);
    if (m_lastSelectedUnit >= 0)
    {
      UnitControl* unitCtrl = m_unitControls[m_lastSelectedUnit];
      m_lastSelectedPort = unitCtrl->getSelectedPort(x, y);
      m_lastSelectedParam = unitCtrl->getSelectedParam(x, y);

      if (pMod->L && m_lastSelectedParam >= 0)
      {
        m_currAction = MOD_PARAM;
      }
      else if (pMod->L && m_lastSelectedPort.paramid >= 0)
      {
        m_currAction = CONNECT;
      }
      else if (pMod->C && pMod->L)
      {
        m_currAction = RESIZE;
      }
      else if (pMod->L)
      {
        m_currAction = MOVE;
      }
    }
  }

  void CircuitPanel::createSourceUnit(int factoryid, int x, int y) {
    WDL_MutexLock guilock(&mPlug->GetGUI()->mMutex);
    Instrument* instr = m_vm->getProtoInstrument();
    SourceUnit* srcunit = m_unitFactory->createSourceUnit(factoryid);
    int uid = instr->addSource(srcunit);
    m_unitControls[uid] = new UnitControl(mPlug, m_vm, srcunit, x, y);
    updateInstrument();
  }

  void CircuitPanel::createUnit(int factoryid, int x, int y) {
    WDL_MutexLock guilock(&mPlug->GetGUI()->mMutex);
    Instrument* instr = m_vm->getProtoInstrument();
    Unit* unit = m_unitFactory->createUnit(factoryid);
    int uid = instr->addUnit(unit);
    m_unitControls[uid] = new UnitControl(mPlug, m_vm, unit, x, y);
    updateInstrument();
  }

  void CircuitPanel::OnMouseUp(int x, int y, IMouseMod* pMod)
  {
    Instrument* instr = m_vm->getProtoInstrument();
    int currSelectedUnit = getSelectedUnit(x, y);
    if (m_isMouseDown == 2 && currSelectedUnit == -1)
    { // Right clicking on open space
      IPopupMenu* selectedMenu = mPlug->GetGUI()->CreateIPopupMenu(&m_main_menu, x, y);
      if (selectedMenu == &m_sourceunit_menu)
      { // Create a source unit
        int itemChosen = selectedMenu->GetChosenItemIdx();
        createSourceUnit(itemChosen, x, y);
      }
      else if (selectedMenu == &m_unit_menu)
      { // Create a non-source unit
        int itemChosen = selectedMenu->GetChosenItemIdx();
        createUnit(itemChosen, x, y);
      }
    }
    else if (m_isMouseDown == 2 && currSelectedUnit >= 0)
    { // Right clicking on a unit
      IPopupMenu unitmenu;
      unitmenu.AddItem("Set sink");
      unitmenu.AddItem("Delete");
      unitmenu.AddItem("Set oscilloscope source");
      if (instr->isSourceUnit(currSelectedUnit))
      {
        unitmenu.AddSeparator();
        unitmenu.AddItem("Set oscilloscope trigger");
        unitmenu.AddItem("Set primary source");
      }
      Unit* unit = m_unitControls[currSelectedUnit]->getUnit();
      IPopupMenu* selectedmenu = mPlug->GetGUI()->CreateIPopupMenu(&unitmenu, x, y);
      if (selectedmenu == &unitmenu)
      {
        int selectedItem = selectedmenu->GetChosenItemIdx();
        if (selectedItem == 0)
        { // Set sink
          setSink(currSelectedUnit);
          updateInstrument();
        }
        else if (selectedItem == 1)
        { // Delete unit
          deleteUnit(currSelectedUnit);
          updateInstrument();
        }
        else if (selectedItem == 2)
        { // Set oscilloscope source
          VOSIMSynth* vs = static_cast<VOSIMSynth*>(mPlug);
          vs->m_Oscilloscope->connectInput(instr->getUnitId(unit));
        }
        else if (selectedItem == 4)
        { // Set oscilloscope trigger
          VOSIMSynth* vs = static_cast<VOSIMSynth*>(mPlug);
          vs->m_Oscilloscope->connectTrigger(instr->getUnitId(unit));
        }
        else if (selectedItem == 5)
        { // Set primary source
          instr->resetPrimarySource(instr->getUnitId(unit));
          updateInstrument();
        }
      }
    }
    else if (m_currAction == CONNECT && currSelectedUnit >= 0)
    {
      m_vm->getProtoInstrument()->addConnection({ currSelectedUnit, m_lastSelectedUnit, m_lastSelectedPort.paramid, m_lastSelectedPort.modaction });
      updateInstrument();
    }
    m_currAction = NONE;
    m_isMouseDown = 0;
  }

  void CircuitPanel::OnMouseDrag(int x, int y, int dX, int dY, IMouseMod* pMod)
  {
    NDPoint<2, int> currMousePos = NDPoint<2, int>(x, y);
    if (m_lastSelectedUnit >= 0)
    {
      UnitControl* unitCtrl = m_unitControls[m_lastSelectedUnit];
      NDPoint<2, int> unitPos = unitCtrl->getPos();
      if (m_currAction == MOD_PARAM)
      {
        unitCtrl->m_portLabels[m_lastSelectedParam].OnMouseDrag(x,y,dX,dY,pMod);
      }
      else if (m_currAction == RESIZE)
      {
        unitCtrl->resize(dX+unitCtrl->m_size);
      }
      else if (m_currAction == MOVE)
      {
        NDPoint<2, int> newUnitPos = currMousePos - m_lastMousePos + unitPos;
        unitCtrl->move(newUnitPos[0], newUnitPos[1]);
      }
    }
    m_lastMousePos = currMousePos;
  }

  void CircuitPanel::OnMouseDblClick(int x, int y, IMouseMod* pMod)
  {
    WDL_MutexLock lock(&mPlug->GetGUI()->mMutex);
    int currSelectedUnit = getSelectedUnit(x, y);
    if(currSelectedUnit>=0)
    {
      UnitControl* unitctrl = m_unitControls[currSelectedUnit];
      unitctrl->OnMouseDblClick(x,y,pMod);
    }
  }

  bool CircuitPanel::Draw(IGraphics* pGraphics)
  {
    WDL_MutexLock lock(&pGraphics->mMutex);
    // Local palette
    IColor bg_color = globalPalette[0];
    pGraphics->FillIRect(&bg_color, &mRECT);
    for (pair<int, UnitControl*> unitpair : m_unitControls)
    {
      unitpair.second->Draw(pGraphics);
    }
    Instrument* instr = m_vm->getProtoInstrument();
    for (pair<int, UnitControl*> unitpair : m_unitControls)
    {
      const vector<ConnectionMetadata>& connections = instr->getConnectionsTo(unitpair.first);
      for (int j = 0; j < connections.size(); j++)
      {
        NDPoint<2, int> pt1 = m_unitControls[connections[j].srcid]->getOutputPos();
        SelectedPort selectedPort{ connections[j].portid,connections[j].action };
        NDPoint<2, int> pt2 = m_unitControls[connections[j].targetid]->getPortPos(selectedPort);
        pGraphics->DrawLine(&COLOR_WHITE, pt1[0], pt1[1], pt2[0], pt2[1], nullptr, true);
      }
    }
    if (m_currAction == CONNECT)
    {
      pGraphics->DrawLine(&COLOR_WHITE, m_lastClickPos[0], m_lastClickPos[1], m_lastMousePos[0], m_lastMousePos[1], nullptr, true);
    }
    return true;
  }

  int CircuitPanel::getSelectedUnit(int x, int y)
  {
    int selectedUnit = -1;
    for (pair<int, UnitControl*> unitpair : m_unitControls)
    {
      if (unitpair.second->IsHit(x, y))
      {
        selectedUnit = unitpair.first;
      }
    }
    return selectedUnit;
  }

  ByteChunk CircuitPanel::serialize() const
  {
    ByteChunk serialized;

    unsigned int numunits = m_unitControls.size();
    serialized.PutBytes(&numunits, sizeof(unsigned int));
    for (pair<int, UnitControl*> ctrlpair : m_unitControls)
    {
      ByteChunk unitctrl_chunk = serializeUnitControl(ctrlpair.first);
      serialized.PutChunk(&unitctrl_chunk);
    }

    Instrument* instr = m_vm->getProtoInstrument();
    for (pair<int, UnitControl*> ctrlpair : m_unitControls)
    {
      int unitid = instr->getUnitId(ctrlpair.second->m_unit);
      const vector<ConnectionMetadata> connections = instr->getConnectionsTo(unitid);

      unsigned int numconnections = connections.size();
      serialized.PutBytes(&numconnections, sizeof(unsigned int));
      for (int j = 0; j < connections.size(); j++)
      {
        serialized.Put<ConnectionMetadata>(&connections[j]);
      }
    }
    return serialized;
  }

  int CircuitPanel::unserialize(ByteChunk* serialized, int startPos)
  {
    int chunkpos = startPos;
    Instrument* instr = m_vm->getProtoInstrument();
    vector<int> unitctrlids;
    for (pair<int, UnitControl*> ctrlpair : m_unitControls) {
      unitctrlids.push_back(ctrlpair.first);
    }
    for (int i = 0; i < unitctrlids.size(); i++) {
      deleteUnit(unitctrlids[i]);
    }

    // Unserialize units
    unsigned int numunits;
    chunkpos = serialized->Get<unsigned int>(&numunits, chunkpos);
    for (int i = 0; i < numunits; i++) {
      chunkpos = unserializeUnitControl(serialized, chunkpos);
    }

    // Unserialize connections
    for (int i = 0; i < numunits; i++) {
      unsigned int numConns;
      chunkpos = serialized->Get<unsigned int>(&numConns, chunkpos);
      for (int j = 0; j < numConns; j++) {
        ConnectionMetadata conn;
        chunkpos = serialized->Get<ConnectionMetadata>(&conn, chunkpos);
        instr->addConnection(conn);
      }
    }
    updateInstrument();
    return chunkpos;
  }
  ByteChunk CircuitPanel::serializeUnitControl(int ctrlidx) const
  {
    ByteChunk serialized;
    UnitControl* uctrl = m_unitControls.at(ctrlidx);
    Instrument* instr = m_vm->getProtoInstrument();
    unsigned int unitClassId = uctrl->m_unit->getClassIdentifier();
    int unitid = instr->getUnitId(uctrl->m_unit);
    bool isSource = instr->isSourceUnit(unitid);
    bool isPrimarySource = isSource ? instr->isPrimarySource(unitid) : false;

    // Write unique class identifier
    serialized.Put<unsigned int>(&unitClassId);
    // Write the ID given to the unit by its parent circuit so it can be placed in the same location
    serialized.Put<int>(&unitid);
    // Write whether or not this unit is a SourceUnit
    serialized.Put<bool>(&isSource);
    // Write whether or not this unit is a primary source within its circuit
    serialized.Put<bool>(&isPrimarySource);
    // Write whether or not this unit is the audio sink
    serialized.Put<bool>(&uctrl->m_is_sink);

    // Write the unit's parameter configuration
    serialized.Put<unsigned int>(&uctrl->m_nParams);
    for (int i = 0; i < uctrl->m_nParams; i++) {
      double paramval = uctrl->m_unit->getParam(i);
      // Write the parameter's name
      serialized.PutStr(uctrl->m_unit->getParam(i).getName().c_str());
      // Write the parameter's current value
      serialized.Put<double>(&paramval);
    }

    // Write info related to this unit's GUI window
    serialized.Put<int>(&uctrl->m_size);
    serialized.Put<int>(&uctrl->m_x);
    serialized.Put<int>(&uctrl->m_y);
    return serialized;
  }

  int CircuitPanel::unserializeUnitControl(ByteChunk* chunk, int startPos)
  {
    WDL_MutexLock guilock(&mPlug->GetGUI()->mMutex);
    int chunkpos = startPos;
    unsigned int unitClassId;
    int unitid;
    bool isSource, isPrimarySource, isSink;
    unsigned int numparams;
    int x, y;
    int size;
    Unit* unit;
    chunkpos = chunk->Get<unsigned int>(&unitClassId, chunkpos);
    chunkpos = chunk->Get<int>(&unitid, chunkpos);
    chunkpos = chunk->Get<bool>(&isSource, chunkpos);
    chunkpos = chunk->Get<bool>(&isPrimarySource, chunkpos);
    chunkpos = chunk->Get<bool>(&isSink, chunkpos);
    unit = isSource ? m_unitFactory->createSourceUnit(unitClassId) : m_unitFactory->createUnit(unitClassId);

    chunkpos = chunk->Get<unsigned int>(&numparams, chunkpos);
    for (int i = 0; i < numparams; i++) {
      double paramval;
      WDL_String paramname;
      chunkpos = chunk->GetStr(&paramname, chunkpos);
      chunkpos = chunk->Get<double>(&paramval, chunkpos);
      int paramid = unit->getParamId(paramname.Get());
      if(paramid==-1)
      {
        continue;
      }
      unit->modifyParameter(paramid, paramval, SET);
    }
    chunkpos = chunk->Get<int>(&size, chunkpos);
    chunkpos = chunk->Get<int>(&x, chunkpos);
    chunkpos = chunk->Get<int>(&y, chunkpos);

    Instrument* instr = m_vm->getProtoInstrument();
    int uid = isSource ? instr->addSource(dynamic_cast<SourceUnit*>(unit), unitid) : instr->addUnit(unit, unitid);
    m_unitControls[uid] = new UnitControl(mPlug, m_vm, unit, x, y, size);
    if (isSink) setSink(uid);
    if (isPrimarySource) instr->resetPrimarySource(uid);
    updateInstrument();
    return chunkpos;
  }
}