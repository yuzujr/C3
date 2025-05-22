// toast.js

// 显示 Toast 提示框的函数
function showToast(message) {
    var toast = document.getElementById("toast");
    toast.innerHTML = message;   // 设置提示框内容
    toast.className = "toast show";  // 显示提示框

    // 3秒后自动隐藏
    setTimeout(function() {
        toast.className = toast.className.replace("show", "");  // 隐藏提示框
    }, 3000);  // Toast 显示 3 秒钟后消失
}
