#this demo1 was developed on a Windows 10 platform, using Chrome as the browser
#in order to run this demo, you need node_modules in the same scope as the demo folder, with chart.js, 
#child_process, and socket.io modules.

#to compile laser.cpp to laser.o :
  g++ laser.cpp -o laser.exe -std=c++11

#runnung the demo
1. open index.html in a browser (preferrably Chrome)
2. run test.js using node:

  node test.js
  
The website should be populated with a dynamically updating line graph

#here's how it works
The laser.exe program acts as the laser device, outputting number every fraction of a second,
to the parent process (usually the terminal if you just run it from cmd).

When you run:

  node test.js
  
the program creates a socket-based server AND runs laser.exe as a child process, so it will get the 
output of laser.exe. If the browser running index.html is open, it is hardwired to connect to the local
port that test.js is serving on. When test.js recieves a connection from the browser running index.html, 
it will start emitting the information in "real-time" to the web-browser, which will plot the data 
points on a nice little chart (using a library called chart.js).

Going forward, we will need to learn how to communicate data through a serial socket connection between
test.js (the main javascript program), and the laser device.

We will also have to implement client to laser commands, so test.js will have to listen to events 
emitted by the client browser through their socket (which is different from the socket between test.js 
and the laser device).