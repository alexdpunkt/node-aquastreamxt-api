
#ifndef AQUASTREAMXT_H
#define AQUASTREAMXT_H

#include <node.h>

class Aquastream : public node::ObjectWrap {

	public:
		static void Init(v8::Handle<v8::Object> target);

	private:
		Aquastream();
		~Aquastream();

	static v8::Handle<v8::Value> New(const v8::Arguments& args);
	static v8::Handle<v8::Value> GetReport(const v8::Arguments& args);
	static v8::Handle<v8::Value> SetReport(const v8::Arguments& args);
	static v8::Handle<v8::Value> GetDeviceInfo(const v8::Arguments& args);

	int vendorId;
	int productId;
	int handle;

};

#endif