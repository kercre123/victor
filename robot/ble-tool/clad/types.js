class Signed8 {
	static getSize(data) { return 1; }
	static _serialize(data, buffer, offset) { buffer.writeInt8(value || 0, offset) }
	static _deserialize(buffer, offset) { return buffer.readInt8(offset); }
}

class Signed16 {
	static getSize(data) { return 2; }
	static _serialize(data, buffer, offset) { buffer.writeInt16LE(value || 0, offset) }
	static _deserialize(buffer, offset) { return buffer.readInt16LE(offset); }
}

class Signed32 {
	static getSize(data) { return 4; }
	static _serialize(data, buffer, offset) { buffer.writeInt32LE(value || 0, offset) }
	static _deserialize(buffer, offset) { return buffer.readInt32LE(offset); }
}

class Unsigned8 {
	static getSize(data) { return 1; }
	static _serialize(data, buffer, offset) { buffer.writeUInt8(value || 0, offset) }
	static _deserialize(buffer, offset) { return buffer.readUInt8(offset); }
}

class Unsigned16 {
	static getSize(data) { return 2; }
	static _serialize(data, buffer, offset) { buffer.writeUInt16LE(value || 0, offset) }
	static _deserialize(buffer, offset) { return buffer.readUInt16LE(offset); }
}

class Unsigned32 {
	static getSize(data) { return 4; }
	static _serialize(data, buffer, offset) { buffer.writeUInt32LE(value || 0, offset) }
	static _deserialize(buffer, offset) { return buffer.readUInt32LE(offset); }
}

class Float32 {
	static getSize(data) { return 4; }
	static _serialize(data, buffer, offset) { buffer.writeUInt32LE(value || 0, offset) }
	static _deserialize(buffer, offset) { return buffer.readUInt32LE(offset); }
}
class Float64 {
	static getSize(data) { return 8; }
	static _serialize(data, buffer, offset) { buffer.writeUInt32LE(value || 0, offset) }
	static _deserialize(buffer, offset) { return buffer.readUInt32LE(offset); }
}

class Bool {
	static getSize(data) { return 1; }
	static _serialize(data, buffer, offset) { buffer.writeUInt8(value ? 1 : 0, offset) }
	static _deserialize(buffer, offset) { return !!buffer.readUInt8(offset); }
}

class Void {
	static getSize(data) { return 0; }
	static _serialize(data, buffer, offset) { }
	static _deserialize(buffer, offset) { return null; }
}

class StringType {
	constructor (base) {
		this.index = (base.size && base.size.index) || Unsigned8;
		
		// NOTE: I DO NOT ENFORCE MAX STRING LENGTH, IS THAT BEHAVIOR SUPPORTED?!
	}

	getSize (data) {
		var len = this.index.getSize();
		return len + Buffer.from(data, "utf-8").length;
	}

	_serialize (data, buffer, offset) {
		var string = Buffer.from(data, "utf-8");

		this.index._serialize(string.length, buffer, offset);
		offset += this.index.getSize(string.length);

		string.copy(buffer, offset);
		offset += string.length;
	}

	_deserialize (buffer, offset) {
		var size;

		size = this.index._deserialize(buffer, offset);
		offset += this.index.getSize(data.length);

		return buffer.toString('utf-8', offset, offset+size);
	}
};

class ArrayType {
	constructor (base, size, index) {
		this.base = base;
		this.size = size;
		this.index = index;
	}

	getSize(data) {
		if (this.index) {
			if (this.size && data.length > this.size) {
				throw new Error("Source array is too large");
			}

			return data.reduce((acc, value) => acc + this.base.getSize(value), this.index.getSize(data.length));
		} else {
			if (this.size != data.length) {
				throw new Error("Source array is an incorrect size");
			}

			return data.reduce((acc, value) => acc + this.base.getSize(value), 0);
		}
	}

	static _serialize(data, buffer, offset) {
		// Size / range checking done by getSize

		if (this.index) {
			this.index._serialize(data.length, buffer, offset);
			offset += this.index.getSize(data.length);
		}


		data.forEach((value) => {
			this.index._serialize(value, buffer, offset);
			offset += this.base.getSize(value);
		});
	}

	static _deserialize(buffer, offset) {
		var size;

		if (this.index) {
			size = this.index._deserialize(buffer, offset);
			offset += this.index.getSize(data.length);
		} else {
			size = this.size;
		}

		var output = [];
		while (--size > 0) {
			var val = this.base._deserialize(buffer, offset);
			offset += this.index.getSize(val);
			output.push(val);
		}
	}
};

module.exports = {
	'SignedType': function (base) {
		switch (base.size) {
		case 8: return Signed8;
		case 16: return Signed16;
		case 32: return Signed32;
		}

		throw new Error(`Cannot create signed integer of size ${base.size}`)
	},
	'UnsignedType': function (base) {
		switch (base.size) {
		case 8: return Unsigned8;
		case 16: return Unsigned16;
		case 32: return Unsigned32;
		}

		throw new Error(`Cannot create unsigned integer of size ${base.size}`)
	},
	'FloatType': function (base) {
		switch (base.size) {
		case 32: return Float32;
		case 64: return Float64;
		}

		throw new Error(`Cannot create float of size ${base.size}`)
	},
	'BoolType': function () {
		return Bool;
	},
	'VoidType': function () {
		return Void;
	},
	'StringType': StringType,
	'ArrayType': ArrayType
};
