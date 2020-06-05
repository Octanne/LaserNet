if(window.addEventListener) {
    window.addEventListener("load",onLoadPage,false);
} else if (window.attachEvent) {
    window.attachEvent("onload",onLoadPage);
}



function onLoadPage() {
    $("#darkmodeClicker").on("click", function (e) {
        setDarkMode(!$("#darkmodeChecker")[0].checked);
    });
    setDarkMode(localStorage.getItem('darkmode'));
}

function setDarkMode(enabled) {
    document.getElementById("darkmodeSwitcher").setAttribute("darkmode", enabled);
    localStorage.setItem('darkmode', enabled);/*https://developer.mozilla.org/en-US/docs/Learn/JavaScript/Client-side_web_APIs/Client-side_storage#Basic_syntax*/
    if (typeof(enabled) == "string") {
        $("#darkmodeChecker")[0].checked = (enabled=="true");
    }
}


