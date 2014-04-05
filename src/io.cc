/**
 * Functions for communicating with the device
 *
 * @package node-aquastreamxt-api
 * @author Alexander Dick <alex@dick.at>
 */

#include <node.h>
#include <v8.h>

#include <linux/hiddev.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include "io.h"
#include "convert.h"

using namespace v8;

/**
 * Checks if provided handle is an Aquastream XT pump
 *
 * @param int handle
 */
int IO::isAquastreamXt(int handle, int vendorId, int productId) {

	struct hiddev_devinfo deviceInfo;

	ioctl(handle, HIDIOCGDEVINFO, &deviceInfo);

	return (
		(deviceInfo.vendor == vendorId) &&
		((deviceInfo.product & 0xffff) == productId)
	);

};

/**
 * Returns the correct handle (file descriptor) on success
 *
 * @param int vendorId
 * @param int productId
 * @return int
 */
int IO::openDevice(int vendorId, int productId) {

	char devicePath[24];

	const char *devicePaths[] = {
		"/dev/usb/hiddev\%d",
		"/dev/usb/hid/hiddev\%d",
		"/dev/hiddev\%d",
		NULL
	};

	unsigned int numIterations = 15, i, j;
	int handle;

	for (i = 0; devicePaths[i]; i++) {

		for (j = 0; j < numIterations; j++) {

			sprintf(devicePath, devicePaths[i], j);

			if((handle = open(devicePath, O_RDONLY)) >= 0) {

				if(isAquastreamXt(handle, vendorId, productId))
					return handle;

				close(handle);
			}
		}
	};

	ThrowException(Exception::Error(String::New("Couldn't find Aquastream XT!")));
	return 0;
};

/**
 * Gets a HID feature report
 * @param int handle				The device handle
 * @param int reportId				The requested Report number
 * @param unsigned char *buffer
 */
int IO::getFeatureReport(
	int handle,
	int reportId,
	unsigned char *buffer
) {

	struct hiddev_report_info       reportInfo;
	struct hiddev_field_info        fieldInfo;
	struct hiddev_usage_ref_multi   usageRef;

	fieldInfo.report_type 	= HID_REPORT_TYPE_FEATURE;
	fieldInfo.report_id 	= reportId;
	fieldInfo.field_index 	= 0;

	int ret 		 = ioctl(handle, HIDIOCGFIELDINFO, &fieldInfo);
	int reportLength = fieldInfo.maxusage;

	if (reportLength > REPORT_LENGTH) {
		ThrowException(Exception::TypeError(String::New("Invalid Report: too large")));
		return 0;
	}

	reportInfo.report_type = HID_REPORT_TYPE_FEATURE;
	reportInfo.report_id = reportId;
	reportInfo.num_fields = 1;

	usageRef.uref.report_type = HID_REPORT_TYPE_FEATURE;
	usageRef.uref.report_id = reportId;
	usageRef.uref.field_index = 0;
	usageRef.uref.usage_index = 0;
	usageRef.num_values = reportLength;

	// get info report
	ret = ioctl(handle, HIDIOCGREPORT, &reportInfo);

	if (ret != 0) {
		ThrowException(Exception::Error(String::New("HIDIOCGREPORT error")));
		return 0;
	}

	// get usage report
	ret = ioctl(handle, HIDIOCGUSAGES, &usageRef);

	if (ret != 0) {
		ThrowException(Exception::Error(String::New("HIDIOCGUSAGES error")));
		return 0;
	}

	// transfer to local buffer
	int i;
	for (i = 0; i < reportLength - 1; i++)
		buffer[i] = usageRef.values[i];

	return reportLength;
};

/**
 * Sets a feature report
 * @param int handle
 * @param int reportId
 * @param unsigned char *buffer
 * @return int reportLength
 */
