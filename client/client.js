var net = require('net');

var client = net.Socket();
client.connect(8888, 'localhost', function() {
    console.log('Connected to server!');
    msg = {
        "type":"REQUEST",
        "command":"AUTH",
        "username":"root",
        "password":"root"
    }
    client.write(JSON.stringify(msg)+'\0');
    //client.write('Hello server! This is client speaking!\0');
});

client.on('data', function(data) {
    console.log('Received from server: ' + data);
    //client.destroy();
});


client.on('close', function() {
    console.log('Connection closed');
});
