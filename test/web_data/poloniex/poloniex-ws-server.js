var WebSocketServer = require('ws').Server,
    wss = new WebSocketServer({port: 8181});
var uuid = require('node-uuid'),
   _     = require('lodash')._;



wss.on('connection', function(ws) {
    var client_uuid = uuid.v4();
    console.log('[%s] connected', client_uuid);

    ws.on('message', function(message) {
		console.log('[%s] %s', client_uuid, message);

        var request = JSON.parse(message);
        response = {};
        if(request['command'] == 'subscribe'){
            response = [request['channel'], 1];
        }
        ws.send(JSON.stringify(response));

        if(request['channel'] == 1002){
            //[1002,null,[149,"655.85494783","656.20257851","653.00808262","-0.04198809","4070403.44163582","6117.26972201",0,"693.99999999","628.17135284"]]
            response = [1002, null, [
                           149,
                           "655.85494783",
                           "656.20257851",
                           "653.00808262",
                           "-0.04198809",
                           "4070403.44163582",
                           "6117.26972201",
                           0,
                           "693.99999999",
                           "628.17135284"]
                        ];
        }
        ws.send(JSON.stringify(response));
                            
        ws.close();
    });

    ws.on('close', function() {
        console.log('[%s] closed connection', client_uuid);
    });
});

console.log("Server up and listening...");
