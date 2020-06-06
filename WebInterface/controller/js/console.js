if(window.addEventListener) {
    window.addEventListener("load",onLoadPage,false);
} else if (window.attachEvent) {
    window.attachEvent("onload",onLoadPage);
}


var consoleScrollEnd = true;

function loadStyle(source) {
    $('head').append('<link rel="stylesheet" type="text/css" href="'+source+'">');
}
function loadScript(source) {
    $('head').append('<script type="text/javascript" src="'+source+'"></script>');
}


function onLoadPage() {
    loadScript("controller/js/controlClient.js");
    loadStyle("controller/css/widgets.css");
    document.getElementById("credit").className = "widget";
    $("#credit").append(
        `<h3>Developer's Data</h3>
        <p>
            Project directed by<br/>
            Jérôme Lécuyer & Corentin Levalet<br/>
            ISN Project - 2019-2020
        </p>`);
    //création de la console des widgets...
}


function addLog(type, arg) {
    var time = new Date();//ms
    var timeStr = (time.getHours()<10?"0":"")+time.getHours()
                +":"+(time.getMinutes()<10?"0":"")+time.getMinutes()
                +":"+(time.getSeconds()<10?"0":"")+time.getSeconds()
                +"."+(time.getMilliseconds()<100?"0":"")+(time.getMilliseconds()<10?"0":"")+time.getMilliseconds();
    if (typeof arg == "string")
        arg = arg.split("\n").join("<br>");//replace \n by <br>
    
    $("#console").append("<div class='console_line' type='"+type+"'>"
                      +"<time datetime='"+time.toString()+"'>"
                      +timeStr+"</time>"
                      +"<p>"+arg+"</p>"
                      +"</div>");

    if (debugOn)console.log("addLog");
    if (consoleScrollEnd) {
        scrollToTheBottom();
    }
}
function scrollToTheBottom() {
    var maxScroll = $("#console")[0].scrollHeight - $("#console").outerHeight();
    $("#console").animate({ scrollTop: maxScroll }, 0);//animated with 2nd valueat 100
    //document.getElementById("console").scrollTo(0, maxScroll);//scroll to the bottom (doesn't work with edge 18204)
}
function splitLine(line, separation) {
    var lineStrings = line.split("\"");//string
    var lineSplit = [];
    for (let i=0; i<lineStrings.length; i+=2) {
        var lineNoString = lineStrings[i].split(separation);
        while (lineNoString.length > 0)
            lineSplit.push(lineNoString.shift());//it's not a string => multiple strings
        if ((i+1) < lineStrings.length)
            lineSplit.push(lineStrings[i+1]);//it's a string here (between 2 ")
    }
    return lineSplit;
}
function newCommand(command) {
    if(command.startsWith("!")) {
        var args = splitLine(command, " ");
        command = args.shift().toLowerCase();
        switch(command) {
            case "!help":
                addLog("help", "Aide de la console :\n"
                   +"    !help : affiche cette aide\n"
                   +"    !auth : s'authentifier\n"
				   +"    !debug : switch debug mod\n"
				   +"    !refresh : refresh widgets\n"
				   +"    !connect : start connection\n"
				   +"    !disconnect : stop connection\n"
				   +"    [commande] : envoie une commande du RaspberryPi\n"
                   +"    <a href='./#Documentation' style='color: -webkit-link;'>Obtenir plus d'informations</a>");
                break;
            case "!auth":
				if(isConnect){
					if(args.length > 0){
						var authMessage = {
						type: "auth",
						date: Date.now().toString(),
						key: args[0]
						};
						wSocket.send(JSON.stringify(authMessage).toString());
					}else{
						addLog("error", "Veulliez préciser la secret key : !auth [secretkey]");
					}
				}else{
					addLog("error", "Veulliez lancer une connexion avant : !connect");
				}
				break;
			case "!debug":
				if(debugOn){
					debugOn = false;
					addLog("info", "Message de DEBUG désactivé!");
				}
				else {
					debugOn = true;
					addLog("info", "Message de DEBUG activé!");
				}
				break;
			case "!refresh":
				sendMessage("status", "nothing");
				addLog("info", "Rafraichissement des widgets!");
				break;
			case "!disconnect":
				if(isConnect){
					wSocket.close();
				}else{
					addLog("error", "Aucune connection active!");
				}
				break;
			case "!connect":
				if(!isConnect){
					initControlClient();
				}else{
					addLog("error", "Connection déjà active!");
				}
				break;
			case "!clear":
				$(".console_line").remove("");
				addLog("info", "La console a été vidée!");
				break;
			default:
                addLog("error", "Commande de la console inconnue : \"" + command + "\"");
                break;
        }
    }
    else
        sendMessage("command", command);
}