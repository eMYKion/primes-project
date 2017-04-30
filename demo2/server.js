const app = require('express')();
const server = require('http').Server(app);
const io = require('socket.io')(server);
//const child_process = require("child_process");//this is asynchronous

//const spawn = child_process.spawn;

const IP = '192.168.1.10';
const PORT = 3000;

app.get('/', function(req, res){
  res.sendFile('/home/root/demo2/client-media/index.html');
});

app.get('/socket.io.js', function(req, res){
  res.sendFile('/home/root/demo2/client-media/socket.io.js');
});

io.on('connection', function(socket){
  console.log('a user connected');
});

server.listen(PORT, IP, function(){
  console.log("listening on " + IP + ":" + PORT);
});