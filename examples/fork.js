"use strict";
const { syscall, ref } = require("../index");

const SYS_DUP2 = 33;
const SYS_FORK = 57;
const SYS_EXECVE = 59;
const SYS_WAIT4 = 61;
const WNOHANG = 1;
const EBUSY = 16;

function cstr(string) {
	let buf = Buffer.alloc(string.length+1);
	buf.write(string);
	buf[buf.length-1] = 0;
	return buf;
}

function array(buffers) {
	let buf = new BigUint64Array(buffers.length+1);
	for (let i = 0; i < buffers.length; i++) {
		let x = ref(buffers[i]);
		buf[i] = x;
	}
	buf[buf.length-1] = 0n;
	return buf;
}

function fork() {
	return parseInt(syscall(SYS_FORK));
}

function execvp(file, argv) {
	let filename = cstr(file);
	let args = array(argv.map(cstr));
	return syscall(SYS_EXECVE, filename, args, 0);
}

function dup2(oldFd, newFd) {
	let result;
	while ((result = syscall(SYS_DUP2, oldFd, newFd)) == -EBUSY);
	return result;
}

function wait4(pid, statusPtr, options, usagePtr) {
	return parseInt(syscall(SYS_WAIT4, pid, statusPtr, options, usagePtr));
}

process.on("SIGCHLD", () => {
	console.log("[parent] received SIGCHLD");
	while (true) {
		let pid = wait4(-1, 0, WNOHANG, 0);
		if (pid <= 0) break;
		console.log(`[parent] reaped zombie ${pid}`);
	}
	
	process.exit(0); // child did its thing
});

let pid = fork();
if (pid === 0) {
	console.log("[child] execing /bin/echo");
	
	dup2(process.stdin._handle.fd, 0);
	dup2(process.stdin._handle.fd, 1);
	dup2(process.stdin._handle.fd, 2);
	
	let result = execvp("/bin/echo", ["/bin/echo", "hello"]);
	console.log("[child] execvp error", result);
} else if (pid < 0) {
	console.log("[parent] fork() error");
} else {
	console.log("[parent] spawned child process with pid", pid);
	setInterval(() => {}, 100000); // keep process running
}
