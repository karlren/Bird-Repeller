#include "MyOTA.h"
const char* OTAhost = "esp32"; // 定义OTA主机名
WebServer OTAserver(80); // 创建Web服务器，监听80端口
/*
 * 登录页面
 */
static const char* OTAloginIndex =
"<form name='loginForm'>"
    "<table width='20%' bgcolor='A09F9F' align='center'>"
        "<tr>"
            "<td colspan=2>"
                "<center><font size=4><b>ESP32 Login Page</b></font></center>" // 登录页面标题
                "<br>"
            "</td>"
            "<br>"
            "<br>"
        "</tr>"
        "<tr>"
             "<td>Username:</td>" // 用户名标签
             "<td><input type='text' size=25 name='userid'><br></td>" // 用户名输入框
        "</tr>"
        "<br>"
        "<br>"
        "<tr>"
            "<td>Password:</td>" // 密码标签
            "<td><input type='Password' size=25 name='pwd'><br></td>" // 密码输入框
            "<br>"
            "<br>"
        "</tr>"
        "<tr>"
            "<td><input type='submit' onclick='check(this.form)' value='Login'></td>" // 登录按钮
        "</tr>"
    "</table>"
"</form>"
"<script>"
    "function check(form)" // 登录验证函数
    "{"
    "if(form.userid.value=='admin' && form.pwd.value=='admin')"
    "{"
    "window.open('/OTAserverIndex')" // 登录成功后跳转到主页面
    "}"
    "else"
    "{"
    " alert('Error Password or Username')/*显示错误消息*/" // 登录失败时提示
    "}"
    "}"
"</script>";

/*
 * 服务器主页面
 */
static const char* OTAserverIndex =
"<!DOCTYPE html>"
"<html lang='zh'>"
"<head>"
"<meta charset='UTF-8'>"
"<title>ESP32 OTA</title>"
"</head>"
"<body>"
"<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>" // 引入jQuery
"<form method='POST' action='update' enctype='multipart/form-data' id='upload_form'>"
   "<input type='file' name='update'>"
   "<input type='submit' value='Update'>" // 更新按钮
"</form>"
"<div id='prg'>progress: 0%</div>" // 上传进度显示
"<div id='loader' style='display:none;'>正在上传，请稍候...</div>" // 上传过程提示
"<script>"
    "$('form').submit(function(e){" // 提交表单时的处理
    "e.preventDefault();" // 阻止默认提交
    "$('#loader').show();" // 显示上传提示
    "var form = $('#upload_form')[0];" // 获取表单元素
    "var data = new FormData(form);"
    " $.ajax({"
    "url: '/update',"
    "type: 'POST',"
    "data: data,"
    "contentType: false,"
    "processData: false,"
    "xhr: function() {"
    "var xhr = new window.XMLHttpRequest();"
    "xhr.upload.addEventListener('progress', function(evt) {" // 监听上传进度
    "if (evt.lengthComputable) {"
    "var per = evt.loaded / evt.total;" // 计算上传进度
    "$('#prg').html('progress: ' + Math.round(per*100) + '%');" // 更新进度显示
    "}"
    "}, false);"
    "return xhr;"
    "},"
    "success:function(d, s) {" // 上传成功的处理
    "console.log('success!');"
    "$('#loader').hide();"
    "},"
    "error: function (a, b, c) {" // 上传失败的处理
    "console.error('上传失败', a, b, c);"
    "$('#loader').hide();"
    "}"
    "});"
    "});"
"</script>"
"</body>"
"</html>";

/*
 * 初始化OTA功能
 */
void OTA_Init(void)
{
    /* 使用mDNS进行OTA主机名解析 */
    if (!MDNS.begin(OTAhost)) { // http://esp32.local
        Serial.println("Error setting up MDNS responder!"); // 初始化失败时输出错误信息
        while (1) {
            vTaskDelay(pdMS_TO_TICKS(1000)); // 循环等待
        }
    }
    // Serial.println("mDNS responder started");
    /* 返回存储在OTAserverIndex中的首页 */
    OTAserver.on("/", HTTP_GET, []() {
        OTAserver.sendHeader("Connection", "close"); // 设置连接关闭
        OTAserver.send(200, "text/html; charset=utf-8", OTAloginIndex); // 返回登录页面
    });
    OTAserver.on("/OTAserverIndex", HTTP_GET, []() {
        OTAserver.sendHeader("Connection", "close"); // 设置连接关闭
        OTAserver.send(200, "text/html; charset=utf-8", OTAserverIndex); // 返回主页面
    });
    /* 处理固件文件上传 */
    OTAserver.on("/update", HTTP_POST, []() {
        OTAserver.sendHeader("Connection", "close"); // 设置连接关闭
        OTAserver.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK"); // 返回上传结果
        ESP.restart(); // 重启ESP
    }, []() {
        HTTPUpload& upload = OTAserver.upload(); // 获取上传对象
        if (upload.status == UPLOAD_FILE_START) {
            Serial.printf("Update: %s\n", upload.filename.c_str()); // 输出上传文件名
            if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { // 以最大可用大小开始更新
                Update.printError(Serial); // 输出错误信息
            }
        } else if (upload.status == UPLOAD_FILE_WRITE) {
            /* 将固件写入ESP */
            if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
                Update.printError(Serial); // 输出错误信息
            }
        } else if (upload.status == UPLOAD_FILE_END) {
            if (Update.end(true)) { // true表示设置大小为当前进度
                Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize); // 输出更新成功信息
            } else {
                Update.printError(Serial); // 输出错误信息
            }
        }
    });
    OTAserver.begin(); // 启动服务器
}

void OTA_Get(void)
{
    OTAserver.handleClient(); // 处理客户端请求
}
