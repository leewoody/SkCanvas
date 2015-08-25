#include "JSCore.h"
#include "SkTypes.h"
#include "string"


// Extracts a C string from a V8 Utf8Value.
const char* ToCString(const v8::String::Utf8Value& value)
{
	return *value ? *value : "<string conversion failed>";
}

v8::Local<v8::Context> CreateShellContext(v8::Isolate* isolate) 
{
	v8::Local<v8::ObjectTemplate> global = v8::ObjectTemplate::New(isolate);
	global->Set(v8::String::NewFromUtf8(isolate, "print", v8::NewStringType::kNormal).ToLocalChecked(), v8::FunctionTemplate::New(isolate, Print));
	global->Set(v8::String::NewFromUtf8(isolate, "read", v8::NewStringType::kNormal).ToLocalChecked(), v8::FunctionTemplate::New(isolate, Read));
	global->Set(v8::String::NewFromUtf8(isolate, "load", v8::NewStringType::kNormal).ToLocalChecked(), v8::FunctionTemplate::New(isolate, Load));
	global->Set(v8::String::NewFromUtf8(isolate, "quit", v8::NewStringType::kNormal).ToLocalChecked(), v8::FunctionTemplate::New(isolate, Quit));
	global->Set(v8::String::NewFromUtf8(isolate, "version", v8::NewStringType::kNormal).ToLocalChecked(), v8::FunctionTemplate::New(isolate, Version));
	global->Set(v8::String::NewFromUtf8(isolate, "require", v8::NewStringType::kNormal).ToLocalChecked(), v8::FunctionTemplate::New(isolate, V8_Require));
	return v8::Context::New(isolate, NULL, global);
}


// The callback that is invoked by v8 whenever the JavaScript 'print'
// function is called.  Prints its arguments on stdout separated by
// spaces and ending with a newline.
void Print(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	bool first = true;
	for (int i = 0; i < args.Length(); i++) {
		v8::HandleScope handle_scope(args.GetIsolate());
		if (first) {
			first = false;
		}
		else {
			printf(" ");
		}
		v8::String::Utf8Value str(args[i]);
		const char* cstr = ToCString(str);
		printf("%s", cstr);
	}
	printf("\n");
	fflush(stdout);
}


// The callback that is invoked by v8 whenever the JavaScript 'read'
// function is called.  This function loads the content of the file named in
// the argument into a JavaScript string.
void Read(const v8::FunctionCallbackInfo<v8::Value>& args) {
	if (args.Length() != 1) {
		args.GetIsolate()->ThrowException(
			v8::String::NewFromUtf8(args.GetIsolate(), "Bad parameters",
			v8::NewStringType::kNormal).ToLocalChecked());
		return;
	}
	v8::String::Utf8Value file(args[0]);
	if (*file == NULL) {
		args.GetIsolate()->ThrowException(
			v8::String::NewFromUtf8(args.GetIsolate(), "Error loading file",
			v8::NewStringType::kNormal).ToLocalChecked());
		return;
	}
	v8::Local<v8::String> source;
	if (!ReadFile(args.GetIsolate(), *file).ToLocal(&source)) {
		args.GetIsolate()->ThrowException(
			v8::String::NewFromUtf8(args.GetIsolate(), "Error loading file",
			v8::NewStringType::kNormal).ToLocalChecked());
		return;
	}
	args.GetReturnValue().Set(source);
}


// The callback that is invoked by v8 whenever the JavaScript 'load'
// function is called.  Loads, compiles and executes its argument
// JavaScript file.
void Load(const v8::FunctionCallbackInfo<v8::Value>& args) {
	for (int i = 0; i < args.Length(); i++) {
		v8::HandleScope handle_scope(args.GetIsolate());
		v8::String::Utf8Value file(args[i]);
		if (*file == NULL) {
			args.GetIsolate()->ThrowException(
				v8::String::NewFromUtf8(args.GetIsolate(), "Error loading file",
				v8::NewStringType::kNormal).ToLocalChecked());
			return;
		}
		v8::Local<v8::String> source;
		if (!ReadFile(args.GetIsolate(), *file).ToLocal(&source)) {
			args.GetIsolate()->ThrowException(
				v8::String::NewFromUtf8(args.GetIsolate(), "Error loading file",
				v8::NewStringType::kNormal).ToLocalChecked());
			return;
		}
		if (!ExecuteString(args.GetIsolate(), source, args[i], false, false)) {
			args.GetIsolate()->ThrowException(
				v8::String::NewFromUtf8(args.GetIsolate(), "Error executing file",
				v8::NewStringType::kNormal).ToLocalChecked());
			return;
		}
	}
}


// The callback that is invoked by v8 whenever the JavaScript 'quit'
// function is called.  Quits.
void Quit(const v8::FunctionCallbackInfo<v8::Value>& args) {
	// If not arguments are given args[0] will yield undefined which
	// converts to the integer value 0.
	int exit_code =
		args[0]->Int32Value(args.GetIsolate()->GetCurrentContext()).FromMaybe(0);
	fflush(stdout);
	fflush(stderr);
	exit(exit_code);
}


void Version(const v8::FunctionCallbackInfo<v8::Value>& args) {
	args.GetReturnValue().Set(
		v8::String::NewFromUtf8(args.GetIsolate(), v8::V8::GetVersion(),
		v8::NewStringType::kNormal).ToLocalChecked());
}

