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
	if (debugOn)addLog("debug", "[WebSocket] new data en approche : "+webMessage.type);
	if(debugOn)addLog("debug","[WebSocket] data : " +ev.data);
	
	if(webMessage.type == "sysStatus"){
		var temp = webMessage.temp;
		var cpuUsage = parseFloat(webMessage.cpuUsage).toFixed(2);
		var ramUsage = parseFloat(webMessage.ramUsage).toFixed(1);
		var totalDown = webMessage.totalDown;
		var totalUp = webMessage.totalUp;
		
		/*console.log("---------------------------------------------");
		console.log("Temp : " + temp + "°C | Network : " + netUsage + "%");
		console.log("CPU Usage : " + cpuUsage + "% | Ram Usage : " + ramUsage + "%");
		console.log("Web : " + webMessage.webStatus + ");
		console.log("---------------------------------------------");*/
		if (debugOn)addLog("info", "[WebSocket] new SysStatus reçu.");
		
		document.getElementById("temperature").innerHTML = temp;
		setProgressBarValue(document.getElementById("cpuUsage"), cpuUsage);
		setProgressBarValue(document.getElementById("ramUsage"), ramUsage);
		
		if(temp <= 45)
			document.getElementById("temperature").setAttribute("color", "green");
		else if(temp <= 60)
			document.getElementById("temperature").setAttribute("color", "darkorange");
		else
			document.getElementById("temperature").setAttribute("color", "red");

		document.getElementById("webStat").value = webMessage.webStatus;

		document.getElementById("wiringPiStatus").setAttribute("value", webMessage.wiringPiStatus);
		document.getElementById("tinsStatus").setAttribute("value", webMessage.tinsStatus);

		var upload = webMessage.totalUpload;
		if(upload < 1000000){
			document.getElementById("totalUpload").setAttribute("unit", "k");
			document.getElementById("totalUpload").value = parseFloat(upload / 1000).toFixed(3);
		}else if(upload >= 1000000 && upload < 1000000000){
			document.getElementById("totalUpload").setAttribute("unit", "m");
			document.getElementById("totalUpload").value = parseFloat(upload / 1000000).toFixed(3);
		}else{
			document.getElementById("totalUpload").setAttribute("unit", "g");
			document.getElementById("totalUpload").value = parseFloat(upload / 1000000000).toFixed(3);
		}
		var download = webMessage.totalDownload;
		if(download < 1000000){
			document.getElementById("totalDownload").setAttribute("unit", "k");
			document.getElementById("totalDownload").value = parseFloat(download / 1000).toFixed(3);
		}else if(download >= 1000000 && download < 1000000000){
			document.getElementById("totalDownload").setAttribute("unit", "m");
			document.getElementById("totalDownload").value = parseFloat(download / 1000000).toFixed(3);
		}else{
			document.getElementById("totalDownload").setAttribute("unit", "g");
			document.getElementById("totalDownload").value = parseFloat(download / 1000000000).toFixed(3);
		}

		var time = new Date(webMessage.workingTime / 1000 - 3600000);
		document.getElementById("workingTime").innerHTML = time.toLocaleTimeString("fr-fr");
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
		addLog("info", "[Connexion] "+webMessage.msg);
	}else if(webMessage.type == "chat"){
		addLog("info", "[Chat] "+webMessage.msg);
	}else{
		addLog("error", "Type inconnue : " + webMessage.type);
		return;
	}
		
};

function sendMessage(type, arg = "nothing"){
	if (!wSocket) {
		addLog("error", "[WebSocket] : non initialisé");
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
