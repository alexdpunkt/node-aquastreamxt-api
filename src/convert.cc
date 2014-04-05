/**
 * Some converting functions
 *
 * @package node-aquastreamxt-api
 * @author Alexander Dick <alex@dick.at>
 */

#include "convert.h"

double Convert::temperature(u_int16_t temperature) {

	double res = 0;

	res = temperature;
	res /= SCALE_TEMPERATURE;

	return res;
};

u_int16_t Convert::toTemperature(double temperature) {

	u_int16_t res = 0;

	res = temperature;
	res *= SCALE_TEMPERATURE;

	return res;
};

double Convert::staticTachoRpm(u_int32_t value) {
	double ret = (TIMER_RPM * 60) / ((double)value * 9.9);
	return ret;
};

u_int32_t Convert::toStaticTachoRpm(double value) {
	u_int32_t ret = (TIMER_RPM * 60) * ((double)value * 9.9);
	return ret;
};

double Convert::flow(u_int32_t rawFlow, int measureEdges, int calImpulse) {

	double res = 0;
	double time, tempFlow, cal;

    time = TIMER_RPM; //impulses per sec
	time *= 3600;

	cal 		= ((double)measureEdges/2) / calImpulse;
	tempFlow 	= time / (rawFlow / cal);

	//check minimum flow
	if (rawFlow >= MIN_FLOW)
		tempFlow = 0;

	res = tempFlow;

	return res;
};

int Convert::fanRpm(u_int32_t rawRpm, int measureEdges) {

	int res = 0;
	double temp, tempRpm, cal;

	temp = TIMER_RPM; //impulses per sec
	temp *= 60;

	cal = (double)measureEdges / 4;

	tempRpm = rawRpm / cal;
	temp = temp / tempRpm;

	if (rawRpm >= MIN_RPM)
		temp = 0;

	res = (int)temp;

	return res;
};

double Convert::frequency(u_int32_t time) {

	double res = TIMER_PUMP;

	res /= (double)time;
	res /= (double)2;

	return res;
};

u_int32_t Convert::toFrequency(double frequency) {

	u_int32_t res = 0;

	res = (int)(frequency * 2);
	res = TIMER_PUMP / res;

	return res;
};


int Convert::frequencyResetCycle(u_int32_t frequencyResetCycle) {
	return frequencyResetCycle / 0x113;
};

u_int32_t Convert::toFrequencyResetCycle(int frequencyResetCycle) {
	return frequencyResetCycle * 0x113;
};

double Convert::frequencyLimiter(u_int32_t time) {
	return time * LIMITER_SCALE;
}

u_int32_t Convert::toFrequencyLimiter(double frequency) {

	u_int32_t res = 0;
	res = (u_int32_t)(frequency / LIMITER_SCALE);

	if (res > 255)
		res = 255;

	if (res <= 0)
		res = 1;

	return res;
};

double Convert::voltage(u_int32_t rawVoltage) {
	return (double)rawVoltage / (double)SCALE_12V;
}

int Convert::current(u_int32_t rawCurrent) {
	return (int)(rawCurrent * SCALE_PUMP_CURRENT);
};

double Convert::fanVoltage(u_int32_t rawVoltage) {
	return (double)rawVoltage / (double)SCALE_FAN_OUT;
}

double Convert::scalePercent(u_int16_t value) {

	double res = (double)value;
	res /= 2.55;

	return res;
}

u_int16_t Convert::toScalePercent(double value) {

	u_int16_t res = 0;

	value *= 2.55;

	if (value > 255)
		value = 255;

	if (value < 0)
		value = 0;

	res = value;

	return res;
}

double Convert::controllerOutScale(int32_t value) {

	double res;

	res = 0xffffff / SCALE_TEMPERATURE;

	res = value / res;
	return (res);

};