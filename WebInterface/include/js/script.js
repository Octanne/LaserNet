//events (mieux que window.event=function)
if(window.addEventListener) {
    window.addEventListener("load",onLoadPage,false);
    window.addEventListener("hashchange",loadPage,false);
} else if (window.attachEvent) {
    window.attachEvent("onload",onLoadPage);
    window.attachEvent("onhashchange",loadPage);
}


function onLoadPage() {
    if(navigator.appName != "Netscape")//Chrome, Edge, Firefox, Opéra, (IE11?)
        alert("Warning: this site is maybe not avaiable in this browser");
    //opéra : tout semble fonctionner
    //firefox : il arrive qu'il y ai des bugs (?)
    //edge : pas de scrollbar avec la version 18204//nouveau edge : fonctionne bien //https://caniuse.com/#feat=css-scrollbar
    loadPage();
}

function loadPage() {
    var elementFocus = window.location.href;
    elementFocus = elementFocus.substring(elementFocus.lastIndexOf('/')+2);
    var menu_links = document.getElementsByClassName("menu_link");
    if (menu_links.length >= 1 && (elementFocus == "" || !document.getElementById("page"+elementFocus))) {
        elementFocus = menu_links[0].id.substring(menu_links[0].id.lastIndexOf("menu")+4);
        //si elementFocus n'est pas défini (url sans #Name) alors le premier onglet est selectionné
    }
    
    //change le "style" des élements actuels (sommaire et page)
    for (let i=0; i < menu_links.length; i++) {
        menu_links[i].setAttribute("open", menu_links[i].id == "menu"+elementFocus ? "true" : "false");
    }
    var pages_content = document.getElementsByClassName("pageContent");
    for (let i=0; i < pages_content.length; i++) {
        pages_content[i].setAttribute("open", pages_content[i].id == "page"+elementFocus ? "true" : "false");
    }
    
    if (elementFocus == "Controller" && document.getElementById("consoleLine")) {
        document.getElementById("consoleLine").focus();
    }
}
function openPage(element) {
    window.open("./#"+element.id.substring((element.id.indexOf('u')+1)),"_self");
    //l'actualisation va se faire par loadPage lors de l'evenement onhashchange
}


