#include <opentracing/tracer.h>
#include <opentracing/noop.h>

using namespace opentracing;

// namespace Anki {
// namespace Switchboard {

std::shared_ptr<Tracer> tracer;

void initializeTracing() {
  #ifdef USE_LIGHTSTEP_TRACER
  LightStepTracerOptions options;
  options.access_token = "bd61f2a69616b03c68839edbf68d19a0";
  tracer = MakeLightStepTracer(options);
  #else
  tracer = MakeNoopTracer();
  #endif
}

// See: https://github.com/opentracing/opentracing-cpp/blob/master/example/tutorial/tutorial-example.cpp

struct MyCarrierWriter : TextMapWriter {
  explicit MyCarrierWriter(std::unordered_map<std::string, std::string>& data_)
      : data{data_} {}

  expected<void> Set(string_view key, string_view value) const override {
    expected<void> result;

    auto was_successful = data.emplace(key, value);
    if (was_successful.second) {
      return result;
    } else {
      return make_unexpected(
          std::make_error_code(std::errc::not_supported));
    }
  }

  std::unordered_map<std::string, std::string>& data;
};

void inject(std::unique_ptr<Span> span) {
  std::unordered_map<std::string, std::string> data;
  MyCarrierWriter carrier{data};
  auto was_successful = tracer->Inject(span->context(), carrier);
  if (!was_successful) {
    // std::cerr << was_successful.error().message() << "\n";
  }
}

std::unique_ptr<Span> createSpanFromClad(const std::string& operationName, const std::string& spanContext) {
  auto span = tracer->StartSpan(operationName);
  return span;
}

void createTestTrace() {
  Tracer::InitGlobal(tracer);

  auto span = tracer->StartSpan("first_cpp_trace");
  if (!span) {
    // Error creating span.
    // ...
  }
  span->Finish();
}

// }
// }
