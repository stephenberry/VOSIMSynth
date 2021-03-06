#include "vosimlib/units/MathUnits.h"

syn::MovingAverage::MovingAverage() :
    m_windowSize(1),
    m_lastOutput(0.0)
{
    m_delay.resizeBuffer(m_windowSize);
}

void syn::MovingAverage::setWindowSize(int a_newWindowSize)
{
    m_windowSize = a_newWindowSize;
    m_delay.resizeBuffer(m_windowSize);
    m_delay.reset();
    m_lastOutput = 0.0;
}

double syn::MovingAverage::getWindowSize() const
{
    return m_windowSize;
}

double syn::MovingAverage::process(double a_input)
{
    double output = (1.0 / m_windowSize) * (a_input - m_delay.process(a_input)) + m_lastOutput;
    m_lastOutput = output;
    return output;
}

double syn::MovingAverage::getPastInputSample(int a_offset)
{
    return m_delay.readTap(a_offset);
}

syn::DCRemoverUnit::DCRemoverUnit(const string& a_name) :
    Unit(a_name),
    m_pAlpha(addParameter_(UnitParameter("hp", 0.0, 1.0, 0.995))),
    m_lastOutput(0.0),
    m_lastInput(0.0)
{
    addInput_("in");
    addOutput_("out");
}

syn::DCRemoverUnit::DCRemoverUnit(const DCRemoverUnit& a_rhs) :
    DCRemoverUnit(a_rhs.name()) {}

void syn::DCRemoverUnit::process_()
{
    BEGIN_PROC_FUNC
        double input = READ_INPUT(0);
        double alpha = param(m_pAlpha).getDouble();
        double gain = 0.5 * (1 + alpha);
        // dc removal
        input = input * gain;
        double output = input - m_lastInput + alpha * m_lastOutput;
        m_lastInput = input;
        m_lastOutput = output;
        WRITE_OUTPUT(0, output);
    END_PROC_FUNC
}

void syn::DCRemoverUnit::reset()
{
    m_lastInput = 0.0;
    m_lastOutput = 0.0;
}

void syn::DCRemoverUnit::onNoteOn_()
{
    reset();
}

syn::RectifierUnit::RectifierUnit(const string& a_name) :
    Unit(a_name),
    m_pRectType(addParameter_(UnitParameter{"type",{"full","half"}}))
{
    addInput_("in");
    addOutput_("out");
}

syn::RectifierUnit::RectifierUnit(const RectifierUnit& a_rhs) :
    RectifierUnit(a_rhs.name()) { }

void syn::RectifierUnit::process_()
{
    BEGIN_PROC_FUNC
        double input = READ_INPUT(0);
        double output;
        switch (param(m_pRectType).getInt())
        {
        case 1: // half
            output = input > 0 ? input : 0;
            break;
        case 0: // full
        default:
            output = abs(input);
            break;
        }
        WRITE_OUTPUT(0, output);
    END_PROC_FUNC
}

syn::SummerUnit::SummerUnit(const string& a_name) :
    Unit(a_name),
    m_pBias(addParameter_(UnitParameter("bias", -1E6, 1E6, 0.0, UnitParameter::None, 2).setControlType(UnitParameter::Unbounded)))
{
    for (int i = 0; i < MAX_OUTPUTS; i++) {
        addInput_(std::to_string(i), 0.0);
    }
    addOutput_("out");
}

syn::SummerUnit::SummerUnit(const SummerUnit& a_rhs) :
    SummerUnit(a_rhs.name()) { }

void syn::SummerUnit::process_()
{
    BEGIN_PROC_FUNC
        double output = param(m_pBias).getDouble();
        for (int i = 0; i < numInputs(); i++) {
            int id = inputs().ids()[i];
            if (isInputConnected(id))
                output += READ_INPUT(id);
        }
        WRITE_OUTPUT(0, output);
    END_PROC_FUNC
}

syn::GainUnit::GainUnit(const string& a_name) :
    Unit(a_name),
    m_pGain(addParameter_(UnitParameter("gain", -1E6, 1E6, 1.0, UnitParameter::None, 2).setControlType(UnitParameter::Unbounded)))
{
    for (int i = 0; i < MAX_OUTPUTS; i++) {
        addInput_(std::to_string(i), 1.0);
    }
    addOutput_("out");
}

syn::GainUnit::GainUnit(const GainUnit& a_rhs) :
    GainUnit(a_rhs.name()) { }

void syn::GainUnit::process_()
{
    BEGIN_PROC_FUNC
        double output = param(m_pGain).getDouble();
        for (int i = 0; i < numInputs(); i++) {
            int id = inputs().ids()[i];
            if (isInputConnected(id))
                output *= READ_INPUT(id);
        }
        WRITE_OUTPUT(0, output);
    END_PROC_FUNC
}