void V8_Require(const v8::FunctionCallbackInfo<v8::Value> &args)
{
	BEGIN_SCOPE_WHITH_ARGS(1);
	v8::String::Utf8Value str(args[0]);
	const char* cstr = ToCString(str);
	std::string  strfile = "C:/tmp/egret-game/";
	strfile += cstr;
	RunJavaScript(strfile.c_str());
	return;
}


// Reads a file into a v8 string.
v8::MaybeLocal<v8::String> ReadFile(v8::Isolate* isolate, const char* name) {
	FILE* file = fopen(name, "rb");
	if (file == NULL) return v8::MaybeLocal<v8::String>();

	fseek(file, 0, SEEK_END);
	size_t size = ftell(file);
	rewind(file);

	char* chars = new char[size + 1];
	chars[size] = '\0';
	for (size_t i = 0; i < size;) {
		i += fread(&chars[i], 1, size - i, file);
		if (ferror(file)) {
			fclose(file);
			return v8::MaybeLocal<v8::String>();
		}
	}
	fclose(file);
	v8::MaybeLocal<v8::String> result = v8::String::NewFromUtf8(
		isolate, chars, v8::NewStringType::kNormal, static_cast<int>(size));
	delete[] chars;
	return result;
}

// Executes a string within the current v8 context.
bool ExecuteString(v8::Isolate* isolate, v8::Local<v8::String> source,
	v8::Local<v8::Value> name, bool print_result,
	bool report_exceptions) 
{
	v8::HandleScope handle_scope(isolate);
	v8::TryCatch try_catch(isolate);
	v8::ScriptOrigin origin(name);
	v8::Local<v8::Context> context(isolate->GetCurrentContext());
	v8::Local<v8::Script> script;
	if (!v8::Script::Compile(context, source, &origin).ToLocal(&script)) {
		// Print errors that happened during compilation.
		if (report_exceptions)
			ReportException(isolate, &try_catch);
		return false;
	}
	else {
		v8::Local<v8::Value> result;
		if (!script->Run(context).ToLocal(&result)) {
			assert(try_catch.HasCaught());
			// Print errors that happened during execution.
			if (report_exceptions)
				ReportException(isolate, &try_catch);
			return false;
		}
		else {
			assert(!try_catch.HasCaught());
			if (print_result && !result->IsUndefined()) {
				// If all went well and the result wasn't undefined then print
				// the returned value.
				v8::String::Utf8Value str(result);
				const char* cstr = ToCString(str);
				printf("%s\n", cstr);
			}
			return true;
		}
	}
}

bool RunJavaScript( const char *fileName )
{
	v8::Isolate * isolate = v8::Isolate::GetCurrent();
	Local<Context> context = isolate->GetCurrentContext();
	Context::Scope context_scope(context);
	v8::Local<v8::String> file_name = v8::String::NewFromUtf8(isolate, fileName, v8::NewStringType::kNormal).ToLocalChecked();
	v8::Local<v8::String> source;
	if (!ReadFile(isolate, fileName).ToLocal(&source))
	{
		fprintf(stderr, "Error reading '%s'\n", fileName);
		return false;
	}
	if (!ExecuteString(isolate, source, file_name, true, true))
		return false;
	return true;
}


void ReportException(v8::Isolate* isolate, v8::TryCatch* try_catch) {
	v8::HandleScope handle_scope(isolate);
	v8::String::Utf8Value exception(try_catch->Exception());
	const char* exception_string = ToCString(exception);
	v8::Local<v8::Message> message = try_catch->Message();
	if (message.IsEmpty()) {
		// V8 didn't provide any extra information about this error; just
		// print the exception.
		fprintf(stderr, "%s\n", exception_string);
	}
	else {
		// Print (filename):(line number): (message).
		v8::String::Utf8Value filename(message->GetScriptOrigin().ResourceName());
		v8::Local<v8::Context> context(isolate->GetCurrentContext());
		const char* filename_string = ToCString(filename);
		int linenum = message->GetLineNumber(context).FromJust();
		fprintf(stderr, "%s:%i: %s\n", filename_string, linenum, exception_string);
		// Print line of source code.
		v8::String::Utf8Value sourceline(
			message->GetSourceLine(context).ToLocalChecked());
		const char* sourceline_string = ToCString(sourceline);
		fprintf(stderr, "%s\n", sourceline_string);
		// Print wavy underline (GetUnderline is deprecated).
		int start = message->GetStartColumn(context).FromJust();
		for (int i = 0; i < start; i++) {
			fprintf(stderr, " ");
		}
		int end = message->GetEndColumn(context).FromJust();
		for (int i = start; i < end; i++) {
			fprintf(stderr, "^");
		}
		fprintf(stderr, "\n");
		v8::String::Utf8Value stack_trace(
			try_catch->StackTrace(context).ToLocalChecked());
		if (stack_trace.length() > 0) {
			const char* stack_trace_string = ToCString(stack_trace);
			fprintf(stderr, "%s\n", stack_trace_string);
		}
	}
}

void ReportException(TryCatch& try_catch)
{
	HandleScope handle_scope(v8::Isolate::GetCurrent());
	String::Utf8Value exception(try_catch.Exception());
	const char* exception_string = ToCString(exception);
	Handle<Message> message = try_catch.Message();

	if (!message.IsEmpty()) {
		String::Utf8Value filename(message->GetScriptResourceName());
		const char* filename_string = ToCString(filename);
		int linenum = message->GetLineNumber();
		String::Utf8Value sourceline(message->GetSourceLine());
		SkDebugf("%s:%d : %s", filename_string, linenum, exception_string);
	}

}

