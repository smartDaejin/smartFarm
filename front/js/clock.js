function showClock() {
    var currentDate = new Date();
    var divClock = document.getElementById("divClock");
    var apm = currentDate.getHours();
    if (apm < 12) {
        apm = "오전";
    }
    else {
        apm = "오후";
    }

    var msg = apm + (currentDate.getHours() - 12) + "시";
    msg += currentDate.getMinutes() + "분";
    msg += currentDate.getSeconds() + "초";

    divClock.innerText = msg;

    setTimeout(showClock, 1000);
}