var global = 8 / 2;

function foo() {
	var local = "Example string";
	return 1 + 2;
}

function bar() {
	return global - foo();
}
