#ifndef opentracing_h
#define opentracing_h

#include <opentracing/span.h>

// void createTestTrace();
void initializeTracing();
std::unique_ptr<opentracing::Span> createSpanFromClad(const std::string& operationName, const std::string& spanContext);

#endif /* opentracing_h */
