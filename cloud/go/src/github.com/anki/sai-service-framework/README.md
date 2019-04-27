# SAI Service Framework

A shared library providing a framework for building long running
service processes with all the bells and whistles that a server
process needs to be properly monitorable.

Functionality is focused on several areas:

* Lifecycle (i.e. start/stop) management
* Configuration and command-line handling
* Composition of re-usable components with minimal boilerplate

And the library also provides standard components for building
services:

* [Logging](svc/component_logger.go) with Anki's logging package
* Capturing go [runtime profiling](svc/component_profiler.go) information
* Periodic [status checks](svc/component_status_monitor.go)
* A [profile server](svc/component_profile_server.go) which exposes
  metrics, status checks, build version, and go's runtime debug
  facilities on an HTTP port.
* Service components for building [gRPC services](grpc/svc/grpc_service.go)
    * Including a standard suite of chained interceptors for
      [logging](grpc/interceptors/accesslog.go),
      [metrics](grpc/interceptors/metrics.go),
      [authentication](grpc/interceptors/auth.go), conversion of
      [panics to gRPC errors](grpc/interceptors/recovery.go), and
      [request tracing](grpc/interceptors/requestid.go).
    * Standard configuration for initializing TLS

This library is built in conjunction with the
[sai-service-template-rest](https://github.com/anki/sai-service-template-rest)
and
[sai-service-template-grpc](https://github.com/anki/sai-service-template-grpc)
projects which provide a template for using the library.

## Packages

* [svc](svc): Provides the bulk of the framework, as well as the
  core standardized components and command-line handling utilities.
* [grpc/svc](grpc/svc): Provides the primary component for managing a
  gRPC service implementation and TLS configuration.
* [grpc/interceptors](grpc/interceptors): Provides the standard suite
  of gRPC interceptors and utilities for defining new ones.
* [auth](auth) provides authentication and authorization support
* [rest](rest) provides service framework-based configuration for REST
  API clients based on the
  [sai-go-util/http/apiclient](https://github.com/anki/sai-go-util/tree/master/http/apiclient)
  package.
* [rest/svc](rest/svc): Provides a service component to hook Anki's
  REST API framework into the service framework. Not ready for
  production, as the REST API framework assumes control of a lot of
  things.