int IO::setFeatureReport(
	int handle,
	int reportId,
	unsigned char *buffer
) {

	struct hiddev_report_info 		reportInfo;
	struct hiddev_field_info 		fieldInfo;
	struct hiddev_usage_ref_multi 	usageRef;

	fieldInfo.report_type 	= HID_REPORT_TYPE_FEATURE;
	fieldInfo.report_id 	= reportId;
	fieldInfo.field_index 	= 0;

	int ret 		 = ioctl(handle, HIDIOCGFIELDINFO, &fieldInfo);
	int reportLength = fieldInfo.maxusage;

	if (reportLength > REPORT_LENGTH) {
		ThrowException(Exception::Error(String::New("Received report too large")));
		return 0;
	}

	reportInfo.report_type = HID_REPORT_TYPE_FEATURE;
	reportInfo.report_id = reportId;
	reportInfo.num_fields = 1;

	usageRef.uref.report_type = HID_REPORT_TYPE_FEATURE;
	usageRef.uref.report_id = reportId;
	usageRef.uref.field_index = 0;
	usageRef.uref.usage_index = 0;
	usageRef.num_values = reportLength;


	// copy bytes to output buffer
	// can't use memcpy, because values are signed int32
	int i;
	for(i = 0; i < reportLength-1; i++)
		usageRef.values[i] = buffer[i];

	// multibyte transfer to device
	ret = ioctl(handle, HIDIOCSUSAGES, &usageRef);

	if (ret != 0) {
		ThrowException(Exception::Error(String::New("HIDIOCSUSAGE error")));
		return 0;
	}

	// write report to device
    ret = ioctl(handle, HIDIOCSREPORT, &reportInfo);

	if (ret != 0) {
		ThrowException(Exception::Error(String::New("HIDIOCSREPORT error")));
		return 0;
	}

	return reportLength;
};

/**
 * Returns a Node readable object of the pumpDataReport struct
 * @param Local<Value> handle
 * @param Local<Value> reportId
 * @param Handle<Object> settings
 * @return Local<Object> data
 */
Handle<Object> IO::getData(Local<Value> handle, Local<Value> reportId, Handle<Object> settings) {

	HandleScope scope;

	unsigned char *buffer 			= (unsigned char*) malloc(REPORT_LENGTH);
	struct pumpDataReport *report 	= (struct pumpDataReport*) buffer;

	int bytes;

	Local<Object> data = Object::New();

	bytes = getFeatureReport(handle->NumberValue(), reportId->NumberValue(), buffer);

	if(bytes <= 0)	{
		free(buffer);
		ThrowException(Exception::Error(String::New("Couldn't get data report")));
		return data;
	}

	// Controller data
	Local<Object> controller = Object::New();

		controller->Set(String::NewSymbol("i"), Number::New(Convert::controllerOutScale(report->controllerI)));
		controller->Set(String::NewSymbol("p"), Number::New(Convert::controllerOutScale(report->controllerP)));
		controller->Set(String::NewSymbol("d"), Number::New(Convert::controllerOutScale(report->controllerD)));
		controller->Set(String::NewSymbol("output"), Number::New(Convert::controllerOutScale(report->controllerOut)));

		data->Set(String::NewSymbol("controller"), Local<Object>::New(controller));

	// Current values
	Local<Object> current = Object::New();

		current->Set(String::NewSymbol("flow"), Number::New(report->flow));

		current->Set(String::NewSymbol("frequency"), Integer::New(Convert::frequency(report->frequency)));

		current->Set(String::NewSymbol("frequencyMax"), Integer::New(
			Convert::fanRpm(
				report->frequencyMax,
				settings->Get(String::NewSymbol("measureFanEdges"))->NumberValue()
			)
		));

		current->Set(String::NewSymbol("fanVoltageMeasured"), Number::New(Convert::fanVoltage(report->rawSensorData[3])));
		current->Set(String::NewSymbol("fanVoltage"), Number::New(
			Convert::voltage(report->rawSensorData[4]) * (Convert::scalePercent(report->fanPower) / 100)
		));
		current->Set(String::NewSymbol("voltage"), Number::New(Convert::voltage(report->rawSensorData[4])));
		current->Set(String::NewSymbol("pumpCurrent"), Number::New(Convert::current(report->rawSensorData[5])));
		current->Set(String::NewSymbol("pumpPower"), Number::New(
			(Convert::current(report->rawSensorData[5]) * Convert::voltage(report->rawSensorData[4])) / 1000
		));

		current->Set(String::NewSymbol("fanRpm"), Number::New(
			Convert::fanRpm(
				report->fanRpm,
				settings->Get(String::NewSymbol("measureFanEdges"))->NumberValue()
			)
		));

		// Temperature data
		Local<Object> temperature = Object::New();

			temperature->Set(String::NewSymbol("pump"), Number::New(Convert::temperature(report->temperatureRaw[0])));
			temperature->Set(String::NewSymbol("external"), Number::New(Convert::temperature(report->temperatureRaw[1])));
			temperature->Set(String::NewSymbol("water"), Number::New(Convert::temperature(report->temperatureRaw[2])));

			current->Set(String::NewSymbol("temperature"), Local<Object>::New(temperature));

			data->Set(String::NewSymbol("current"), Local<Object>::New(current));

	// Alarm data
	Local<Object> alarm = Object::New();

		alarm->Set(String::NewSymbol("sensor0"), Number::New(report->alarmSensor0));
		alarm->Set(String::NewSymbol("sensor1"), Number::New(report->alarmSensor1));
		alarm->Set(String::NewSymbol("fan"), Number::New(report->alarmFan));
		alarm->Set(String::NewSymbol("flow"), Number::New(report->alarmFlow));

		data->Set(String::NewSymbol("alarm"), Local<Object>::New(alarm));

	// Pump Mode information
	Local<Object> mode = Object::New();

		mode->Set(String::NewSymbol("advancedPumpSettings"), Number::New(report->modeAdvancedPumpSettings));
		mode->Set(String::NewSymbol("aquastreamModeAdvanced"), Number::New(report->modeAquastreamModeAdvanced));
		mode->Set(String::NewSymbol("aquastreamModeUltra"), Number::New(report->modeAquastreamModeUltra));

		data->Set(String::NewSymbol("mode"), Local<Object>::New(mode));

	// Pump Hardware information
	Local<Object> hardware = Object::New();

		hardware->Set(String::NewSymbol("firmware"), Number::New(report->firmware));
		hardware->Set(String::NewSymbol("bootloader"), Number::New(report->bootloader));
		hardware->Set(String::NewSymbol("hardware"), Number::New(report->hardware));
		hardware->Set(String::NewSymbol("serial"), Number::New(report->serial));

		char tmpKey[4];
		Local<Array> publicKey = Array::New();
		for (int i = 0; i < 6; i++) {
			sprintf(tmpKey, "%02X", report->publicKey[ i ]);
			publicKey->Set(Number::New(i), String::New(tmpKey));
		}

		hardware->Set(String::NewSymbol("publicKey"), Local<Array>::New(publicKey));

		data->Set(String::NewSymbol("hardware"), Local<Object>::New(hardware));

	free(buffer);

	return data;
}

