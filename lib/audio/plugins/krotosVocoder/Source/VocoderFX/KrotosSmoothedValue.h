#pragma once

/*
==============================================================================

KrotosSmooothedValue.h
Created: 06 June 2018 12:36:11pm
Author:  Dave Stevenson

==============================================================================
*/

#include <math.h>

class KrotosSmoothedValue
{
public:
	KrotosSmoothedValue() {}

	KrotosSmoothedValue(float value)
	{
		m_currentValue = value;
		m_target = value;
	}

	void reset(float sampleRate, float rateOfChangeSeconds)
	{
		m_stepsToTarget = (int)floor(rateOfChangeSeconds * sampleRate);
		m_currentValue = m_target;
		m_countdown = 0;
	}

	void setValue(float value)
	{
		if (m_target != value)
		{
			m_target = value;
			m_countdown = m_stepsToTarget;

			if (m_countdown <= 0)
			{
				m_currentValue = m_target;
			}
			else
			{
				m_step = (m_target - m_currentValue) / (float)m_countdown;
			}
		}
	}

	float getNextValue()
	{
		if (m_countdown <= 0)
		{
			return m_target;
		}

		m_countdown--;
		m_currentValue += m_step;
		return m_currentValue;
	}

	bool isSmoothing()
	{
		return m_countdown > 0;
	}

	float getTarget()
	{
		return m_target;
	}
private:

	float m_currentValue{ 0 };
	float m_target{ 0 };
	float m_step{ 0 };
	int m_countdown{ 0 };
	int m_stepsToTarget{ 0 };
};
