start
	= _ x:Statement*
		{ return x.join(" "); }

Statement
	= Include
	/ x:StringLiteral
		{ return JSON.stringify(x) }
	/ x:$(!Whitespace .)+ _
		{ return x; }

Include
	= "#include" __ fn:StringLiteral
		{ return parser.load(fn); }

StringLiteral
	= '"' lit:$(!'"' .)* '"' _
		{ return lit; }

Whitespace
	= [ \n\r\t]
	/ Comment

Comment
	= "//" (![\n\r] .)* 
	/ "/*" (!"*/" .)* "*/"

_ = Whitespace*
	{ return " " }

__ = Whitespace+
	{ return " " }
