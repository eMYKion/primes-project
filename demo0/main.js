const spawn = require("child_process").spawn;//asynchonous

var child = spawn("./laser.o");

child.stdout.on('data', function (dataChunk) {
    console.log('stdout: ' + dataChunk.toString());
});

child.stderr.on('data', function (data) {
  console.log('stderr: ' + data.toString());
});

child.on('exit', function (code) {
  console.log('child process exited with code ' + code.toString());
});