/**
 * Returns a settings object
 * @param Local<Value> handle
 * @param Local<Value> reportId
 * @return Local<Object> settings
 */
Handle<Object> IO::getSettings(Local<Value> handle, Local<Value> reportId) {

	HandleScope scope;

	unsigned char *buffer 				= (unsigned char*) malloc(REPORT_LENGTH);
	struct pumpSettingsReport *report 	= (struct pumpSettingsReport*) buffer;

	int bytes;

	bytes = getFeatureReport(handle->NumberValue(), reportId->NumberValue(), buffer);

	Local<Object> settings = Object::New();

	if(bytes <= 0)	{
		free(buffer);
		ThrowException(Exception::Error(String::New("Couldn't get settings report")));
		return settings;
	}

	// Pump Hardware information
	Local<Object> pumpMode = Object::New();

		pumpMode->Set(String::NewSymbol("deaeration"), Number::New(report->pumpMode_deaeration));
		pumpMode->Set(String::NewSymbol("autoPumpMaxFrequency"), Number::New(report->pumpMode_autoPumpMaxFreq));
		pumpMode->Set(String::NewSymbol("deaerationModeSensor"), Number::New(report->pumpMode_deaerationModeSens));
		pumpMode->Set(String::NewSymbol("resetPumpMaxFrequency"), Number::New(report->pumpMode_resetPumpMaxFreq));
		pumpMode->Set(String::NewSymbol("i2cControl"), Number::New(report->pumpMode_i2cControl));
		pumpMode->Set(String::NewSymbol("minFrequencyForce"), Number::New(report->pumpMode_minFreqForce));
		pumpMode->Set(String::NewSymbol("pumpModeB"), Number::New(report->pumpModeB));

		settings->Set(String::NewSymbol("pumpMode"), Local<Object>::New(pumpMode));

	// i2c settings
	Local<Object> i2c = Object::New();

		i2c->Set(String::NewSymbol("address"), Number::New(report->i2cAddress));
		i2c->Set(String::NewSymbol("settingAquabusEnable"), Number::New(report->i2cSetting_aquabusEnable));

		settings->Set(String::NewSymbol("i2c"), Local<Object>::New(i2c));

	settings->Set(String::NewSymbol("sensorBridge"), Number::New(report->sensorBridge));
	settings->Set(String::NewSymbol("measureFanEdges"), Number::New(report->measureFanEdges));
	settings->Set(String::NewSymbol("measureFlowEdges"), Number::New(report->measureFlowEdges));

	// Pump frequency information
	Local<Object> frequency = Object::New();

		Local<Object> pumpFrequency = Object::New();

			pumpFrequency->Set(String::NewSymbol("current"), Integer::New(Convert::frequency(report->pumpFrequency)));
			pumpFrequency->Set(String::NewSymbol("min"), Integer::New(Convert::frequency(report->minPumpFrequency)));
			pumpFrequency->Set(String::NewSymbol("max"), Integer::New(Convert::frequency(report->maxPumpFrequency)));

		frequency->Set(String::NewSymbol("pump"), Local<Object>::New(pumpFrequency));
		frequency->Set(String::NewSymbol("resetCycle"), Number::New(Convert::frequencyResetCycle(report->frequencyResetCycle)));

		settings->Set(String::NewSymbol("frequency"), Local<Object>::New(frequency));

	// Alarm information
	Local<Object> alarm = Object::New();

		alarm->Set(String::NewSymbol("sensor0"), Number::New(report->alarm_sensor0));
		alarm->Set(String::NewSymbol("sensor1"), Number::New(report->alarm_sensor1));

		alarm->Set(String::NewSymbol("pump"), Number::New(report->alarm_pump));
		alarm->Set(String::NewSymbol("fan"), Number::New(report->alarm_fan));
		alarm->Set(String::NewSymbol("flow"), Number::New(report->alarm_flow));
		alarm->Set(String::NewSymbol("fanShort"), Number::New(report->alarm_fanShort));
		alarm->Set(String::NewSymbol("fanOverTemp70"), Number::New(report->alarm_fanOverTemp70));
		alarm->Set(String::NewSymbol("fanOverTemp90"), Number::New(report->alarm_fanOverTemp90));

		settings->Set(String::NewSymbol("alarm"), Local<Object>::New(alarm));

	// Tacho information
	Local<Object> tacho = Object::New();

		Local<Object> tachoMode = Object::New();

			tachoMode->Set(String::NewSymbol("linkFan"), Number::New(report->tachoMode_linkFan));
			tachoMode->Set(String::NewSymbol("linkFlow"), Number::New(report->tachoMode_linkFlow));
			tachoMode->Set(String::NewSymbol("linkPump"), Number::New(report->tachoMode_linkPump));
			tachoMode->Set(String::NewSymbol("linkStatic"), Number::New(report->tachoMode_linkStatic));
			tachoMode->Set(String::NewSymbol("linkAlarmInterrupt"), Number::New(report->tachoMode_linkAlarmInterrupt));
			tachoMode->Set(String::NewSymbol("linkFan"), Number::New(report->tachoMode_linkFan));

			tacho->Set(String::NewSymbol("mode"), Local<Object>::New(tachoMode));

		tacho->Set(String::NewSymbol("frequency"), Number::New(Convert::staticTachoRpm(report->tachoFrequency)));
		tacho->Set(String::NewSymbol("flowAlarmValue"), Number::New(report->flowAlarmValue));

		settings->Set(String::NewSymbol("tacho"), Local<Object>::New(tacho));

	//settings->sensorAlarmTemperature[2];
	Local<Object> fanMode = Object::New();

		fanMode->Set(String::NewSymbol("manual"), Number::New(report->fanMode_manual));
		fanMode->Set(String::NewSymbol("auto"), Number::New(report->fanMode_auto));
		fanMode->Set(String::NewSymbol("holdMinPower"), Number::New(report->fanMode_holdMinPower));

		settings->Set(String::NewSymbol("fanMode"), Local<Object>::New(fanMode));

	settings->Set(String::NewSymbol("fanManualPower"), Number::New(Convert::scalePercent(report->fanManualPower)));

	Local<Object> controller = Object::New();

		controller->Set(String::NewSymbol("hysterese"), Number::New(Convert::temperature(report->controllerHysterese)));
		controller->Set(String::NewSymbol("sensor"), Number::New(report->controllerSensor));
		controller->Set(String::NewSymbol("setTemp"), Number::New(Convert::temperature(report->controllerSetTemp)));
		controller->Set(String::NewSymbol("P"), Number::New(report->controllerP));
		controller->Set(String::NewSymbol("I"), Number::New(report->controllerI));
		controller->Set(String::NewSymbol("D"), Number::New(report->controllerD));

		settings->Set(String::NewSymbol("controller"), Local<Object>::New(controller));

	settings->Set(String::NewSymbol("sensorMinTemperature"), Number::New(Convert::temperature(report->sensorMinTemperature)));
	settings->Set(String::NewSymbol("sensorMaxTemperature"), Number::New(Convert::temperature(report->sensorMaxTemperature)));
	settings->Set(String::NewSymbol("fanMinimumPower"), Number::New(report->fanMinimumPower));
	settings->Set(String::NewSymbol("fanMaximumPower"), Number::New(report->fanMaximumPower));
	settings->Set(String::NewSymbol("ledSettings"), Number::New(report->ledSettings));
	settings->Set(String::NewSymbol("aquabusTimeout"), Number::New(report->aquabusTimeout));

	free(buffer);

	return scope.Close(settings);
}

