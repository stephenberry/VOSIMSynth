#ifndef __VOICEMANAGER__
#define __VOICEMANAGER__

#include "Circuit.h"
#include "Unit.h"
#include "UnitFactory.h"
#include "stk/Mutex.h"
#include <map>
#include <string>
#include <list>
#include <memory>

using std::list;
using std::map;
using std::string;
using std::shared_ptr;
using stk::Mutex;

namespace syn {

    /* Modifying actions that should be queued and processed in between samples */
    enum EMuxAction {
        ModifyParam,
        ModifyParamNorm,
        DeleteUnit,
        ConnectInput,
        ConnectOutput,
        ConnectInternal,
        DisconnectInput,
        DisconnectOutput,
        DisconnectInternal
    };

    struct MuxArgs {
        int id1;
        int id2;
        union{
            int id3;
            double value;
        };
        int id4;
    };

    class VoiceManager {
    public:
        VoiceManager(shared_ptr<Circuit> a_proto, shared_ptr<UnitFactory> a_factory) :
                m_numVoices(0),
                m_maxVoices(0),
				m_tickCount(0),
                m_instrument(a_proto),
                m_factory(a_factory)
        {
        };

        ~VoiceManager()
        {
            m_allVoices.clear();
        }

        void tick(double& left_output, double& right_output);

        /**
         * Queue an action to be processed before the next sample.
         *
         * The following is a description of parameter signatures corresponding to each possible action:
         * ModifyParam:         (int unit_id, int param_id, double value)
         * ModifyParamNorm:     (int unit_id, int param_id, double norm_value)
         * DeleteUnit:          (int unit_id)
         * ConnectInput:        (int circuit_input_id, int unit_id, int unit_input_id)
         * ConnectOutput:       (int circuit_output_id, int unit_id, int output_port_id)
         * ConnectInternal:     (int from_unit_id, int from_unit_port, int to_unit_id, int to_unit_port)
         * DisconnectInput:     (int circuit_input_id, int unit_id, int input_port_id)
         * DisconnectOutput:    (int circuit_output_id, int unit_id, int output_port_id)
         * DisconnectInternal:  (int from_unit_id, int from_unit_port, int to_unit_id, int to_unit_port
         */
		unsigned queueAction(EMuxAction a_action, const MuxArgs& a_params);

        void doAction(EMuxAction a_action, const MuxArgs& a_params);

		unsigned getTickCount() const;

        void setFs(double a_newFs);

        void setTempo(double a_newTempo);

        void noteOn(int a_noteNumber, int a_velocity);

        void noteOff(int a_noteNumber, int a_velocity);

        void setMaxVoices(unsigned a_newMax);

        int getNumVoices() const;

        int getMaxVoices() const;

        const Unit& getUnit(int a_id);

		template<typename T>
        int addUnit(T a_prototypeId);

        int getNumUnits() const;

        const Circuit& getCircuit() const;

    private:
        /**
         * Processes all actions from the action queue
         */
        void _flushActionQueue();

        /**
         * Processes the next action from the action queue
         */
        void _processAction(EMuxAction a_action, const MuxArgs& a_params);

        int _createVoice(int a_note, int a_velocity);

        void _makeIdle();

        void _makeIdle(int a_voiceIndex);

        int _findIdleVoice();

        int _getLowestVoiceIndex();

        int _getNewestVoiceIndex();

        int _getOldestVoiceIndex();

        int _getHighestVoiceIndex();

    private:
        typedef list<int> VoiceList;
        typedef map<int, VoiceList> VoiceMap;
        int m_numVoices;
        int m_maxVoices;
		unsigned m_tickCount;
        VoiceMap m_voiceMap;
        VoiceList m_voiceStack;
        VoiceList m_idleVoiceStack;
        vector<shared_ptr<Circuit> > m_allVoices;
        shared_ptr<Circuit> m_instrument;
        shared_ptr<UnitFactory> m_factory;
        list<pair<EMuxAction, MuxArgs> > m_queuedActions;

		Mutex m_queueMutex, m_voiceMutex;
    };

	template <typename T>
	int VoiceManager::addUnit(T a_prototypeId) {
		shared_ptr<Circuit> voice;
		// Apply action to all voices
		int numVoices = m_allVoices.size();
		int returnId = -1;
		shared_ptr<Unit> unit = m_factory->createUnit(a_prototypeId);
		for (int i = 0; i <= numVoices; i++) {
			if (i == m_allVoices.size()) { // Apply action to prototype voice at end of loop
				voice = m_instrument;
				returnId = voice->addUnit(unit);
			}
			else {
				voice = m_allVoices[i];
				returnId = voice->addUnit(shared_ptr<Unit>(unit->clone()));
			}
		}
		return returnId;
	}
}
#endif
