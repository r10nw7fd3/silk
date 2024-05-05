var overshadow = 16 / 2;
bar();

function foo() {
	var overshadow = 1 + 2;
	return overshadow * 2;
}

function bar() {
	return overshadow - foo();
}
