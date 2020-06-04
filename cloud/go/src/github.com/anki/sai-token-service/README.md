# Token Management Service

For authentication of Victor users & robots, etc...

## Package Overview

* [proto/tokenpb](proto/tokenpb) Protobuf definitions for the service.
* [server/token](server/token) Implementation of the gRPC interface ([server.go](server/token/server.go)) and configuration & lifecycle management ([service.go](server/token/service.go))
* [client/token](client/token) Token Service client package for
  consumers of JWT authentication to use. Provides a gRPC client + the
  necessary plumbing for services to validate JWTs and handle token
  revocation notifications.
* [db](db) Database backend for storing tokens in DynamoDB.
* [certstore](certstore) Robot Session Certificate store -- stores
  certificates uploaded by robots for Vector app access to the robot.
* [stsgen](stsgen) STS token generation - Generates STS tokens used by the
  robot to upload logs to S3.

#### Testing Support

* [integration](integration) Docker-based test stack for running the
  token service locally. Includes local implementations of all AWS
  dependencies and Accounts.
