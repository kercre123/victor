## Authorization and Authentication

This package implements plug-in support for identifying RPC callers
(authentication) and determining if they're allowed to make the call
they're attempting to make (authorization).

Authentication & authorization both take the form of _interceptors_
that can be inserted into the call chain of a gRPC service.

There is a
[presentation on authn/authz](docs/Authorization.html). Download the
HTML file and open in browser to play it.

### Authentication

Authentication support is provided for 2 forms of authentication --
Anki's traditional appkey/session authentication (mediated by the
[Accounts Service](https://github.com/anki/sai-go-accounts)) and
JWT-based authentication (mediated by the
[Token Service](https://github.com/anki/sai-token-service)). The
authenticators decorate the context with credentials, passing them
down to the authorizer component for enforcing access controls.

* [accounts_authenticator.go](accounts_authenticator.go) implements
  the authenticator for appkey/session auth.
* [token_authenticator.go](token_authenticator.go) implements the
  authenticator for JWT-based auth.

### Authorization

* [grpc_authorizer.go](grpc_authorizer.go) implements an interceptor
  for authorizing gRPC calls.
* [authorization_spec.go](authorization_spec.go) implements the actual
  authorization logic, in a manner independent of gRPC vs. REST
  invocation (in theory).

### Additional Plumbing

The accounts authenticator delegates most of its work to a
_validator_, which is implemented by
[accounts_validator.go](accounts_validator.go), which contacts the
Accounts service to validate appkeys and
sessions. [caching_validator.go](caching_validator.go) implements a
caching wrapper around the accounts validator, with the actual cache
provided by [accounts_cache.go](accounts_cache.go).
