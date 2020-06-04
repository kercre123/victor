## Robot Session Certificate Store

Dirt-simple backing store for saving robot session certificates. Robot
session certificates are uploaded in the `AssociatePrimaryUser` gRPC
call, and stored to S3.

* [S3 store](s3_store.go)
