var fs = require('fs');

var table = [];
try {
	table = JSON.parse(fs.readFileSync("./known_robots.json"));
} catch(e){}

function add(key) {
	fs.writeFileSync("./known_robots.json", JSON.stringify(table));
	table.push(key);
}

module.exports = {
	list: table,
	add: add
};
