CladDefinition
	= _ list:CladStatement*
		{ return list; }

CladStatement
	= EnumElement
	/ NamespaceElement
	/ MessageElement
	/ UnionElement

// Top level members
NamespaceElement
	= "namespace" __ name:Identifier "{" _ members:CladStatement* "}" _
		{ return { type: 'Namespace', name: name, members: members }; }

EnumElement
	= "enum" __ name:Identifier "{" _ members:EnumMemberList? "}" _ 
		{ return { type: 'Enum', name: name, members: members }; }
	/ "enum" __ type:TypeDefinition name:Identifier "{" _ members:EnumMemberList? "}" _ 
		{ return { type: 'Enum', type: type, name: name, members: members }; }

MessageElement
	= ("message" / "structure") __ name:Identifier "{" _  members:StrucureMemberList? "}" _ 
		{ return { type: 'Structure', name: name, members: members }; }

UnionElement
	= "union" __ name:Identifier "{" _ members:StrucureMemberList? "}" _
		{ return { type: 'Union', name: name, members: members }; }
	/ "union" __ type:TypeDefinition name:Identifier "{" _ members:StrucureMemberList? "}" _
		{ return { type: 'Union', type: type, name: name, members: members }; }

// Array types
EnumMemberList
	= a:EnumMember b:("," _ b:EnumMember { return b; })* ","? _
		{ return [a].concat(b) }

StrucureMemberList
	= a:StrucureMember b:("," _ b:StrucureMember { return b; })* ","? _
		{ return [a].concat(b) }

// Member Elements
EnumMember
	= name:Identifier "=" _ literal:Literal
		{ return { type: "EnumMember", name: name, value: literal } }
	/ name:Identifier
		{ return { type: "EnumMember", name: name } }

StrucureMember
	= member:ValueElement "=" _ literal:Literal
		{ return { type: "EnumMember", member: member, value: literal } }
	/ member:ValueElement
		{ return { type: "EnumMember", member: member } }

// Value definitions
ValueElement
	= type:TypeDefinition name:Identifier array:ArrayQualifier?
		{ return { type: "ValueType", name: name, array: array } }

TypeDefinition
	= "void" _
		{ return { type: "VoidType" } }
	/ "bool" _
		{ return { type: "BoolType" } }
	/ "string" _ size:ArrayQualifier?
		{ return { type: "StringType", size: size } }
	/ "float_" bits:("32" / "64") _
		{ return { type: "FloatType", size: parseInt(bits, 10) } }
	/ "int_" bits:("8" / "16" / "32" / "64") _
		{ return { type: "SignedType", size: parseInt(bits, 10) } }
	/ "uint_" bits:("8" / "16" / "32" / "64") _
		{ return { type: "UnsignedType", size: parseInt(bits, 10) } }
	/ ElementName

ElementName
	= Identifier ("::" _ Identifier)*

ArrayQualifier
	= "[" _ index:TypeDefinition ":" _ size:NumberLiteral "]" _
		{ return { type: "ArrayDefinition", index: index, size: size } }
	/ "[" _ index:TypeDefinition "]" _
		{ return { type: "ArrayDefinition", index: index } }
	/ "[" _ size:NumberLiteral "]" _
		{ return { type: "ArrayDefinition", size: size } }

ReservedWord
	= "union"
	/ "enum"
	/ "message"
	/ "void"
	/ "string"
	/ "bool"
	/ "int_" ("8" / "16" / "32" / "64")
	/ "uint_" ("8" / "16" / "32" / "64")
	/ "float_" ("32" / "64")

Literal
	= NumberLiteral
	/ StringLiteral

NumberLiteral
	= "0x"i digits:$[0-9a-f]i+
		{ return parseInt(digits, 16) }	
	/ digits:$([0-9]+ ("." [0-9]+)?)
		{ return parseInt(digits, 10) }
	/ "-" _ num:NumberLiteral
		{ return -num; }

StringLiteral
	= lit:$('"' (!'"' .)* '"') _
		{ return JSON.parse(lit); }

Identifier
	= !(ReservedWord [^a-z0-9_]i) word:$([a-z_]i[a-z0-9_]i*) _
		{ return { type: 'Identifier', name: word } }

Whitespace
	= [ \n\r\t]

_ = Whitespace*
__ = Whitespace+
