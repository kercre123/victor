### protoCppPlugin

An Anki plugin for the protobuf compiler `protoc`. This gives the generated C++ code some of the nice properties of CLAD, namely:

+ CLAD union tags are enum classes, so are safer and can be forward
  declared. CLAD also generates a separate \*Tags.h file containing only
  the union tags. Now, for every proto message containing a single
  OneOf, an enum class that matches the oneof case is added to a
  separate \*Tags.pb.h. Messages are given an accessor `GetTag()` to match
  CLAD.
+ Some CLAD message constructor definitions enumerate all fields, which
  ensures that all values are set when creating a message. Now, most
  proto messages have the same constructors. While protobuf allows for
  optional params, it's probably not the most common use case for app
  comms, and the default constructor and `set_param_name()` setters are
  still exposed.
+ The Tag of a CLAD union gets set during union construction based on
  the type passed to the constructor. This now also works in proto
  messages containing a single OneOf field.

### Building protoCppPlugin

These are temporary build instructions until the plugin is included in the regular build process. For now, the below commands were run to generate a binary stored in Artifactory, pulled in by DEPS.

#### Mac

See [make.mac.sh](/tools/protobuf/plugin/make.mac.sh)

#### Linux
1) Build protobuf (runtime and protoc compiler). Instructions [here](https://github.com/google/protobuf/blob/master/src/README.md).

2) 

```
INCLUDES="path/to/headers"
LIBS="path/to/libs" # the above installs it at /usr/local/lib/
SRCS=`find . -type f -iname "*.cpp"`
OUTPUT="../protocCppPlugin"
PROTOC_STATIC_LIB="libprotoc.a"
PROTOBUF_STATIC_LIB="libprotobuf.a"

g++                                   \
    --std=c++14                       \
    -I.  -I${INCLUDES}                \
    ${SRCS}                           \
    -o ${OUTPUT}                      \
    -L${LIBS} ${PROTOC_STATIC_LIB}    \
      ${PROTOBUF_STATIC_LIB} -lpthread
```

### Running
See [engine/CMakeLists.txt](/engine/CMakeLists.txt)

To run outside of the normal build process:

```
PROTOC="/path/to/protoc"
OUT="path/to/output/dir"
INPUT="path/to/input/file"
PLUGIN="protocCppPlugin"
PLUGIN_NAME="protoc-gen-protocCppPlugin"

${PROTOC} --plugin=${PLUGIN_NAME}=${PLUGIN} --proto_path=. --cpp_out=${OUT} --${PLUGIN}_out=${OUT} ${INPUT}
```