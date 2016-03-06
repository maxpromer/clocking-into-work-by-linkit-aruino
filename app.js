var m = require('mraa');
var fs = require('fs');
var http = require('http');
var url = require("url");
var path = require("path");


function Sleep(delay) {
	delay += new Date().getTime();
	while (new Date() < delay) { }
}



u = new m.Uart("/dev/ttyS1");

u.setBaudRate(115200);
u.setMode(8, 0, 1);
u.setFlowcontrol(false, false);
Sleep(200);

u.readLine = function() {
	var line = "";
	while(this.dataAvailable(0)) {
		c = this.read(1).toString();
		if (c=='\r') {
			this.read(1);
			break;
		}
		line += c;
	}
	return line;
}

var isrUart = function() {
	if (u.dataAvailable(0)) {
		var line = u.readLine();
		var start = line.substring(0, 2);
		if (start=="ID") {
			var name = "[Not Found]";
			var card_id = line.substring(3);
			var users = JSON.parse(fs.readFileSync("./data/users.json", "utf8"));
			var name, id;
			for (var i=0;i<users.length;i++) {
				if (card_id == users[i].card_id) {
					name = users[i].name;
					id = i;
					break;
				}
			}
			if (name.length > 8)
				name = name.substring(0, 8) + "...";
				
			process.env.TZ = 'Asia/Bangkok';
			var time = new Date(new Date().getTime() + (7*60*60*1000));
			time = time.getFullYear() + "-" + ("0" + time.getMonth()).slice(-2) + "-" + ("0" + time.getDate()).slice(-2)
				+ " " + ("0" + time.getHours()).slice(-2) + ":" + ("0" + time.getMinutes()).slice(-2) + ":" + ("0" + time.getSeconds()).slice(-2);
			
			fs.writeFile(
				"./data/log.csv",
				time + "," + id + "\n" + fs.readFileSync("./data/log.csv", "utf8"),
				function(err) {
					if(err) {
						return console.error(err);
					}

					u.writeStr("NA:" + name + "\r\n");
					u.writeStr("TI:" + time.split(" ")[1] + "\r\n");
					console.log("Name:", name, "\tTime:", time);
				}
			);
		}
	}
	setTimeout(isrUart, 50);
}

isrUart();

http.createServer(function(request, response) {
	var uri = "site/" + url.parse(request.url).pathname
	var filename = path.join(process.cwd(), uri);
  
	if (filename.indexOf("site/data.json")>=0) {
		var users = JSON.parse(fs.readFileSync("./data/users.json", "utf8"));
		
		response.writeHead(200);
		
		var data = "[";
		var lineReader = require('readline').createInterface({
			input: require('fs').createReadStream('./data/log.csv')
		});
		
		lineReader.on('line', function (line) {
			data += "{\"name\":\"" + users[line.split(",")[1]].name + "\", \"time\": \"" + line.split(",")[0] + "\"}, ";
		});
		
		lineReader.on('close', function () {
			response.write(data.substring(0, data.length-2) + "]");
			response.end();
		});
	} else {

		fs.exists(filename, function(exists) {
			if(!exists) {
				response.writeHead(404, {"Content-Type": "text/plain"});
				response.write("404 Not Found\n");
				response.end();
				return;
			}

			if (fs.statSync(filename).isDirectory()) filename += '/index.html';

			fs.readFile(filename, "binary", function(err, file) {
				if(err) {        
					response.writeHead(500, {"Content-Type": "text/plain"});
					response.write(err + "\n");
					response.end();
					return;
				}

				response.writeHead(200);
				response.write(file, "binary");
				response.end();
			});
		});
	}
}).listen(88);

console.log('Server running at http://127.0.0.1:88/');