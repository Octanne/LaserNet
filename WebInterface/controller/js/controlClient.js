var wSocket = null;
var debugOn = false;
var isAuth = false;

if (!window.console)
		window.console = { log: function() {} };

function initControlClient() {
    wSocket = new WebSocket('wss://' + location.host + '/wss');
	wSocket.onopen = socketOnOpen;
	wSocket.onerror = socketOnError;
	wSocket.onclose = socketOnClose;
	wSocket.onmessage = socketReceiveMessage;
}


function socketOnOpen(ev) {
	if (debugOn)
		console.log(ev);
}
function socketOnError(ev) {
	if (debugOn)
		console.log(ev);
	switch (ev.eventPhase) {
        case 2:
			addLog("error", "Socket can't reach the server");
			break;
		default:
			addLog("error", "Socket Error at the phase "+ev.eventPhase);

			break;
    }
}
function socketOnClose(ev) {
    if (debugOn)
		console.log(ev);
	addLog("debug", "Socket Closed because: \""+ev.reason+"\"");
}
function socketReceiveMessage(ev) {
	var webMessage = JSON.parse(ev.data);
	//addLog("info","DATA : " + ev.data);
	//console.log("[WebSocket] new data incoming : "+webMessage.type);
	if (debugOn)addLog("debug", "[WebSocket] new data incoming : "+webMessage.type);
	
	
	if(webMessage.type == "sysStatus"){
		var temp = webMessage.temp;
		var cpuUsage = webMessage.cpuUsage;
		var ramUsage = webMessage.ramUsage;
		var netUsage = webMessage.netUsage;
		
		var webStatus = webMessage.webStatus;
		var syncStatus = webMessage.syncStatus;
		/*console.log("---------------------------------------------");
		console.log("Temp : " + temp + "°C | Network : " + netUsage + "%");
		console.log("CPU Usage : " + cpuUsage + "% | Ram Usage : " + ramUsage + "%");
		console.log("Web : " + webStatus + " | Sync : " + syncStatus);
		console.log("---------------------------------------------");*/
		if (debugOn)addLog("info", "[WebSocket] new SysStatus receive.");
		
		document.getElementById("temperature").innerHTML = temp;
		setProgressBarValue(document.getElementById("cpuUsage"), cpuUsage);
		setProgressBarValue(document.getElementById("ramUsage"), ramUsage);
		setProgressBarValue(document.getElementById("netUsageRX"), netUsage);
		
		if(temp <= 45)
			document.getElementById("temperature").setAttribute("color", "green");
		else if(temp <= 60)
			document.getElementById("temperature").setAttribute("color", "darkorange");
		else
			document.getElementById("temperature").setAttribute("color", "red");

		document.getElementById("syncStat").value = syncStatus;
		document.getElementById("webStat").value = webStatus;
	}
	if(webMessage.type == "consoleUP"){
		addLog("info", "Console is up: \""+webMessage.msg+"\"");
	}
	if(webMessage.type == "error"){
		addLog("error", "[WebSocket] : " + webMessage.msg);
	}
	if(webMessage.type == "answer"){
		addLog("info", webMessage.msg);
	}
	if(webMessage.type == "managment"){
		if(webMessage.msg == "connected")launchAutoRefresh();
		if(webMessage.msg == "disconnected")return;
	}
	if(webMessage.type == "connection"){
		isAuth = true;
		addLog("info", "[Connection] "+webMessage.msg);
	}
	if(webMessage.type == "chat"){
		addLog("info", "[Chat] "+webMessage.msg);
	}
};

function sendMessage(type, arg = "nothing"){
	if (!wSocket) {
		addLog("error", "[WebSocket] : not initialized");
		return false;
	}
	var webMessage = {
		date: Date.now().toString(),
		type: type,
		args: arg
	};
	wSocket.send(JSON.stringify(webMessage).toString());
}
function launchAutoRefresh(){
	sendMessage("status");
	setInterval(sendMessage, 4000, "status", "nothing");
}




function setProgressBarValue(progressBar, value) {
    progressBar.innerHTML = value;
    progressBar.style.width = value+"%";
}
