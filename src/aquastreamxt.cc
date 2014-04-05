#include <node.h>
#include <v8.h>
#include <sys/stat.h>
#include "aquastreamxt.h"
#include "io.h"

using namespace v8;

Aquastream::Aquastream() {};
Aquastream::~Aquastream() {};

void Aquastream::Init(Handle<Object> target) {

	// Prepare constructor template
	Local<FunctionTemplate> tpl = FunctionTemplate::New(New);
	tpl->SetClassName(String::NewSymbol("Aquastream"));
	tpl->InstanceTemplate()->SetInternalFieldCount(3);

	// Prototype
	tpl->PrototypeTemplate()->Set(
		String::NewSymbol("getReport"),
		FunctionTemplate::New(GetReport)->GetFunction()
	);

	tpl->PrototypeTemplate()->Set(
		String::NewSymbol("setReport"),
		FunctionTemplate::New(SetReport)->GetFunction()
	);

	tpl->PrototypeTemplate()->Set(
        String::NewSymbol("getDeviceInfo"),
        FunctionTemplate::New(GetDeviceInfo)->GetFunction()
    );

	Persistent<Function> constructor = Persistent<Function>::New(tpl->GetFunction());
	target->Set(String::NewSymbol("Aquastream"), constructor);
};

Handle<Value> Aquastream::New(const Arguments& args) {

	HandleScope scope;

	Aquastream* aquastream	= new Aquastream();
	aquastream->vendorId 	= args[0]->NumberValue();
	aquastream->productId 	= args[1]->NumberValue();

	aquastream->handle 		= IO::openDevice(aquastream->vendorId, aquastream->productId);

	aquastream->Wrap(args.This());

	return args.This();

};

Handle<Value> Aquastream::GetReport(const Arguments& args) {

	HandleScope scope;

	if (args.Length() < 2) {
		ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
		return scope.Close(Undefined());
	}

	if (!args[0]->IsNumber()) {
		ThrowException(Exception::TypeError(String::New("Invalid Feature Report ID")));
		return scope.Close(Undefined());
	}

	Aquastream* aquastream 	= ObjectWrap::Unwrap<Aquastream>(args.This());
	int reportId 			= args[0]->NumberValue();

	// get settings
	Handle<Object> settings = IO::getSettings(Number::New(aquastream->handle), Number::New(reportId));
	Handle<Object> returnValue;

	switch(reportId) {
		case 4:
			returnValue = IO::getData(Number::New(aquastream->handle), Number::New(reportId), settings);
		break;
		case 6:
			returnValue = settings;
        break;
	}

	Local<Function> cb = Local<Function>::Cast(args[1]);
	const unsigned argc = 1;
	Local<Value> argv[argc] = { Local<Object>::New(returnValue) };
	cb->Call(Context::GetCurrent()->Global(), argc, argv);

	return scope.Close(Undefined());
};

Handle<Value> Aquastream::SetReport(const Arguments& args) {

	HandleScope scope;

	if (args.Length() < 2) {
		ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
		return scope.Close(Undefined());
	}

	if (!args[0]->IsNumber()) {
		ThrowException(Exception::TypeError(String::New("Invalid Feature Report ID")));
		return scope.Close(Undefined());
	}

	Aquastream* aquastream 		= ObjectWrap::Unwrap<Aquastream>(args.This());
	int reportId 				= args[0]->NumberValue();

	Handle<Value> returnValue;
	Handle<Object> data 		= args[1]->ToObject();

	switch(reportId) {
		case 6:
			returnValue = IO::setSettings(Number::New(aquastream->handle), Number::New(reportId), data);
		break;
		default:
			returnValue = Number::New(-1);
		break;
	}

	//return returnValue;
	return scope.Close(returnValue);
};


Handle<Value> Aquastream::GetDeviceInfo(const Arguments& args) {

	HandleScope scope;

	Aquastream* aquastream 	= ObjectWrap::Unwrap<Aquastream>(args.This());

	// get info
	Handle<Object> info = IO::getDeviceInfo(Number::New(aquastream->handle));

	Local<Function> cb = Local<Function>::Cast(args[0]);
	const unsigned argc = 1;
	Local<Value> argv[argc] = { Local<Object>::New(info) };
	cb->Call(Context::GetCurrent()->Global(), argc, argv);

	return scope.Close(Undefined());
};

void InitAll(Handle<Object> target) {
	Aquastream::Init(target);
};

NODE_MODULE(aquastreamxt_api, InitAll)