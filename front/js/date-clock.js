function showClock() {
    var currentDate = new Date();
    var divClock = document.getElementById("divClock");
	var week = ['일', '월', '화', '수', '목', '금', '토'];

    var msg = currentDate.getFullYear() + "년";
    msg += currentDate.getMonth()+1 + "월"; //monthIndex를 반환해주기 때문에 1을 더해준다.
    msg += currentDate.getDate() + "일";
    msg += "("+week[currentDate.getDay()]+")";
    msg += " ";
    msg += currentDate.getHours() + "시";
    msg += currentDate.getMinutes() + "분";
    msg += currentDate.getSeconds() + "초";

    divClock.innerText = msg;

    setTimeout(showClock, 1000);
}