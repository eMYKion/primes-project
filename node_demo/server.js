var net = require('net');
var JsonSocket = require('json-socket');
var EventEmitter = require('events').EventEmitter;

var globalEvents = new EventEmitter();

////////////
// TCP

var port = 9838;
var server = net.createServer();
server.on('connection', function(socket) {
  socket = new JsonSocket(socket);
  
  var sendFunc = socket.sendMessage.bind(socket);
  globalEvents.on('message-to-server', sendFunc);
  socket.on('close', function() {
    globalEvents.removeListener('message-to-server', sendFunc);
  });

  socket.on('data', function(message) {
    globalEvents.emit('message-to-browser', JSON.parse(message.toString('utf8')));
  });
});

server.on('listening', function() {
  console.log('listening', server.address().port);
});

// if already in use, use next port
server.on('error', function (e) {
  if (e.errno === 'EADDRINUSE') {
    port++; server.listen(port);
  } else {
    throw e;
  }
});
server.listen(port);

////////////
// HTTP

var express = require('express');
var app = express();
var sse = require('server-sent-events');

var bodyParser = require('body-parser');
app.use(bodyParser.json());
app.use(bodyParser.urlencoded({ extended: false }));
app.use(express.static(require('path').join(__dirname, 'public')));

function sendingFuncTemplate(obj) {
  this.sse('data: ' + JSON.stringify(obj) + '\n\n');
}

app.get('/stream', sse, function (req, response) {
  var sendFunc = sendingFuncTemplate.bind(response);

  globalEvents.on('message-to-browser', sendFunc);
  req.on('close', function() {
    globalEvents.removeListener('message-to-browser', sendFunc);
  });
});

app.get('/port', function (req, res) {
  res.json(port);
});

// This is copy and pasted from before, didnt finish this yet.
/*app.post('/pichatat', function (req, res) {
  console.log(req.body);
  // if(req.body.hello) globalsocket.push(req.body.hello);
  globalEvents.emit('message-to-server', req.body);
  res.header("Access-Control-Allow-Origin", "*");
  res.json({});
});*/

app.listen(3000);
