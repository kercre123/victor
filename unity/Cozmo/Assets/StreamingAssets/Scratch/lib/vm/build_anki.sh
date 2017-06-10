# *** ANKI CHANGE ***
#
# Build script to compress vm files. 
#
# Note: caching the node_modules directory greatly speeds up this step
#       with npm update it should ensure that it's equivalent to a fresh install
ENABLE_NODE_MODULE_CACHING=0

if [ $ENABLE_NODE_MODULE_CACHING != 0 ]
then
    echo moving our cached node_modules back here: saves npm installing it each time
    mv ~/bak_node_modules ./node_modules
    echo npm update: bring cache up to date with a fresh install:
    npm update
else
    echo node_modules caching disabled.
fi

echo npm install:
npm install

echo npm run build:
npm run build

if [ $ENABLE_NODE_MODULE_CACHING != 0 ]
then
    echo caching node_modules back to home directory out of the way of git
    mv node_modules ~/bak_node_modules
else
    echo deleting node_modules
    rm -rf node_modules
fi

echo removing node_modules.meta incase Unity made one
rm -f node_modules.meta

echo removing lib/vm/dist/node
rm -rf dist/node

echo removing node.meta incase Unity made one
rm -f dist/node.meta

echo removing lib/vm/playground
rm -rf playground