/**
 * Sets the settings
 * @param Local<Value> handle
 * @param Local<Value> reportId
 * @param Handle<Object> settings
 * @return int
 */
Handle<Value> IO::setSettings(Local<Value> handle, Local<Value> reportId, Handle<Object> settings) {

	HandleScope scope;

	unsigned char *buffer 				= (unsigned char*) malloc(REPORT_LENGTH);
	struct pumpSettingsReport *report 	= (struct pumpSettingsReport*) buffer;

	// Pump Hardware information
	Local<Object> pumpMode = settings->Get(String::NewSymbol("pumpMode"))->ToObject();

		report->pumpMode_deaeration = pumpMode->Get(String::NewSymbol("deaeration"))->Uint32Value();
		report->pumpMode_autoPumpMaxFreq = pumpMode->Get(String::NewSymbol("autoPumpMaxFrequency"))->Uint32Value();
		report->pumpMode_deaerationModeSens = pumpMode->Get(String::NewSymbol("deaerationModeSensor"))->Uint32Value();
		report->pumpMode_resetPumpMaxFreq = pumpMode->Get(String::NewSymbol("resetPumpMaxFrequency"))->Uint32Value();
		report->pumpMode_i2cControl = pumpMode->Get(String::NewSymbol("i2cControl"))->Uint32Value();
		report->pumpMode_minFreqForce = pumpMode->Get(String::NewSymbol("minFrequencyForce"))->Uint32Value();
		report->pumpModeB = pumpMode->Get(String::NewSymbol("pumpModeB"))->Uint32Value();

	// i2c settings
	Local<Object> i2c = settings->Get(String::NewSymbol("i2c"))->ToObject();

		report->i2cAddress = i2c->Get(String::NewSymbol("address"))->Uint32Value();
		report->i2cSetting_aquabusEnable = i2c->Get(String::NewSymbol("settingAquabusEnable"))->Uint32Value();

	report->sensorBridge = settings->Get(String::NewSymbol("sensorBridge"))->Uint32Value();
	report->measureFanEdges = settings->Get(String::NewSymbol("measureFanEdges"))->Uint32Value();
	report->measureFlowEdges = settings->Get(String::NewSymbol("measureFlowEdges"))->Uint32Value();

	// Pump frequency information
	Local<Object> frequency = settings->Get(String::NewSymbol("frequency"))->ToObject();

		Local<Object> pumpFrequency = frequency->Get(String::NewSymbol("pump"))->ToObject();

			report->pumpFrequency = Convert::toFrequency(pumpFrequency->Get(String::NewSymbol("current"))->Uint32Value());
			report->minPumpFrequency = Convert::toFrequency(pumpFrequency->Get(String::NewSymbol("min"))->Uint32Value());
			report->maxPumpFrequency = Convert::toFrequency(pumpFrequency->Get(String::NewSymbol("max"))->Uint32Value());

		report->frequencyResetCycle = Convert::toFrequencyResetCycle(frequency->Get(String::NewSymbol("resetCycle"))->Uint32Value());


	// Alarm information
	Local<Object> alarm = settings->Get(String::NewSymbol("alarm"))->ToObject();

		report->alarm_sensor0 = alarm->Get(String::NewSymbol("sensor0"))->Uint32Value();
		report->alarm_sensor1 = alarm->Get(String::NewSymbol("sensor1"))->Uint32Value();

		report->alarm_pump = alarm->Get(String::NewSymbol("pump"))->Uint32Value();
		report->alarm_fan = alarm->Get(String::NewSymbol("fan"))->Uint32Value();
		report->alarm_flow = alarm->Get(String::NewSymbol("flow"))->Uint32Value();
		report->alarm_fanShort = alarm->Get(String::NewSymbol("fanShort"))->Uint32Value();
		report->alarm_fanOverTemp70 = alarm->Get(String::NewSymbol("fanOverTemp70"))->Uint32Value();
		report->alarm_fanOverTemp90 = alarm->Get(String::NewSymbol("fanOverTemp90"))->Uint32Value();

	// Tacho information
	Local<Object> tacho = settings->Get(String::NewSymbol("tacho"))->ToObject();

		Local<Object> tachoMode = tacho->Get(String::NewSymbol("mode"))->ToObject();

			report->tachoMode_linkFan = tachoMode->Get(String::NewSymbol("linkFan"))->Uint32Value();
			report->tachoMode_linkFlow = tachoMode->Get(String::NewSymbol("linkFlow"))->Uint32Value();
			report->tachoMode_linkPump = tachoMode->Get(String::NewSymbol("linkPump"))->Uint32Value();
			report->tachoMode_linkStatic = tachoMode->Get(String::NewSymbol("linkStatic"))->Uint32Value();
			report->tachoMode_linkAlarmInterrupt = tachoMode->Get(String::NewSymbol("linkAlarmInterrupt"))->Uint32Value();
			report->tachoMode_linkFan = tachoMode->Get(String::NewSymbol("linkFan"))->Uint32Value();

		report->tachoFrequency = Convert::toStaticTachoRpm(tacho->Get(String::NewSymbol("frequency"))->Uint32Value());
		report->flowAlarmValue = tacho->Get(String::NewSymbol("flowAlarmValue"))->Uint32Value();

	//settings->sensorAlarmTemperature[2];
	Local<Object> fanMode = settings->Get(String::NewSymbol("fanMode"))->ToObject();

		report->fanMode_manual = fanMode->Get(String::NewSymbol("manual"))->Uint32Value();
		report->fanMode_auto = fanMode->Get(String::NewSymbol("auto"))->Uint32Value();
		report->fanMode_holdMinPower = fanMode->Get(String::NewSymbol("holdMinPower"))->Uint32Value();

	report->fanManualPower = Convert::toScalePercent(settings->Get(String::NewSymbol("fanManualPower"))->Uint32Value());

	Local<Object> controller = settings->Get(String::NewSymbol("controller"))->ToObject();

		report->controllerHysterese = Convert::toTemperature(controller->Get(String::NewSymbol("hysterese"))->Uint32Value());
		report->controllerSensor = controller->Get(String::NewSymbol("sensor"))->Uint32Value();
		report->controllerSetTemp = Convert::toTemperature(controller->Get(String::NewSymbol("setTemp"))->Uint32Value());
		report->controllerP = controller->Get(String::NewSymbol("P"))->Uint32Value();
		report->controllerI = controller->Get(String::NewSymbol("I"))->Uint32Value();
		report->controllerD = controller->Get(String::NewSymbol("D"))->Uint32Value();

	report->sensorMinTemperature = Convert::toTemperature(settings->Get(String::NewSymbol("sensorMinTemperature"))->Uint32Value());
	report->sensorMaxTemperature = Convert::toTemperature(settings->Get(String::NewSymbol("sensorMaxTemperature"))->Uint32Value());
	report->fanMinimumPower = settings->Get(String::NewSymbol("fanMinimumPower"))->Uint32Value();
	report->fanMaximumPower = settings->Get(String::NewSymbol("fanMaximumPower"))->Uint32Value();
	report->ledSettings = settings->Get(String::NewSymbol("ledSettings"))->Uint32Value();
	report->aquabusTimeout = settings->Get(String::NewSymbol("aquabusTimeout"))->Uint32Value();

	int bytes;

	bytes = setFeatureReport(handle->NumberValue(), reportId->NumberValue(), buffer);

	free(buffer);

	if(bytes <= 0)	{
		ThrowException(Exception::Error(String::New("Couldn't set settings report")));
		return Number::New(0);
	}

	return Number::New(1);
}


/**
 * Returns device information
 * @param Local<Value> handle
 * @return Local<Object> info
 */
Handle<Object> IO::getDeviceInfo(Local<Value> handle) {

	HandleScope scope;
	Local<Object> info = Object::New();

	char procPath[24], devicePath[24];

	sprintf(procPath, "/proc/self/fd/\%d", handle->Int32Value());

	ssize_t len = readlink(procPath, devicePath, sizeof(devicePath));
	devicePath[len] = '\0';

    info->Set(String::NewSymbol("devicePath"), String::New(devicePath));
	// to be continued ...

	return scope.Close(info);
}