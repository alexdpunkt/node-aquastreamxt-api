#ifndef CONVERT_H
#define CONVERT_H

#include <sys/types.h>

class Convert {

	public:

		static double staticTachoRpm(u_int32_t value);
		static u_int32_t toStaticTachoRpm(double value);

		static double temperature(u_int16_t temperature);
		static u_int16_t toTemperature(double temperature);

		static double flow(u_int32_t rawFlow, int measureEdges, int calImpulse);

		static int fanRpm(u_int32_t rawRpm, int measureEdges);

		static double frequency(u_int32_t time);
		static u_int32_t toFrequency(double frequency);

		static int frequencyResetCycle(u_int32_t frequencyResetCycle);
		static u_int32_t toFrequencyResetCycle(int frequencyResetCycle);

		static double frequencyLimiter(u_int32_t time);
		static u_int32_t toFrequencyLimiter(double frequency);

		static double voltage(u_int32_t rawVoltage);
		static int current(u_int32_t rawCurrent);
		static double fanVoltage(u_int32_t rawVoltage);

		static double scalePercent(u_int16_t value);
		static u_int16_t toScalePercent(double value);

		static double controllerOutScale(int32_t value);

    private:

		static const int 		CPU_CLOCK 			= 12000000; 	// clock of the pump
		static const int 		TIMER_PUMP 			= 1500000; 		// timer frequency / Timer Frequenz (CPU_CLOCK / 8)

		static const int 		CONTROLLER_TIMER 	= 100; 			// Controller cycle time 100ms
		static const double 	LIMITER_SCALE 		= 0.01333155;	// Frequency Limiter scale
		static const int 		TIMER_RPM 			= 46875;		// Scale for Fan RPM
		static const int		MIN_RPM				= 300000;		// Minimum Fan RPM
		static const int		MIN_FLOW 			= 600000;		// Minimum Fan RPM

		static const int 		SCALE_TEMPERATURE 	= 100; 			// Scale of Temperature
		static const double 	SCALE_12V 			= 61.00;		// Skalierungsfaktor 12V
		static const double 	SCALE_FAN_OUT 		= 63.00;		// Skalierungsfaktor Fanout
		static const double 	SCALE_PUMP_CURRENT	= 1.6;			// Skalierungsfaktor Pumpenstrom

};

#endif