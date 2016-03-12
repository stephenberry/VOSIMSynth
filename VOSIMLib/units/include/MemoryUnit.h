﻿/*
Copyright 2016, Austen Satterlee

This file is part of VOSIMProject.

VOSIMProject is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

VOSIMProject is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with VOSIMProject. If not, see <http://www.gnu.org/licenses/>.
*/

/**
 * \file MemoryUnit.h
 * \brief
 * \details
 * \author Austen Satterlee
 * \date March 6, 2016
 */
#ifndef __MEMORYUNIT__
#define __MEMORYUNIT__

#include "Unit.h"
#include <DSPMath.h>

using namespace std;

namespace syn {	
	class MemoryUnit : public Unit {
	public:
		MemoryUnit(const string& a_name) :
			Unit(a_name),
			m_pBufSize(addParameter_(UnitParameter("samples", 1, 16384, 1)))
		{
			addInput_("in");
			addOutput_("out");
		}
		MemoryUnit(const MemoryUnit& a_rhs) :
			MemoryUnit(a_rhs.getName()) {}
	protected:
		void onParamChange_(int a_paramId) override;

		void process_(const SignalBus& a_inputs, SignalBus& a_outputs) override;
	private:
		string _getClassName() const override { return "MemoryUnit"; }

		Unit* _clone() const override { return new MemoryUnit(*this); }
	private:
		NSampleDelay m_delay;
		int m_pBufSize;
	};
}
#endif

