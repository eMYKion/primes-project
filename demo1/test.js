const LOG_FILE = "./list.log";

const spawn = require("child_process").spawn;//this is asynchronous

var app = require('express')();
var http = require('http').Server();

var io = require('socket.io')(http);

var evConsts = require("./events-server.js");


//only need this if server is serving the page

app.get('/', function(req, res){
  res.sendFile('C:/Users/Mali/Documents/MIT Primes 2016-17/demo1/index.html');
});

app.get('/socket.io-client/dist/socket.io.min.js', function(req, res){
  res.sendFile('C:/Users/Mali/Documents/MIT Primes 2016-17/demo1/socket.io-client/dist/socket.io.min.js');
});

startTime = Date.now();

io.on('connection', function(socket){
  
  //read from the laser simulation
  var child = spawn("laser.exe");

  child.stdout.on('data', function (dataChunk) {
    
    console.log('stdout: ' + dataChunk.toString());
    
    var tmstmp = (Date.now() - startTime)/1000.0;
    
    socket.emit(evConsts.LASER_TEMP,{data: parseFloat(dataChunk), timestamp: tmstmp});
    
  });

  //child.stdout.pipe(dest);
  
  child.stderr.on('data', function (data) {
    console.log('stderr: ' + data.toString());
  });

  child.on('exit', function (code) {
    console.log('child process exited with code ' + code.toString());
  });
  
  console.log('a user connected');
  
  socket.on(evConsts.USR_MSG, function(type){
    console.log("user says: " + type.data);
  });
  
  setInterval(function(){
    var time = (Date.now() - startTime)/1000.0;
    
    socket.emit(evConsts.LASER_TEMP,{data: (Math.random()-0.5)*5 + 50*Math.cos(2*Math.PI/100.0*time) + 100, timestamp: time});
  }, 1000);
  
  socket.on("disconnect", function(){
    console.log('a user disconnected');
  });
});

http.listen(8000, '127.0.0.1', function(){
  console.log('listening on localhost:8000');
});
