const pegjs = require('pegjs');
const fs = require('fs');
const path = require('path');

const basefolder = path.join(__dirname, "../clad/src");

// This creates our preprocessor
var preprocessor = pegjs.buildParser(fs.readFileSync(path.join(__dirname, "preprocessor.pegjs"), 'utf-8'));

preprocessor.load = function (fn) {
	var def = fs.readFileSync(path.join(basefolder, fn), 'utf-8');
	return this.parse(def);
}

// This creates our parser
var parser = pegjs.buildParser(fs.readFileSync(path.join(__dirname, "clad.pegjs"), 'utf-8'), { cache: true });

function process(fn) {
	var def = preprocessor.load(fn)
	var ast = parser.parse(def)


	console.log(JSON.stringify(ast, null, 4));

	return ast;
}

module.exports = process;
