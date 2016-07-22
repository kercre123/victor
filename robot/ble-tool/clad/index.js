const pegjs = require('pegjs');
const fs = require('fs');
const path = require('path');
const types = require('./types');

const basefolder = path.join(__dirname, "../../clad/src");

// These are our processor definitions
var preprocessor = pegjs.buildParser(fs.readFileSync(path.join(__dirname, "preprocessor.pegjs"), 'utf-8'));
var parser = pegjs.buildParser(fs.readFileSync(path.join(__dirname, "clad.pegjs"), 'utf-8'), { cache: true });

function lookup(name, space) {
	return name.reduce((acc, part) => {
		if (acc[part] === undefined) {
			throw new Error(`Type ${name.join("::")} is undefined`);
		}

		return acc[part];
	}, space);
}

function structure(ast, space) {
	var struct = ast.members.reduce((acc, member) => {
		var type;
		
		if (member.base.type === 'ElementType') {
			type = lookup(member.base.name, space);

			// This is an enumeration
			if (typeof type !== 'function') {
				type = new types[type.type](type);
			}
		} else {
			type = new types[member.base.type](member.base);
		}

		if (member.array) {
			if (member.array.index) {
				index = new types[member.array.index.type](member.array.index);
			}

			type = new types.ArrayType(type, member.array.size, index);
		}

		acc.push( { type: type, field: member.name } );
		return acc;
	}, []);

	//console.log(struct);

	function structType(initializer, offset) {
		if (Buffer.isBuffer(initializer)) {
			return structType.fromBuffer(initalizer);
		}

		if (!initializer) {
			return null;
		}

		// TOOD: INITALIZE FROM STRUCTURE
	}

	structType.prototype.getSize = function() {

	}

	structType.prototype.serialize = function() {

	}

	structType.fromBuffer = function() {
	}

	return structType;
}

function union(ast, space) {
	var local = {};

	// This only creates a lookup table
	// No real union support in javascript

	ast.members.forEach(function (block) {
		if (block.base.array) {
			throw new Error("Cannot handle arrays in unions yet")
		}

		if (block.value !== undefined) {
			local[`Tag_${block.name}`] = block.value;
		}

		local[block.value] = lookup(block.base.name, space);
	});

	return local;
}

function enumerate(ast, space) {
	ast.members.reduce((acc, block) => {
		if (block.value != undefined) {
			acc = block.value;
		}

		space[block.name] = acc;

		return acc + 1;
	}, 0);

	return ast.base;
}

function namespace(ast, space) {
	return ast.members.reduce((space, block) => {
		switch (block.type) {
			case 'Namespace':
				space[block.name] = namespace(block, space[block.name] || Object.create(space));
				break ;
			case 'Enum':
				space[block.name] = enumerate(block, space);
				break ;
			case 'Structure':
				space[block.name] = structure(block, space);
				break ;
			case 'Union':
				space[block.name] = union(block, space);
				break ;
			default:
				throw new Error(`Cannot handle block type ${block.type} as a member of a namespace`);
				break ;
		}

		return space;
	}, space || {});
}

// Setup our preprocessor stuff
preprocessor.load = function (fn) {
	var def = fs.readFileSync(path.join(basefolder, fn), 'utf-8');
	return this.parse(def);
}

function process(fn) {
	var def = preprocessor.load(fn)
	var ast = parser.parse(def)

	// Generate a global namespace based on our AST
	return namespace(ast);
}

module.exports = process;
