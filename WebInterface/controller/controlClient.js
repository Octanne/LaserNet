var wSocket = null;
var debugOn = false;
var isAuth = false;
var isConnect = false
var intervalStatus = null;

if (!window.console)
		window.console = { log: function() {} };

$(window).on("beforeunload", function() { 
    wSocket.close();
})

function initControlClient() {
    wSocket = new WebSocket('wss://' + location.host + '/wss');
	wSocket.onopen = socketOnOpen;
	wSocket.onerror = socketOnError;
	wSocket.onclose = socketOnClose;
	wSocket.onmessage = socketReceiveMessage;
}


function socketOnOpen(ev) {
	isConnect = true;
	if (debugOn)
		console.log(ev);
	addLog("info","Connexion effectuée, !auth [secretkey] pour vous authentifier");
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
	addLog("info","Connexion interrompue, !connect pour vous connecter");
	isAuth = false;
	isConnect = false;
	clearInterval(intervalStatus);
}
function socketReceiveMessage(ev) {
	var webMessage = JSON.parse(ev.data);
	if (debugOn)addLog("debug", "[WebSocket] new data incoming : "+webMessage.type);
	
	if(webMessage.type == "sysStatus"){
		var temp = webMessage.temp;
		var cpuUsage = parseFloat(webMessage.cpuUsage).toFixed(2);
		var ramUsage = parseFloat(webMessage.ramUsage).toFixed(1);
		var netUsage = parseFloat(webMessage.netUsage).toFixed(1);
		
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
	else if(webMessage.type == "consoleUP"){
		addLog("info", "Console is up: \""+webMessage.msg+"\"");
	}
	else if(webMessage.type == "error"){
		addLog("error", "[WebSocket] : " + webMessage.msg);
	}
	else if(webMessage.type == "answer"){
		addLog("info", webMessage.msg);
	}
	else if(webMessage.type == "managment"){
		if(webMessage.msg == "connected"){
			interval = launchAutoRefresh();
			isAuth = true;
		}else if(webMessage.msg == "disconnected"){
			wSocket.close();
			isAuth = false;
		}
	}else if(webMessage.type == "connection"){
		addLog("info", "[Connection] "+webMessage.msg);
	}else if(webMessage.type == "chat"){
		addLog("info", "[Chat] "+webMessage.msg);
	}else{
		addLog("error", "Type inconnue : " + webMessage.type);
		return;
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
	intervalStatus = setInterval(sendMessage, 4000, "status", "nothing");
}




function setProgressBarValue(progressBar, value) {
    progressBar.innerHTML = value;
    progressBar.style.width = value+"%";
}
