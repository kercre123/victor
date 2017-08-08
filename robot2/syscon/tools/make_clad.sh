
cd syscon
../../tools/message-buffers/emitters/CPPLite_emitter.py  ./schema/messages.clad -r build -o build
cp ./schema/messages.clad ../clad/src/clad/spine/spine_protocol.clad
../../tools/message-buffers/emitters/C_emitter.py -C ../clad/src/ -I -o ../generated/clad/robot/ clad/spine/spine_protocol.clad
cd -
