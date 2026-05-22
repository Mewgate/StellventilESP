#include "html_pages.h"

const char SAVED_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="de">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <title>WLAN gespeichert</title>
  <style>
    body{
      font-family: Arial, sans-serif;
      margin:0;
      min-height:100vh;
      display:flex;
      align-items:center;
      justify-content:center;
      background:#f5f5f5;
      color:#333;
    }
    .card{
      width:100%;
      max-width:460px;
      background:#fff;
      padding:24px;
      border-radius:16px;
      box-shadow:0 10px 30px rgba(0,0,0,0.15);
      text-align:center;
    }
    h2{margin-top:0;}
    p{line-height:1.5;}
    .spinner{
      width:44px;
      height:44px;
      margin:18px auto;
      border:5px solid #e5e7eb;
      border-top-color:orange;
      border-radius:50%;
      animation:spin 1s linear infinite;
    }
    .hint{
      font-size:14px;
      color:#666;
      margin-top:12px;
    }
    .dashboard-url{
      margin:16px 0;
      padding:12px;
      border-radius:10px;
      background:#f3f4f6;
      overflow-wrap:anywhere;
    }
    a{
      color:#d97706;
      font-weight:bold;
      text-decoration:none;
    }
    button{
      margin-top:12px;
      width:100%;
      padding:12px;
      border:none;
      border-radius:10px;
      background:orange;
      color:white;
      font-size:16px;
      font-weight:bold;
      cursor:pointer;
    }
    @keyframes spin{
      to{transform:rotate(360deg);}
    }
  </style>
</head>
<body>
  <div class="card">
    <h2>WLAN gespeichert</h2>
    <div class="spinner"></div>
    <p><strong>%DEVICE_NAME%</strong></p>
    <p>Der ESP ist jetzt im Ziel-WLAN erreichbar.</p>
    <p class="hint">Verbinde dieses Gerät mit demselben WLAN und öffne dann diese Adresse:</p>
    <p class="dashboard-url">%DASHBOARD_LINK%</p>
    <p class="hint">%MDNS_LINK%</p>
    <button type="button" onclick="startRedirect()">Automatische Weiterleitung starten</button>
    <p class="hint">
      Die IP bleibt hier sichtbar, falls die Weiterleitung von diesem Gerät aus nicht erreichbar ist.
    </p>
    <p class="hint" style="display:none">
      Falls es nicht automatisch klappt, bitte nach ein paar Sekunden manuell öffnen.
    </p>
  </div>
  <script>
    function startRedirect() {
      var button = document.querySelector("button");
      var secondsLeft = %REDIRECT_SECONDS%;
      button.disabled = true;
      button.textContent = "Weiterleitung in " + secondsLeft + " Sekunden...";

      var countdown = setInterval(function() {
        secondsLeft--;

        if (secondsLeft <= 0) {
          clearInterval(countdown);
          button.textContent = "Weiterleitung startet...";
          window.location.href = "%DASHBOARD_URL%";
          return;
        }

        button.textContent = "Weiterleitung in " + secondsLeft + " Sekunden...";
      }, 1000);
    }
  </script>
</body>
</html>
)rawliteral";

const char WIFI_CONNECT_WAIT_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="de">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <title>WLAN Verbindung wird geprueft</title>
  <style>
    body{
      font-family: Arial, sans-serif;
      margin:0;
      min-height:100vh;
      display:flex;
      align-items:center;
      justify-content:center;
      background:#f5f5f5;
      color:#333;
    }
    .card{
      width:100%;
      max-width:460px;
      background:#fff;
      padding:24px;
      border-radius:16px;
      box-shadow:0 10px 30px rgba(0,0,0,0.15);
      text-align:center;
    }
    h2{margin-top:0;}
    p{line-height:1.5;}
    .spinner{
      width:44px;
      height:44px;
      margin:18px auto;
      border:5px solid #e5e7eb;
      border-top-color:orange;
      border-radius:50%;
      animation:spin 1s linear infinite;
    }
    .hint{
      font-size:14px;
      color:#666;
    }
    @keyframes spin{
      to{transform:rotate(360deg);}
    }
  </style>
</head>
<body>
  <div class="card">
    <h2>WLAN wird geprüft</h2>
    <div class="spinner"></div>
    <p>Der ESP versucht sich mit dem WLAN "%SSID%" zu verbinden.</p>
    <p class="hint">Falls die Verbindung zum Setup-WLAN kurz abbricht, bitte wieder mit dem Setup-WLAN verbinden. Diese Seite prüft automatisch weiter.</p>
  </div>
  <script>
    function pollStatus() {
      fetch("/wifi-status", { cache: "no-store" })
        .then(function(response) { return response.json(); })
        .then(function(status) {
          if (status.state === "connected" && status.url) {
            window.location.href = status.url;
            return;
          }

          if (status.state === "failed") {
            window.location.href = "/wifi-failed";
            return;
          }

          setTimeout(pollStatus, 1000);
        })
        .catch(function() {
          setTimeout(pollStatus, 1500);
        });
    }

    setTimeout(pollStatus, 1000);
  </script>
</body>
</html>
)rawliteral";

const char WIFI_CONNECT_FAILED_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="de">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <title>WLAN Verbindung fehlgeschlagen</title>
  <style>
    body{
      font-family: Arial, sans-serif;
      margin:0;
      min-height:100vh;
      display:flex;
      align-items:center;
      justify-content:center;
      background:#f5f5f5;
      color:#333;
    }
    .card{
      width:100%;
      max-width:460px;
      background:#fff;
      padding:24px;
      border-radius:16px;
      box-shadow:0 10px 30px rgba(0,0,0,0.15);
    }
    h2{margin-top:0;color:#b91c1c;}
    p{line-height:1.5;}
    ul{padding-left:20px;line-height:1.5;}
    a{
      display:block;
      margin-top:18px;
      padding:12px;
      border-radius:10px;
      background:orange;
      color:white;
      font-weight:bold;
      text-align:center;
      text-decoration:none;
    }
    .hint{
      font-size:14px;
      color:#666;
    }
  </style>
</head>
<body>
  <div class="card">
    <h2>Verbindung nicht möglich</h2>
    <p>Der ESP konnte sich innerhalb von 15 Sekunden nicht mit dem WLAN "%SSID%" verbinden.</p>
    <p>Mögliche Ursachen:</p>
    <ul>
      <li>Anmeldedaten falsch</li>
      <li>WLAN-Signal zu schwach</li>
      <li>Netzwerk nicht verfügbar</li>
      <li>WLAN nutzt nur 5 GHz statt 2.4 GHz</li>
    </ul>
    <p class="hint">Bitte prüfe SSID und Passwort und wiederhole den Anmeldevorgang.</p>
    <a href="/">WLAN erneut eingeben</a>
  </div>
</body>
</html>
)rawliteral";
