# igprof-book

`igprof-book` is a web GUI to browse one or multiple igprof profile result.

It works by reading the sqlite dumps produced by `igprof-analyse` from a given
kyotocabinet store where they were previously accumulated via the
`igprof-populator` command.

## Building and installing

In order to create `igprof-book` you need to download [protovis][] and
[jquery][] and [ba-hashchange][] somewhere:

  WORKDIR=/tmp/igprof-book
  mkdir -p $WORKDIR
  curl -o$WORKDIR/jquery-1.4.4.min.js http://code.jquery.com/jquery-1.4.4.min.js
  curl -o$WORKDIR/jquery.ba-hashchange.min.js https://raw.github.com/cowboy/jquery-hashchange/master/jquery.ba-hashchange.min.js
  curl -o$WORKDIR/protovis-3.2.zip http://protovis-js.googlecode.com/files/protovis-3.2.zip
  pushd $WORKDIR
    unzip $WORKDIR/protovis-3.2.zip
  popd  $WORKDIR

and then run the following command:

  cmake . -DUNWIND_INCLUDE_DIR= -DUNWIND_LIBRARY= \
          -DPROTOVIS_ROOT=$WORKDIR/protovis-3.2 \
          -DJQUERY_ROOT=$WORKDIR \
          -DJQUERY_BA_HASHCHANGE_ROOT=$WORKDIR
  make igprof-book

which should create a `igprof-book` file in your current directory.

## Running

You can then run `igprof-book` either standalone, by executing it on the
command line, or on in a cgi enabled server. Notice you'll need
[kyotocabinet][] and it's python bindings in order to have it working.

[jquery]: http://www.jquery.com
[ba-hashchange]: http://benalman.com/code/projects/jquery-hashchange/docs/files/jquery-ba-hashchange-js.html
[protovis]: http://vis.stanford.edu/protovis/
[kyotocabinet]: http://fallabs.com/kyotocabinet/
