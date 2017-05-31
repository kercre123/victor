# After modifying files in Scratch/lib/blocks, run this file
# to generate the compressed js files. - msintov, 5/18/17

rm -rf ../closure-library

ln -s $(npm root)/google-closure-library ../closure-library

echo npm install
npm install

echo removing node_modules.meta incase Unity made one
rm -f node_modules.meta

echo removing npm-*log*.meta incase Unity made one
rm -f npm-debug.log.meta

echo removing dist.meta incase Unity made one
rm -f dist.meta

echo removing closure-library symlink
rm ../closure-library

echo reverting gh-pages
git checkout -- gh-pages
git clean -fd gh-pages

echo reverting i18n
git checkout -- i18n
git clean -fd i18n

echo reverting node_modules
rm -rf node_modules

echo reverting msg
git checkout -- msg
git clean -fd msg

