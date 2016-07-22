const pegjs = require('pegjs');
const fs = require('fs');
const path = require('path');

const basefolder = path.join(__dirname, "../clad/src");

// These are our processor definitions
var preprocessor = pegjs.buildParser(fs.readFileSync(path.join(__dirname, "preprocessor.pegjs"), 'utf-8'));
var parser = pegjs.buildParser(fs.readFileSync(path.join(__dirname, "clad.pegjs"), 'utf-8'), { cache: true });

preprocessor.load = function (fn) {
	var def = fs.readFileSync(path.join(basefolder, fn), 'utf-8');
	return this.parse(def);
}

function structure(ast, space) {
	console.log(ast);
	return () => new Buffer(0);
}

function enumerate(ast, space) {
	ast.members.reduce((acc, block) => {
		if (block.value != undefined) {
			acc = block.value;
		}

		space[block.name.name] = acc;

		return acc + 1;
	}, 0);

	return ast.base;
}

function namespace(ast, space) {
	return ast.members.reduce((space, block) => {
		switch (block.type) {
			case 'Namespace':
				space[block.name.name] = namespace(block, space[block.name.name]);
				break ;
			case 'Enum':
				space[block.name.name] = enumerate(block, space);
				break ;
			case 'Structure':
				space[block.name.name] = structure(block, space);
				break ;
			case 'Union':
				// TODO
				break ;
			default:
				//throw new Error(`Cannot handle block type ${block.type} as a member of a namespace`);
				console.log(`Cannot handle block type ${block.type} as a member of a namespace`);
				break ;
		}

		return space;
	}, space || {});
}

// This creates our parser
function process(fn) {
	var def = preprocessor.load(fn)
	var ast = parser.parse(def)

	// Generate a global namespace based on our AST
	return namespace(ast);
}

module.exports = process;
