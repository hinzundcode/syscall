#include <node.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <stdint.h>
#include <sstream>

namespace syscall_ {

using v8::FunctionCallbackInfo;
using v8::Isolate;
using v8::Local;
using v8::Object;
using v8::Value;
using v8::String;
using v8::Exception;
using v8::Function;
using v8::TypedArray;
//using v8::internal::BigInt;

Local<String> v8_str(const char* x) {
	return String::NewFromUtf8(Isolate::GetCurrent(), x, v8::NewStringType::kNormal).ToLocalChecked();
}

void v8_throw_type(const char* x) {
	Isolate::GetCurrent()->ThrowException(Exception::TypeError(v8_str(x)));
}

void v8_throw(const char* x) {
	Isolate::GetCurrent()->ThrowException(v8_str(x));
}

// TODO: temporary solution until I can use BigInt directly in cpp
Local<Value> ToBigInt(Isolate* isolate, long n) {
	Local<Object> global = isolate->GetCurrentContext()->Global();
	Local<Value> bigint = global->Get(isolate->GetCurrentContext(), v8_str("BigInt")).ToLocalChecked();
	
	std::ostringstream os;
	os << n;
	
	Local<Value> args[1] = {
		v8_str(os.str().c_str())
	};
	return Local<Function>::Cast(bigint)->Call(global, 1, args);
}

// TODO: temporary solution until I can use BigInt directly in cpp
long FromBigInt(Isolate* isolate, Local<Value> bigint) {
	String::Utf8Value utf8(isolate, bigint);
	
	long value;
	std::istringstream iss(*utf8);
	iss >> value;
	return value;
}

// TODO: temporary solution until I can use BigInt directly in cpp
bool IsBigInt(Isolate* isolate, Local<Value> value) {
	Local<String> type = value->TypeOf(isolate);
	String::Utf8Value utf8(isolate, type);
	return std::string(*utf8) == "bigint";
	//return value->IsBigInt();
}

long ConvertArg(const FunctionCallbackInfo<Value>& args, int index) {
	Isolate* isolate = args.GetIsolate();
	
	auto arg = args[index];
	
	if (arg->IsInt32()) {
		return arg->Int32Value();
	}
	
	if (arg->IsTypedArray()) {
		Local<TypedArray> ta = arg.As<TypedArray>();
		auto contents = ta->Buffer()->GetContents();
		return (long)contents.Data() + ta->ByteOffset();
	}
	
	if (IsBigInt(isolate, arg)) {
		return FromBigInt(args.GetIsolate(), arg);
	}
	
	v8_throw_type("Bad argument");
	return 0;
}

void Syscall(const FunctionCallbackInfo<Value>& args) {
	Isolate* isolate = args.GetIsolate();
	
	if (args.Length() < 1 || args.Length() > 7) {
		v8_throw_type("syscall needs 1 to 7 arguments");
		return;
	}
	
	uint32_t number = args[0]->Uint32Value();
	
	long result = 0;
	
	switch (args.Length()) {
		case 1:
			result = syscall(number);
			break;
		case 2:
			result = syscall(number, ConvertArg(args, 1));
			break;
		case 3:
			result = syscall(number, ConvertArg(args, 1), ConvertArg(args, 2));
			break;
		case 4:
			result = syscall(number, ConvertArg(args, 1), ConvertArg(args, 2), ConvertArg(args, 3));
			break;
		case 5:
			result = syscall(number, ConvertArg(args, 1), ConvertArg(args, 2), ConvertArg(args, 3), ConvertArg(args, 4));
			break;
		case 6:
			result = syscall(number, ConvertArg(args, 1), ConvertArg(args, 2), ConvertArg(args, 3), ConvertArg(args, 4), ConvertArg(args, 5));
			break;
		case 7:
			result = syscall(number, ConvertArg(args, 1), ConvertArg(args, 2), ConvertArg(args, 3), ConvertArg(args, 4), ConvertArg(args, 5), ConvertArg(args, 6));
			break;
	}
	
	//args.GetReturnValue().Set(BigInt.FromUint64(isolate, result));
	args.GetReturnValue().Set(ToBigInt(isolate, result));
}

void GetPointer(const FunctionCallbackInfo<Value>& args) {
	Isolate* isolate = args.GetIsolate();
	
	if (args.Length() < 1 || !args[0]->IsTypedArray()) {
		v8_throw_type("Bad arguments");
		return;
	}
	
	Local<TypedArray> ta = args[0].As<TypedArray>();
	auto contents = ta->Buffer()->GetContents();
	long address = (long)contents.Data() + ta->ByteOffset();
	args.GetReturnValue().Set(ToBigInt(isolate, address));
}

void Validate(const FunctionCallbackInfo<Value>& args) {
	Isolate* isolate = args.GetIsolate();
	
	if (sizeof(long) != 8)
		v8_throw("syscall only works on systems where sizeof(long) == 8");
	
	Local<Object> global = isolate->GetCurrentContext()->Global();
	bool bigintSupport = global->Has(isolate->GetCurrentContext(), v8_str("BigInt")).ToChecked();
	if (!bigintSupport)
		v8_throw("you have to enable BigInt support (--harmony-bigint)");
}

void init(Local<Object> exports) {
	Isolate* isolate = Isolate::GetCurrent();
	
	NODE_SET_METHOD(exports, "syscall", Syscall);
	NODE_SET_METHOD(exports, "ref", GetPointer);
	NODE_SET_METHOD(exports, "_validate", Validate);
}

NODE_MODULE(NODE_GYP_MODULE_NAME, init);

}
