## Token Store

This package defines the backing store for tokens. The
[`TokenStore`](store.go) interface defines the actions for backing
stores. There is an [in-memory](memory_store.go) implementation for
unit-testing, and the [Dynamo-backed implementation](dynamo_store.go) for
integration and deployment.
