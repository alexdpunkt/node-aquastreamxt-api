/**
 * @package node-aquastreamxt-api
 * @author Alexander Dick <alex@dick.at>
 */

#ifndef IO_H
#define IO_H

#include <v8.h>

using namespace v8;

class IO {

	private:

		static const int REPORT_LENGTH = 512;

	public:

		static int openDevice(int vendorId, int productId);
		static int isAquastreamXt(int handle, int vendorId, int productId);
		static int getFeatureReport(int handle,	int reportId, unsigned char *buffer);
		static int setFeatureReport(int handle,	int reportId, unsigned char *buffer);
		static Handle<Object> getSettings(Local<Value> handle, Local<Value> reportId);
		static Handle<Object> getData(Local<Value> handle, Local<Value> reportId, Handle<Object> settings);
		static Handle<Value> setSettings(Local<Value> handle, Local<Value> reportId, Handle<Object> settings);
		static Handle<Object> getDeviceInfo(Local<Value> handle);

		struct pumpDataReport {

			// rawSensorData[3] = fan voltage
			// rawSensorData[4] = pump voltage
			// rawSensorData[5] = pump current
			u_int16_t rawSensorData[6];

			u_int16_t temperatureRaw[3];

			u_int16_t frequency;
			u_int16_t frequencyMax;

			u_int32_t flow;
			u_int32_t fanRpm;

			u_int8_t fanPower;

			u_int8_t alarmSensor0:1;
			u_int8_t alarmSensor1:1;
			u_int8_t alarmDummy1:1;
			u_int8_t alarmFan:1;
			u_int8_t alarmFlow:1;
			u_int8_t alarmDummy2:3;

			u_int8_t modeAdvancedPumpSettings:1;
			u_int8_t modeAquastreamModeAdvanced:1;
			u_int8_t modeAquastreamModeUltra:1;
			u_int8_t modeDummy:5;

			u_int32_t controllerOut;
			int32_t controllerI;
			int32_t controllerP;
			int32_t controllerD;

			u_int16_t firmware;
			u_int16_t bootloader;
			u_int16_t hardware;

			u_int8_t dummy[2];

			u_int16_t serial;

			u_int8_t publicKey[6];

		} __attribute__((__packed__));

		struct pumpSettingsReport {

			u_int8_t i2cAddress;

			u_int8_t i2cSetting_aquabusEnable:1;
			u_int8_t i2cSetting_dummy:7;

			u_int8_t pumpMode_deaeration:1;
			u_int8_t pumpMode_autoPumpMaxFreq:1;
			u_int8_t pumpMode_deaerationModeSens:1;
			u_int8_t pumpMode_resetPumpMaxFreq:1;
			u_int8_t pumpMode_i2cControl:1;
			u_int8_t pumpMode_minFreqForce:1;
			u_int8_t pumpMode_dummy:2;
			u_int8_t pumpModeB;

			u_int8_t sensorBridge;

			u_int8_t measureFanEdges;
			u_int8_t measureFlowEdges;

			u_int16_t pumpFrequency;
			u_int32_t frequencyResetCycle;

			u_int8_t alarm_sensor0:1;
			u_int8_t alarm_sensor1:1;
			u_int8_t alarm_pump:1;
			u_int8_t alarm_fan:1;
			u_int8_t alarm_flow:1;
			u_int8_t alarm_fanShort:1;
			u_int8_t alarm_fanOverTemp90:1;
			u_int8_t alarm_fanOverTemp70:1;

			u_int8_t tachoMode_linkFan:1;
			u_int8_t tachoMode_linkFlow:1;
			u_int8_t tachoMode_linkPump:1;
			u_int8_t tachoMode_linkStatic:1;
			u_int8_t tachoMode_linkAlarmInterrupt:1;
			u_int8_t tachoMode_dummy:3;

			u_int16_t tachoFrequency;

			u_int32_t flowAlarmValue;
			u_int16_t sensorAlarmTemperature[2];

			u_int8_t fanMode_manual:1;
			u_int8_t fanMode_auto:1;
			u_int8_t fanMode_holdMinPower:1;
			u_int8_t fanMode_dummy:5;

			u_int8_t fanManualPower;

			u_int16_t controllerHysterese;
			u_int8_t controllerSensor;
			u_int16_t controllerSetTemp;
			u_int16_t controllerP;
			u_int16_t controllerI;
			u_int16_t controllerD;

			u_int16_t sensorMinTemperature;
			u_int16_t sensorMaxTemperature;

			u_int8_t fanMinimumPower;
			u_int8_t fanMaximumPower;

			u_int8_t ledSettings;
			u_int8_t aquabusTimeout;

			u_int16_t minPumpFrequency;
			u_int16_t maxPumpFrequency;

		} __attribute__((__packed__));

};

#endif