syn::ConstantUnit::ConstantUnit(const string& a_name) :
    Unit(a_name)
{
    addParameter_(UnitParameter{"out",-1E6,1E6,0.0,UnitParameter::None,2});
    param("out").setControlType(UnitParameter::EControlType::Unbounded);
    addOutput_("out");
}

syn::ConstantUnit::ConstantUnit(const ConstantUnit& a_rhs) :
    ConstantUnit(a_rhs.name()) { }

void syn::ConstantUnit::process_()
{
    BEGIN_PROC_FUNC
        double output = param(0).getDouble();
        WRITE_OUTPUT(0, output);
    END_PROC_FUNC
}

syn::PanningUnit::PanningUnit(const string& a_name) :
    Unit(a_name)
{
    addInput_("in1");
    addInput_("in2");
    addInput_("bal1");
    addInput_("bal2");
    addOutput_("out1");
    addOutput_("out2");
    m_pBalance1 = addParameter_({"bal1",-1.0,1.0,0.0});
    m_pBalance2 = addParameter_({"bal2",-1.0,1.0,0.0});
}

syn::PanningUnit::PanningUnit(const PanningUnit& a_rhs) :
    PanningUnit(a_rhs.name()) { }

void syn::PanningUnit::process_()
{
    BEGIN_PROC_FUNC
        double in1 = READ_INPUT(0);
        double in2 = READ_INPUT(1);
        double bal1 = param(m_pBalance1).getDouble() + READ_INPUT(2);
        double bal2 = param(m_pBalance2).getDouble() + READ_INPUT(3);
        bal1 = 0.5 * (1 + CLAMP(bal1, -1.0, 1.0));
        bal2 = 0.5 * (1 + CLAMP(bal2, -1.0, 1.0));
        WRITE_OUTPUT(0, (1 - bal1) * in1 + (1 - bal2) * in2);
        WRITE_OUTPUT(1, bal1 * in1 + bal2 * in2);
    END_PROC_FUNC
}

syn::LerpUnit::LerpUnit(const string& a_name) :
    Unit(a_name)
{
    m_pMinInput = addParameter_(UnitParameter("min in", -1E6, 1E6, 0.0).setControlType(UnitParameter::Unbounded));
    m_pMaxInput = addParameter_(UnitParameter("max in", -1E6, 1E6, 1.0).setControlType(UnitParameter::Unbounded));
    m_pMinOutput = addParameter_(UnitParameter("min out", -1E6, 1E6, 0.0).setControlType(UnitParameter::Unbounded));
    m_pMaxOutput = addParameter_(UnitParameter("max out", -1E6, 1E6, 1.0).setControlType(UnitParameter::Unbounded));
    m_pClip = addParameter_(UnitParameter("clip", false));
    addInput_("in");
    addOutput_("out");
}

syn::LerpUnit::LerpUnit(const LerpUnit& a_rhs) :
    LerpUnit(a_rhs.name()) { }

void syn::LerpUnit::process_()
{
    BEGIN_PROC_FUNC
        double input = READ_INPUT(0);
        double aIn = param(m_pMinInput).getDouble();
        double bIn = param(m_pMaxInput).getDouble();
        double aOut = param(m_pMinOutput).getDouble();
        double bOut = param(m_pMaxOutput).getDouble();
        double inputNorm = INVLERP(aIn, bIn, input);
        double output = LERP(aOut, bOut, inputNorm);
        if (param(m_pClip).getBool())
            output = CLAMP(output, MIN(aOut, bOut), MAX(aOut, bOut));
        WRITE_OUTPUT(0, output);
    END_PROC_FUNC
}


syn::TanhUnit::TanhUnit(const string& a_name) : Unit(a_name) {
    addParameter_(pSat, UnitParameter("sat", 1.0, 10.0, 1.0));
    addInput_("in");
    addOutput_("out");
}

syn::TanhUnit::TanhUnit(const TanhUnit& a_rhs) : TanhUnit(a_rhs.name()) {}

void syn::TanhUnit::process_() {
    BEGIN_PROC_FUNC
    double input = READ_INPUT(0);
    double sat = param(pSat).getDouble();
    WRITE_OUTPUT(0, fast_tanh_rat(input * sat) / fast_tanh_rat(sat));
    END_PROC_FUNC
}

syn::QuantizerUnit::QuantizerUnit(const string& a_name) : Unit(a_name) {
    addParameter_(0, UnitParameter("step", 0.0, 1e6, 1.0).setControlType(UnitParameter::Unbounded));
    addInput_(iIn, "in");
    addInput_(iStep, "step");
    addOutput_(0, "out");
}

void syn::QuantizerUnit::process_() {
    BEGIN_PROC_FUNC
        double in = READ_INPUT(iIn);
    double quantStep = CLAMP(param(pStep).getDouble() + READ_INPUT(iStep), param(pStep).getMin(), param(pStep).getMax());
    double out = quantStep>0 ? quantStep * std::floor(in / quantStep + 0.5) : in;
    WRITE_OUTPUT(0, out);
    END_PROC_FUNC
}