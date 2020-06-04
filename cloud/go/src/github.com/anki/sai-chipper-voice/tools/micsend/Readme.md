Here's a sample command line for building a tool like this against
the Chipper client, which requires the opus library:

```
CGO_ENABLED=1 CGO_LDFLAGS="-L$HOME/go/src/github.com/anki/opus-go/libopus/mac/lib -lopus" \
  CGO_CPPFLAGS="-I $HOME/go/src/github.com/anki/opus-go/libopus/mac/include" go build
```

  (paths may vary depending on where your GOPATH lives)
