/**
 * \file MidiUnits.h
 * \brief
 * \details
 * \author Austen Satterlee
 * \date March 6, 2016
 */
#ifndef __MIDIUNITS__
#define __MIDIUNITS__

#include "Unit.h"

using namespace std;

namespace syn {
	/** 
	 * Outputs the current midi note
	 */
    class MidiNoteUnit : public Unit {
    public:
        explicit MidiNoteUnit(const string& a_name) :
                Unit(a_name)
        {
            addOutput_("out");
        }

        MidiNoteUnit(const MidiNoteUnit& a_rhs) :
                MidiNoteUnit(a_rhs.getName())
        {

        }

    protected:
        void MSFASTCALL process_(const SignalBus& a_inputs, SignalBus& a_outputs) GCCFASTCALL override
        {
            a_outputs.setChannel(0, getNote()*0.0078125);
        };
    private:
        string _getClassName() const override
        {
            return "MidiNoteUnit";
        }

	    Unit* _clone() const override { return new MidiNoteUnit(*this); }
    };	

	/**
	 * Outputs the current velocity
	 */
	class VelocityUnit : public Unit {
	public:
		explicit VelocityUnit(const string& a_name) :
			Unit(a_name)
		{
			addOutput_("out");
		}

		VelocityUnit(const VelocityUnit& a_rhs) :
			VelocityUnit(a_rhs.getName())
		{

		}

	protected:
		void MSFASTCALL process_(const SignalBus& a_inputs, SignalBus& a_outputs) GCCFASTCALL override
		{
			a_outputs.setChannel(0, getVelocity()*0.0078125);
		};
	private:
		string _getClassName() const override
		{
			return "VelocityUnit";
		}

		Unit* _clone() const override { return new VelocityUnit(*this); }
	};

	/**
	* Outputs a 1 when a key is pressed, and 0 otherwise
	*/
	class GateUnit : public Unit {
	public:
		explicit GateUnit(const string& a_name) :
			Unit(a_name)
		{
			addOutput_("out");
		}

		GateUnit(const GateUnit& a_rhs) :
			GateUnit(a_rhs.getName())
		{

		}

	protected:
		void MSFASTCALL process_(const SignalBus& a_inputs, SignalBus& a_outputs) GCCFASTCALL override
		{
			a_outputs.setChannel(0, static_cast<double>(isNoteOn()));
		};
	private:
		string _getClassName() const override
		{
			return "GateUnit";
		}

		Unit* _clone() const override { return new GateUnit(*this); }
	};

	/**
	 * Outputs the value of the selected Midi CC
	 */
	class MidiCCUnit : public Unit {
	
	public:
		explicit MidiCCUnit(const string& a_name) :
			Unit(a_name),
			m_pCC(addParameter_(UnitParameter("CC", 0, 128, 0))),
			m_pLearn(addParameter_(UnitParameter("learn", false))),
			m_value(0)
		{
			addOutput_("out");
		}

		MidiCCUnit(const MidiCCUnit& a_rhs) :
			MidiCCUnit(a_rhs.getName())
		{

		}

	protected:
		void MSFASTCALL process_(const SignalBus& a_inputs, SignalBus& a_outputs) GCCFASTCALL override
		{
			a_outputs.setChannel(0, m_value); // divide by 128
		};

		void onParamChange_(int a_paramId) override {
			if(a_paramId==m_pCC) {
				m_value = 0;
				setParameter(m_pLearn, false);
			}
		}

		void onMidiControlChange_(int a_cc, double a_value) override {
			if(getParameter(m_pLearn).getBool()) {
				setParameter(m_pCC, a_cc);
			}
			if (a_cc == getParameter(m_pCC).getInt()) {
				m_value = a_value;
			}
		}
	private:
		string _getClassName() const override
		{
			return "MidiCCUnit";
		}

		Unit* _clone() const override { return new MidiCCUnit(*this); }
	private:
		int m_pCC, m_pLearn;
		double m_value;
	};
}
#endif