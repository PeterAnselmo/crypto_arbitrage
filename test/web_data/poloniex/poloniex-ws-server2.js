var WebSocketServer = require('ws').Server,
    wss = new WebSocketServer({port: 8282});
var uuid = require('node-uuid'),
   _     = require('lodash')._;


var message_delay = 22;
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
        response = {};
        if(request['command'] == 'subscribe'){
            response = [request['channel'], 1];
        }
        ws.send(JSON.stringify(response));

        if(request['channel'] == 1002){
            ws.send(JSON.stringify([1002,null,[166,"0.01989811","0.02002357","0.01980052","0.02064462","46.86228529","2395.90971361",0,"0.02017256","0.01880000"]]));
            pausecomp(message_delay);
            ws.send(JSON.stringify([1002,null,[189,"0.14514001","0.14589400","0.14516081","-0.00398016","246.24716615","1717.67107960",0,"0.14658581","0.13904930"]]));
            pausecomp(message_delay);
            ws.send(JSON.stringify([1002,null,[193,"0.00176929","0.00178106","0.00176966","0.00072963","73.88414899","41992.50285468",0,"0.00178801","0.00172815"]]));
            pausecomp(message_delay);
            ws.send(JSON.stringify([1002,null,[15,"0.00000325","0.00000325","0.00000322","0.20817843","147.85232916","48257848.32211412",0,"0.00000339","0.00000262"]]));
            pausecomp(message_delay);
            ws.send(JSON.stringify([1002,null,[121,"9040.00000000","9050.00000000","9040.00000001","-0.01739130","13971875.74185674","1553.77687675",0,"9259.44052055","8823.00000000"]]));
            pausecomp(message_delay);
            ws.send(JSON.stringify([1002,null,[123,"146.70100007","147.49969924","146.70100009","-0.01030492","938619.96287065","6447.66519339",0,"148.99999124","142.04328242"]]));
            pausecomp(message_delay);
            ws.send(JSON.stringify([1002,null,[193,"0.00176929","0.00178106","0.00176966","0.00072963","73.88414899","41992.50285468",0,"0.00178801","0.00172815"]]));
            pausecomp(message_delay);
            ws.send(JSON.stringify([1002,null,[15,"0.00000325","0.00000325","0.00000322","0.20817843","147.85232916","48257848.32211412",0,"0.00000339","0.00000262"]]));
            pausecomp(message_delay);
            ws.send(JSON.stringify([1002,null,[121,"9040.00000000","9050.00000000","9040.00000001","-0.01739130","13971875.74185674","1553.77687675",0,"9259.44052055","8823.00000000"]]));
            pausecomp(message_delay);
            ws.send(JSON.stringify([1002,null,[123,"146.70100007","147.49969924","146.70100009","-0.01030492","938619.96287065","6447.66519339",0,"148.99999124","142.04328242"]]));
            pausecomp(message_delay);
            ws.send(JSON.stringify([1002,null,[193,"0.00176929","0.00178106","0.00176966","0.00072963","73.88414899","41992.50285468",0,"0.00178801","0.00172815"]]));
            pausecomp(message_delay);
            ws.send(JSON.stringify([1002,null,[15,"0.00000325","0.00000325","0.00000322","0.20817843","147.85232916","48257848.32211412",0,"0.00000339","0.00000262"]]));
            pausecomp(message_delay);
            ws.send(JSON.stringify([1002,null,[121,"9040.00000000","9050.00000000","9040.00000001","-0.01739130","13971875.74185674","1553.77687675",0,"9259.44052055","8823.00000000"]]));
            pausecomp(message_delay);
            ws.send(JSON.stringify([1002,null,[123,"146.70100007","147.49969924","146.70100009","-0.01030492","938619.96287065","6447.66519339",0,"148.99999124","142.04328242"]]));
            pausecomp(message_delay);
            ws.send(JSON.stringify([1002,null,[193,"0.00176929","0.00178106","0.00176966","0.00072963","73.88414899","41992.50285468",0,"0.00178801","0.00172815"]]));
            pausecomp(message_delay);
            ws.send(JSON.stringify([1002,null,[15,"0.00000325","0.00000325","0.00000322","0.20817843","147.85232916","48257848.32211412",0,"0.00000339","0.00000262"]]));
            pausecomp(message_delay);
            ws.send(JSON.stringify([1002,null,[121,"9040.00000000","9050.00000000","9040.00000001","-0.01739130","13971875.74185674","1553.77687675",0,"9259.44052055","8823.00000000"]]));
            pausecomp(message_delay);
            ws.send(JSON.stringify([1002,null,[123,"146.70100007","147.49969924","146.70100009","-0.01030492","938619.96287065","6447.66519339",0,"148.99999124","142.04328242"]]));
            pausecomp(message_delay);
            ws.send(JSON.stringify([1002,null,[166,"0.01989811","0.02002356","0.01980052","0.02064462","46.86228529","2395.90971361",0,"0.02017256","0.01880000"]]));
            pausecomp(message_delay);
            ws.send(JSON.stringify([1002,null,[190,"1.95709619","1.97603928","1.95709619","-0.03491416","83.19786523","42.10535442",0,"2.02140064","1.94464880"]]));
            pausecomp(message_delay);
            ws.send(JSON.stringify([1002,null,[121,"9040.00000000","9050.00000000","9040.00000020","-0.01739130","13971875.74185674","1553.77687675",0,"9259.44052055","8823.00000000"]]));
            pausecomp(message_delay);
            ws.send(JSON.stringify([1002,null,[126,"238.99999999","239.25328845","239.07169998","-0.00905593","549272.05317976","2348.39754632",0,"241.63999999","228.00000000"]]));
            pausecomp(message_delay);
            ws.send(JSON.stringify([1002,null,[127,"0.83100000","0.83299075","0.82669744","0.01131799","2946645.54910542","3639378.24472094",0,"0.83989997","0.78445362"]]));
            pausecomp(message_delay);
            ws.send(JSON.stringify([1002,null,[166,"0.01989811","0.02002355","0.01980052","0.02064462","46.86228529","2395.90971361",0,"0.02017256","0.01880000"]]));
            pausecomp(message_delay);
            ws.send(JSON.stringify([1002,null,[198,"0.00322453","0.00424823","0.00421197","0.00586136","12.05556792","3795.09494089",0,"0.00325361","0.00307254"]]));
        }

        ws.close();
    });

    ws.on('close', function() {
        console.log('[%s] closed connection', client_uuid);
    });
});

console.log("Server up and listening...");
