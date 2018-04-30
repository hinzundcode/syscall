"use strict";
const { syscall, ref } = require("../index");

function write(fd, buf, count) {
	return syscall(1, fd, buf, count);
}

let buf = Buffer.from("Hello World\n");
let result = write(1, ref(buf), buf.length);
console.log(result);
