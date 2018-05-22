var WebSocketServer = require('ws').Server,
    wss = new WebSocketServer({port: 8282});
var uuid = require('node-uuid'),
   _     = require('lodash')._;


var message_delay = 4;
function sleep(ms) {
  return new Promise(resolve => setTimeout(resolve, ms));
}
function pausecomp(millis)
{
    var date = new Date();
    var curDate = null;
    do { curDate = new Date(); }
    while(curDate-date < millis);
}

wss.on('connection', function(ws) {
    var client_uuid = uuid.v4();
    console.log('[%s] connected', client_uuid);

    ws.on('message', function(message) {
		console.log('[%s] %s', client_uuid, message);

        var request = JSON.parse(message);
        /*
        response = {};
        if(request['command'] == 'subscribe'){
            response = [request['channel'], 1];
        }
        ws.send(JSON.stringify(response));
        */

        var lineReader = require('readline').createInterface({
              input: require('fs').createReadStream('ws_feed_data.txt')
        });

        if(request['channel'] == 1002){
            lineReader.on('line', function (line) {
                  console.log('Line from file:', line);
                  ws.send(line);
            });
            /*
            ws.send(JSON.stringify([1002,null,[166,"0.01989811","0.02002357","0.01980052","0.02064462","46.86228529","2395.90971361",0,"0.02017256","0.01880000"]]));
            pausecomp(message_delay);
            */
        }

        //ws.close();
    });

    ws.on('close', function() {
        console.log('[%s] closed connection', client_uuid);
    });
});

console.log("Server up and listening...");
