var wSocket = null;
var debugOn = false;

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
	addLog("debug", "Socket Open "+ev);
	launchAutoRefresh();
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
	//console.log("[WebSocket] new data incoming : "+webMessage.type);
	addLog("info", "[WebSocket] new data incoming : "+webMessage.type);
	
	
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
		addLog("info", "---------------------------------------------"
			   +"Temp : " + temp + "°C | Network : " + netUsage + "%"
			   +"CPU Usage : " + cpuUsage + "% | Ram Usage : " + ramUsage + "%"
			   +"Web : " + webStatus + " | Sync : " + syncStatus
			   +"---------------------------------------------");
		
		document.getElementById("temperature").innerHTML = temp;
		setProgressBarValue(document.getElementById("cpuUsage"), cpuUsage);
		setProgressBarValue(document.getElementById("ramUsage"), ramUsage);
		//document.getElementById("netUsageTX").innerHTML = "<div id=\"netUsageRX\" class=\"progress-bar\" style=\"width:0%;\">"+netUsage+"</div>"+(100-netUsage);
		//document.getElementById("netUsageRX").style.width = netUsage+"%";
		setProgressBarValue(document.getElementById("netUsageRX"), netUsage);
		
		if(temp <= 45)
			document.getElementById("temperature").setAttribute("color", "green");
		else if(temp <= 60)
			document.getElementById("temperature").setAttribute("color", "darkorange");
		else
			document.getElementById("temperature").setAttribute("color", "red");

		/*if(syncStatus){
			document.getElementById("syncStat").class = "green";
			document.getElementById("syncStat").innerHTML = "ACTIVE";
		}else{
			document.getElementById("syncStat").class = "red";
			document.getElementById("syncStat").innerHTML = "INACTIF";
		}
		if(webStatus){
			document.getElementById("webStat").class = "green";
			document.getElementById("webStat").innerHTML = "ACTIVE";
		}else{
			document.getElementById("webStat").class = "red";
			document.getElementById("webStat").innerHTML = "INACTIF";
		}*/
		document.getElementById("syncStat").value = syncStatus;
		document.getElementById("webStat").value = webStatus;
	}
	if(webMessage.type == "consoleUP"){
		addLog("info", "Console is up: \""+webMessage.msg+"\"");
	}
	if(webMessage.type == "error"){
		//console.log("[WebSocket] ERROR : " + webMessage.msg);
		addLog("error", "[WebSocket] : " + webMessage.msg);
	}
};

function sendMessage(type, arg = "nothing"){
	if (!wSocket) {
		addLog("error", "[WebSocket] : not initialized");
		return false;
	}
	var webMessage = {
		secretKey: "secretKey",
		date: Date.now().toString(),
		order: type,
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
