"use strict";
const { syscall, ref } = require("../index");

const SYS_READ = 0;
const SYS_WRITE = 1;
const SYS_OPEN = 2;
const SYS_CLOSE = 3;

const O_RDONLY = 0;
const O_WRONLY = 0o1;
const O_CREAT = 0o100;
const O_TRUNC = 0o1000;
const O_CLOEXEC = 0o2000000;

function cstr(string) {
	let buf = Buffer.alloc(string.length+1);
	buf.write(string);
	buf[buf.length-1] = 0;
	return buf;
}

function fail(message) {
	console.error(message);
	process.exit(1);
}

function write(fd, buf, count) {
	return syscall(SYS_WRITE, fd, buf, count);
}

function read(fd, buf, count) {
	return syscall(SYS_READ, fd, buf, count);
}

function open(filename, flags, mode) {
	return syscall(SYS_OPEN, cstr(filename), flags, mode);
}

function close(fd) {
	return syscall(SYS_CLOSE, fd);
}

(() => {
	console.log("write to /tmp/test.txt");
	let fd = open("/tmp/test.txt", O_CREAT|O_WRONLY|O_TRUNC|O_CLOEXEC, 0o600);
	if (fd < 0)
		fail("can't open file");
	
	let str = "Hello world\n";
	let result = write(fd, cstr(str), str.length);
	if (result < 0)
		fail("write failed");
	console.log(`${result } bytes written`);
	
	if (close(fd) < 0)
		fail("can't close file");
})();

(() => {
	console.log("read from /tmp/test.txt");
	let fd = open("/tmp/test.txt", O_RDONLY|O_CLOEXEC, 0);
	if (fd < 0)
		fail("can't open file");
	
	let buf = Buffer.alloc(1024);
	let result = read(fd, buf, 1024);
	if (result < 0)
		fail("read failed");
	
	console.log(buf.toString("utf8", 0, parseInt(result)));
	
	if (close(fd) < 0)
		fail("can't close file");
})();
