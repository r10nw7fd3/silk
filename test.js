var foo = 16 / 2;
bar();

function multiply(val1, val2) {
	var something = val1 * val2;
	return something - 1;
}

function bar() {
	return foo - multiply(2, 3);
}
