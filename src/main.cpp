#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <ESP32SPISlave.h>
#include <Preferences.h>

#define RANGE 4

ESP32SPISlave slave;
Preferences preferences;

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// --------------------------------------------------
// Einstellungen
// --------------------------------------------------

const char* AP_SSID = "Stellventil-Setup";
const char* AP_PASS = "12345678";   // mind. 8 Zeichen
const char* MDNS_NAME = "C7750";

bool wifiConnected = false;
bool restartPending = false;
unsigned long restartAt = 0;

// --------------------------------------------------
// DASHBOARD HTML
// --------------------------------------------------

static const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="de">
<head>
  <meta charset="UTF-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1.0" />
  <title>Stellventil</title>
  <style>
    :root {
      --background-color: #fff;
      --text-color: rgb(55, 55, 55);
      --heading-color: rgb(81, 81, 81);
      --card-color: rgb(235, 235, 235);
      --needle-color: rgb(245, 117, 117);
      --gauge-color: orange;
      --line-color: #eee;
      --container-color: #c7c7c7;
    }

    body {
      --s: 25px;
      --l1: #0000 48%, var(--line-color) 0 52%, #0000 0;
      --l2: #0000 1.3%, var(--background-color) 0 32%, #0000 0;
      background:
        linear-gradient(-45deg, var(--l2)),
        linear-gradient(45deg, var(--l1)),
        linear-gradient(45deg, var(--l2)) calc(var(--s)/2) calc(var(--s)/2),
        linear-gradient(-45deg, var(--l1)) var(--background-color);
      background-size: calc(2*var(--s)) var(--s), var(--s) var(--s);

      margin: 0;
      font-family: Arial, sans-serif;
      color: var(--text-color);
      display: flex;
      flex-direction: column;
      justify-content: center;
      align-items: center;
      height: 100vh;
      caret-color: transparent;
    }

    @media (min-width: 768px) {
      body {
        flex-direction: row;
      }
    }

    .card {
      background: var(--card-color);
      color: var(--text-color);
      box-sizing: border-box;
      border-radius: 20px;
      box-shadow: 0 10px 30px rgba(0,0,0,0.5);
      text-align: center;
      width: 400px;
      height: auto;
      position: relative;
      margin: 10px;
      border: 1px solid rgba(0,0,0,0.2);
    }

    h2 {
      color: var(--heading-color);
    }

    .gauge {
      position: relative;
      width: 280px;
      height: 280px;
      margin: 60px auto 0 auto;
      border-radius: 50%;
      background: conic-gradient(
        var(--gauge-color) 0deg 240deg,
        var(--card-color) 240deg 360deg
      );
      transform: rotate(-120deg);
      display: flex;
      justify-content: center;
      align-items: center;
    }

    .gauge::before {
      content: "";
      position: absolute;
      width: 260px;
      height: 260px;
      background: var(--card-color);
      border-radius: 50%;
    }

    .needle {
      position: absolute;
      width: 12px;
      height: 110px;
      background: var(--needle-color);
      clip-path: polygon(50% 0%, 0% 100%, 100% 100%);
      left: 50%;
      bottom: 50%;
      transform: translateX(-50%) rotate(0deg);
      transform-origin: bottom center;
      z-index: 2;
      transition: transform 0.5s ease;
    }

    .center-dot {
      position: absolute;
      width: 18px;
      height: 18px;
      background: var(--needle-color);
      border-radius: 50%;
      z-index: 3;
    }

    .value {
      position: relative;
      bottom: 20px;
      right: 55px;
      z-index: 4;
      font-size: 42px;
      font-weight: bold;
      margin-top: 10px;
      transform: rotate(120deg);
    }

    .label {
      position: relative;
      font-size: 25px;
      bottom: 80px;
      right: 90px;
      z-index: 4;
      color: #9ca3af;
      margin-top: 5px;
      transform: rotate(120deg);
    }

    .modeContainer {
      position: relative;
      margin: 0 30px 30px 30px;
      width: 330px;
      background-color: var(--container-color);
      padding: 5px;
      border-radius: 10px;
    }

    .value-shoulder-left {
      position: absolute;
      top: 40px;
      left: 40px;
      font-size: 18px;
      font-weight: bold;
    }

    .label2 {
      position: absolute;
      top: 60px;
      left: 40px;
      font-size: 14px;
    }

    .value-shoulder-right {
      position: absolute;
      top: 40px;
      right: 40px;
      font-size: 18px;
      font-weight: bold;
    }

    .label3 {
      position: absolute;
      top: 60px;
      right: 40px;
      font-size: 14px;
    }

    .leftshoulder {
      color: var(--container-color);
      width: 90px;
      position: absolute;
      left: 30px;
      top: 30px;
    }

    .rightshoulder {
      color: var(--container-color);
      width: 90px;
      position: absolute;
      right: 30px;
      top: 30px;
    }

    .chart {
      display: flex;
      justify-content: center;
      align-items: center;
      width: 350px;
      height: 250px;
      margin: 30px;
    }

    .logo {
      position: absolute;
      height: 65px;
      top: 5px;
      left: 5px;
    }
    
    .logobar {
      background-color: var(--card-color);
      position: absolute;
      top: 0;
      right: 0;
      width: 100%;
      height: 75px;
      user-select: none;
      border: 1px solid rgba(0,0,0,0.2);
      box-shadow: 0 5px 10px rgba(0,0,0,0.3);
      z-index: 10;
    }

  </style>
</head>
<body>
  <div class="logobar">
    <svg class="logo" xmlns="http://www.w3.org/2000/svg" id="uuid-de0efa03-f22a-4669-a768-7e1df74cd833" viewBox="0 0 1628 865"><defs><style>.uuid-a5a6110c-feb9-4fa0-a7e1-2d475c065a4b{stroke:#1d1d1b;stroke-miterlimit:10;stroke-width:.25px;fill:#1d1d1b}.uuid-e59c0813-d580-4185-9f05-e0fb25d0cb14{fill:#fff}.uuid-0bfd7670-24aa-41e3-894f-45a0a004ea7a{fill:#e00c19}.uuid-574c5493-fb09-47ac-90f2-a14b20e71e90{fill:none}</style></defs><path d="M0 .82h785.2V865H0z" class="uuid-0bfd7670-24aa-41e3-894f-45a0a004ea7a"/>
      <path d="M204.92 556.8v44.97H56.83V323.42h55.22V556.8zM401.01 381.3v221.05h-53.64v-20.57c-14.38 13.98-33.94 22.57-55.45 22.57-42.76 0-77.67-33.87-79.89-76.49a69 69 0 0 1-.13-4.32V381.31h52.99V525.6c0 21.97 18.47 39.79 41.24 39.79s37.19-17.05 41.24-39.79V381.31h53.64Z" class="uuid-e59c0813-d580-4185-9f05-e0fb25d0cb14"/><path d="M212.03 527.85h-.13v-4.32c0 1.44.03 2.89.13 4.32M495.63 550.34c0 3.34 2.72 6.06 6.08 6.06h28.33c.35 1.98.52 4 .52 6.08v39.29h-42.57v-.02c-.22.02-.44.02-.66.02-23.3 0-42.29-16.74-43.19-37.72-.03-.52-.05-1.05-.05-1.57V424.24h-30.12v-42.93h30.12v-57.88h51.54v57.88h34.93v42.93h-34.93z" class="uuid-e59c0813-d580-4185-9f05-e0fb25d0cb14"/><path d="M444.14 564.05h-.05v-1.57c0 .52.02 1.05.05 1.57M530.56 556.4v6.08c0-2.07-.17-4.1-.52-6.08zM696.7 298.58c-17.57 0-31.82 14.2-31.82 31.71s14.24 31.72 31.82 31.72 31.82-14.2 31.82-31.72-14.24-31.71-31.82-31.71m0 57.35c-14.21 0-25.72-11.48-25.72-25.64s11.51-25.64 25.72-25.64 25.74 11.48 25.74 25.64-11.53 25.64-25.74 25.64" class="uuid-e59c0813-d580-4185-9f05-e0fb25d0cb14"/><path d="M722.44 330.28c0 14.16-11.53 25.64-25.74 25.64s-25.72-11.48-25.72-25.64 11.51-25.64 25.72-25.64 25.74 11.48 25.74 25.64" class="uuid-574c5493-fb09-47ac-90f2-a14b20e71e90"/><path d="M722.44 330.28c0 14.16-11.53 25.64-25.74 25.64s-25.72-11.48-25.72-25.64 11.51-25.64 25.72-25.64 25.74 11.48 25.74 25.64" class="uuid-574c5493-fb09-47ac-90f2-a14b20e71e90"/><path d="M705.62 333.45v-1.12c4.48-1.12 7.8-5.37 7.8-10.44 0-5.95-4.57-10.77-10.19-10.77h-19.9v37.74h7.91v-15.41h5.78l7.59 15.41h8.87l-7.87-15.41Zm-14.37-6.15v-9.97h9.17c2.76 0 4.99 2.23 4.99 4.99s-2.23 4.98-4.99 4.98zM606.63 556.4h119.24v45.37H543.54v-44.25l1.04-1.12 123.1-132H544.45v-43.24h182.08v46.27l.2.18z" class="uuid-e59c0813-d580-4185-9f05-e0fb25d0cb14"/><path d="m544.58 556.4-1.04 1.12v-1.12zM543.54 557.52v.03l-.01-.02zM56.68 648.12h669.21v45.75H56.68zM95.53 735.88v13.13H83.64v51.81H68.26v-51.81H56.78v-13.13zM135.43 759.52v41.3h-14.04v-34.63c0-2.07-1.66-3.75-3.71-3.75-1.29 0-2.31.64-2.31.64s-1.3.88-1.31 1.53l-.08 36.21h-13.9v-64.53h13.9v19.5c1.82-2.87 5.68-4.87 10.16-4.87 5.78 0 10.54 3.31 11.18 7.57.06.33.09.68.09 1.02Z" class="uuid-e59c0813-d580-4185-9f05-e0fb25d0cb14"/><path d="M135.43 758.5v1.02q0-.525-.09-1.02zM141.46 791.33h-.03v-.71c0 .24.02.47.03.71" class="uuid-e59c0813-d580-4185-9f05-e0fb25d0cb14"/><path d="M155.14 778.61h22.46v-15.28c0-1.15-.22-2.28-.61-3.34-2.06-5.5-9.11-9.56-17.48-9.56-9.19 0-16.77 4.88-17.92 11.23-.11.55-.16 1.12-.16 1.68v27.29c0 .24.02.47.03.71.57 6.14 8.31 11.01 17.79 11.01s17.24-4.87 17.81-11.01c.02-.24.03-.47.03-.71v-6.69h-13.55s.04 5.05-.07 5.38c-.5 1.76-2.17 2.82-4.25 2.82-1.81 0-3.38-.99-4.08-2.4-.27-.52 0-11.12 0-11.12Zm0-14.16c0-2.26 2.01-4.1 4.49-4.1s4.49 1.84 4.49 4.1v5.1h-8.98z" class="uuid-e59c0813-d580-4185-9f05-e0fb25d0cb14"/><path d="M177.09 790.63v.71h-.03c.02-.24.03-.47.03-.71" class="uuid-e59c0813-d580-4185-9f05-e0fb25d0cb14"/><path d="M164.12 764.45v5.1h-8.98v-5.1c0-2.26 2.01-4.1 4.49-4.1s4.49 1.84 4.49 4.1" class="uuid-574c5493-fb09-47ac-90f2-a14b20e71e90"/><path d="M219.01 749.04v11.36h18.82v13.33h-18.82v26.9h-15.05v-64.35h35.74v12.76zM246.28 736.28h13.63v64.35h-13.63zM302.65 752.4v48.23h-13.96v-3.25c-2.1 3.02-5.64 4.99-9.63 4.99-6.03 0-11.01-4.51-11.61-10.3-.05-.38-.06-.77-.06-1.16V752.4h14.46v34.31c0 2.31 1.54 4.19 3.41 4.19s3.42-1.88 3.42-4.19V752.4h13.96Z" class="uuid-e59c0813-d580-4185-9f05-e0fb25d0cb14"/><path d="M267.45 792.07h-.06v-1.16c0 .39.02.79.06 1.16M310.57 735.95h14.42v11.64h-14.42zM310.57 752.41h14.42v48.22h-14.42zM353.59 736.28v20.09c-2.18-3.47-5.79-5.73-9.88-5.73-6.52 0-11.83 5.75-12.04 12.92-.02.16-.02.33-.02.49v24.58c0 .2 0 .39.02.6.28 7.11 5.56 12.81 12.04 12.81 4.08 0 7.69-2.26 9.88-5.73v4.33h13.73v-64.35h-13.73Zm-3.9 54.8c-2.17 0-3.91-2.86-3.91-6.39v-16.08c0-3.53 1.74-6.41 3.91-6.41s3.89 2.87 3.89 6.41v16.08c0 3.53-1.74 6.39-3.89 6.39" class="uuid-e59c0813-d580-4185-9f05-e0fb25d0cb14"/><path d="M331.67 763.55c-.02.16-.02.33-.02.49v-.49zM331.65 788.62c0 .2 0 .39.02.6h-.02zM452.49 735.9v64.81h-14.46l.99-48.63-1.18 6.51-8.18 42.1-12.34.02v-.11l-9.49-49 .57 49.11h-14.08v-64.43h21.83l7.34 39.55 7.3-39.93zM457.63 792.68c0 .11 0 .24.02.35h-.02zM457.66 783.83c-.02.22-.03.42-.03.64v-.64z" class="uuid-e59c0813-d580-4185-9f05-e0fb25d0cb14"/><path d="M494.66 761.83c-1.6-6.78-5.57-11.18-17.6-11.32-9.14-.11-17.33 4.62-18.85 11.48-.2.83-.3 1.68-.3 2.56 0 1.11.16 2.21.47 3.27h13.93v-2.92c0-2.72 1.95-4.92 4.35-4.92s4.35 2.2 4.35 4.92c0 2.37-1.46 4.33-3.41 4.82l-.06.02-13.66 5.56s.09.08.16.11c-3.61 1.68-6.12 4.81-6.38 8.43-.02.22-.03.42-.03.64v8.21c0 .11 0 .24.02.35.24 5.07 5.72 9.12 12.42 9.12 4.73 0 10.95-4.7 10.95-4.7h1.29v3.25H495v-36.17c0-.93-.11-1.84-.33-2.72Zm-19.04 30.02c-2.58.01-3.91-2.4-3.91-5.66 0 0 .02-2.92 1.92-4.67l3.43-2.14 3.95-1.8v10.6c-.11.16-1.15 3.65-5.39 3.67M577.96 761.77c-1.6-6.78-5.57-11.18-17.6-11.32-9.14-.11-17.33 4.62-18.85 11.48-.2.83-.3 1.68-.3 2.56 0 1.11.16 2.21.47 3.27h13.93v-2.92c0-2.72 1.95-4.92 4.35-4.92s4.35 2.2 4.35 4.92c0 2.37-1.46 4.33-3.41 4.82l-.06.02-13.66 5.56s.09.08.16.11c-3.61 1.68-6.12 4.81-6.38 8.43-.02.22-.03.42-.03.64v8.21c0 .11 0 .24.02.35.24 5.07 5.72 9.12 12.42 9.12 4.73 0 10.95-4.7 10.95-4.7h1.29v3.25h12.69v-36.17c0-.93-.11-1.84-.33-2.72Zm-19.03 30.02c-2.58.01-3.91-2.4-3.91-5.66 0 0 .02-2.92 1.92-4.67l3.43-2.14 3.95-1.8v10.6c-.11.16-1.15 3.65-5.39 3.67M638.92 778.5h22.46v-15.28c0-1.15-.22-2.28-.61-3.34-2.06-5.5-9.11-9.56-17.48-9.56-9.19 0-16.77 4.88-17.92 11.23-.11.55-.16 1.12-.16 1.68v27.29c0 .24.02.47.03.71.57 6.14 8.31 11.01 17.79 11.01s17.24-4.87 17.81-11.01c.02-.24.03-.47.03-.71v-6.69h-13.55s.04 5.05-.07 5.38c-.5 1.76-2.17 2.82-4.25 2.82-1.81 0-3.38-.99-4.08-2.4-.27-.52 0-11.12 0-11.12Zm0-14.16c0-2.26 2.01-4.1 4.49-4.1s4.49 1.84 4.49 4.1v5.1h-8.98zM535.39 800.57h-13.55l-.04-34.74c0-2.07-1.21-3.63-3.25-3.64-1.82-.01-3.58 1.14-3.59 3.24l-.1 35.14h-13.9l.08-47.89h13.9l-.08 2.86c1.82-2.87 5.69-5.02 10.16-4.87 5.87.2 9.79 3.32 10.43 7.57.06.33-.06 42.32-.06 42.32ZM606.25 752.25v4.27l-1.12.03c-2.01-3.53-5.64-5.89-9.8-5.89-5.95 0-10.06 4.48-11.43 10.3l-.23 26.19c0 1.02.13 2.03.36 2.97.71 2.97 2.45 5.51 4.84 7.19 1.92 1.37 4.24 2.17 6.75 2.17 3.88 0 7.3-1.87 9.47-4.84l.03 9.85c0 2.09-1.68 3.41-3.78 3.45-3.01.06-3.86-1.36-3.86-3.45v-1.19l-14.21.05c-.02.82 0 1.53 0 2.37 0 1.12.25 2.2.71 3.24 2.15 4.82 9 8.32 17.12 8.32 9.2 0 13.55-3.01 17.73-10.32.13-.22.11-.82.11-1.24v-53.46h-12.69Zm-1.12 32a3.826 3.826 0 0 1-7.65 0v-18.06c0-2.12 1.71-3.83 3.83-3.83s3.82 1.71 3.82 3.83zM690.36 751.16v15.37h-5.57c-2.43 0-4.4 1.96-4.4 4.4v29.85h-13.63V752.4h13.63v6.23c0-1.87.94-3.53 2.37-4.52l.05-.03.74-.42 4.4-2.5h2.42ZM682.76 754.12l-.08.04M698.82 778.24l3.83 1.9c-1.37-.49-2.65-1.13-3.83-1.9M727.04 765.92c0 .38-.02.74-.05 1.1v-2.2c.03.36.05.72.05 1.1M719.16 773.18l-2.48-1.07c.86.3 1.7.66 2.48 1.07M727 764.43v.39c-.02-.14-.02-.27-.03-.39zM712.19 784.88l-1.02-.5" class="uuid-e59c0813-d580-4185-9f05-e0fb25d0cb14"/><path d="M728.16 786.98c0 8.86-8.24 16.03-18.4 16.03-9.49 0-17.27-6.25-18.28-14.29v-3.16h13.63v3.16c.13 2.32 2.1 4.15 4.54 4.15s4.55-1.95 4.55-4.37c0-1.48-.75-2.78-1.93-3.58l-.08-.05c-.31-.2-.66-.38-1.02-.5l-8.51-4.24-3.83-1.9-.05-.03c-.93-.61-1.81-1.3-2.58-2.07-2.78-2.72-4.46-6.3-4.46-10.21 0-8.5 7.9-15.39 17.65-15.39 9.17 0 16.71 6.09 17.57 13.9.02.13.02.25.03.39v2.25h-13.3v-2.64c0-2.31-1.93-4.19-4.3-4.19s-4.29 1.88-4.29 4.19c0 1.96 1.37 3.6 3.2 4.05l.02.02 8.35 3.61 2.48 1.07.09.05c1.35.72 2.59 1.57 3.69 2.54 3.24 2.91 5.21 6.85 5.21 11.21ZM846.56 2.5h778.94v860H846.56z" class="uuid-e59c0813-d580-4185-9f05-e0fb25d0cb14"/><path d="M1623 5v855H849.06V5zm5-5H844.06v865H1628z" style="fill:#1d1d1b"/><path d="M1596.53 569.07c-9.47 0-17.15 7.65-17.15 17.08s7.68 17.09 17.15 17.09 17.15-7.65 17.15-17.09-7.68-17.08-17.15-17.08Zm0 30.9c-7.66 0-13.87-6.18-13.87-13.82s6.2-13.82 13.87-13.82 13.87 6.18 13.87 13.82-6.21 13.82-13.87 13.82Z" class="uuid-a5a6110c-feb9-4fa0-a7e1-2d475c065a4b"/><path d="M1601.34 587.86v-.6c2.41-.6 4.21-2.89 4.21-5.63 0-3.21-2.46-5.8-5.49-5.8h-10.73v20.33h4.27v-8.3h3.12l4.09 8.3h4.78l-4.24-8.3Zm-7.74-3.32v-5.37h4.94c1.49 0 2.69 1.2 2.69 2.69s-1.2 2.68-2.69 2.68zM995.47 383.16v207.63c0 6.94-5.61 12.55-12.55 12.55H859.67l31.12-59.33 45.13-.17c2.09-.02 4.13-1.55 4.12-3.71V383.15h55.43ZM1061.81 472.36c-.02.16-.03.32-.03.47s.02.32.03.47v21.01c0 2.59 2.1 4.68 4.68 4.68h54.44v36.51h-54.44a4.68 4.68 0 0 0-4.68 4.68v21.46c0 2.59 2.1 4.68 4.68 4.68h54.44v36.52h-99.65c-7.13 0-12.9-5.79-12.9-12.9V444.52c0-7.13 5.77-12.9 12.9-12.9h99.65v36.52h-54.49a4.65 4.65 0 0 0-4.64 4.19ZM1185.52 474.06v19.59c0 2.95 2.38 5.35 5.33 5.35h63.26c7.13 0 12.9 5.77 12.9 12.9v78.06c0 7.11-5.77 12.9-12.9 12.9h-121.68v-36.52h76.5c2.93 0 5.31-2.38 5.31-5.35v-20.14c0-2.95-2.38-5.35-5.31-5.35h-62.59c-7.13 0-12.9-5.77-12.9-12.9v-78.06c0-7.11 5.77-12.9 12.9-12.9h120.68v37.08h-76.16c-2.95 0-5.33 2.4-5.33 5.35ZM1334.77 473.51v87.48c0 2.96 2.38 5.36 5.33 5.36h55.8v36.51h-103.49c-7.13 0-12.9-5.79-12.9-12.9V444.54c0-7.11 5.77-12.9 12.9-12.9h103.49v36.52h-55.8c-2.95 0-5.33 2.38-5.33 5.35ZM1570.79 580.23v10.11c0 7.13-5.77 12.9-12.9 12.9h-136.81c-7.13 0-12.9-5.77-12.9-12.9v-145.8c0-7.11 5.77-12.9 12.9-12.9h88.76l-16.95 36.89h-26.4c-3.25 0-5.88 2.63-5.88 5.87v86.07c0 3.25 2.63 5.88 5.88 5.88h47.83c3.25 0 5.88-2.63 5.88-5.88v-22.71l47.53-101.29c.93 1.31 1.88 2.62 2.82 3.93.08 42 .14 83.98.22 125.96 0 4.62.02 9.26.02 13.88Z" class="uuid-a5a6110c-feb9-4fa0-a7e1-2d475c065a4b"/><path d="M1523.35 531.03h-59.88l-2.86 6.74h59.58zM1532.76 510.98h-59.49l-5.57 11.49h59.58zM1554.37 464.93h-59.83l-17.23 37.26h59.58zM1580.45 408.92h-59.9l-22.13 47.85H1558zM1612.7 340.73h-59.97l-27.77 60.03h59.59z" class="uuid-0bfd7670-24aa-41e3-894f-45a0a004ea7a"/>
    </svg>
  </div>

  <div class="card">
    <div>
      <svg class="leftshoulder" viewBox="0 0 70 70">
        <path d="M5.79 70.13h9.47a160.76 160.76 0 0 1 54.86-53.38V5.79c0-3.13-2.54-5.67-5.67-5.67H5.79C2.66.12.12 2.66.12 5.79v58.66c0 3.13 2.54 5.67 5.67 5.67Z" style="fill:currentColor;stroke:#868686;stroke-miterlimit:10;stroke-width:.25px"/>
      </svg>
      <div class="value-shoulder-left" id="leftvalue">--</div>
      <div class="label2">kg/h</div>
    </div>

    <div>
      <svg class="rightshoulder" viewBox="0 0 70 70">
        <path d="M64.46 70.12h-9.47A160.9 160.9 0 0 0 .12 16.74V5.79C.12 2.66 2.66.12 5.79.12h58.66c3.13 0 5.67 2.54 5.67 5.67v58.66c0 3.13-2.54 5.67-5.67 5.67Z" style="fill:currentColor;stroke:#868686;stroke-miterlimit:10;stroke-width:.25px"/>
      </svg>
      <div class="value-shoulder-right" id="rightvalue">--</div>
      <div class="label3">h min</div>
    </div>

    <div class="gauge">
      <div class="needle" id="needle"></div>
      <div class="center-dot"></div>
      <div>
        <div class="value" id="value">--</div>
        <div class="label">g/h</div>
      </div>
    </div>

    <div class="modeContainer">
      <h2 id="mode">--</h2>
    </div>
  </div>

  <div class="card">
    <div class="chart">
      <canvas id="flowChart"></canvas>
    </div>
  </div>

  <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
  <script>
    let websocket = null;
    let flowChart = null;

    function updateGauge(value) {
      const valueText = document.getElementById("value");
      const needle = document.getElementById("needle");

      valueText.textContent = value;

      const angle = (value / 50000) * 240;
      needle.style.transform = "translateX(-50%) rotate(" + angle + "deg)";
    }

    function initChart() {
      const ctx = document.getElementById("flowChart").getContext("2d");

      flowChart = new Chart(ctx, {
        type: "line",
        data: {
          labels: [],
          datasets: [{
            label: "Ventilfluss g/h",
            data: [],
            borderColor: "orange",
            borderWidth: 1,
            backgroundColor: "rgba(255,165,0,0.2)",
            tension: 0.3,
            fill: true,
            pointRadius: 1
          }]
        },
        options: {
          responsive: true,
          maintainAspectRatio: false,
          animation: false,
          plugins: {
            legend: {
              labels: {
                color: '#aaa'
              },
              display: false
            }
          },
          scales: {
            x: {
              ticks: {
                color: "#aaa"
              }
            },
            y: {
              min: 0,
              max: 50000,
              ticks: {
                color:"#aaa"
              },
              grid: {
                color: "rgba(150,150,150,0.15)"
              }
            }
          }
        }
      });
    }

    function addChartValue(value) {
      if (!flowChart) return;

      const now = new Date();
      const label =
        String(now.getHours()).padStart(2, "0") + ":" +
        String(now.getMinutes()).padStart(2, "0") + ":" +
        String(now.getSeconds()).padStart(2, "0");

      flowChart.data.labels.push(label);
      flowChart.data.datasets[0].data.push(value);

      const maxPoints = 1440;
      if (flowChart.data.labels.length > maxPoints) {
        flowChart.data.labels.shift();
        flowChart.data.datasets[0].data.shift();
      }

      flowChart.update();
    }

    function initWebSocket() {
      const gateway = "ws://" + window.location.host + "/ws";
      console.log("Verbinde zu:", gateway);

      websocket = new WebSocket(gateway);

      websocket.onopen = function () {
        console.log("WebSocket verbunden");
      };

      websocket.onclose = function (event) {
        console.log("WebSocket getrennt", event);
        setTimeout(initWebSocket, 2000);
      };

      websocket.onerror = function (error) {
        console.error("WebSocket Fehler:", error);
      };

      websocket.onmessage = function (event) {
        console.log("Empfangen:", event.data);

        const parts = event.data.split("|");
        if (parts.length < 5) {
          console.warn("Ungültiges Format:", event.data);
          return;
        }

        const valveFlow = parseInt(parts[0], 10);
        const leftVal = parts[1];
        const timeText = parts[2];
        const modeText = parts[3];
        const rssi = parts[4];

        updateGauge(valveFlow);
        addChartValue(valveFlow);

        document.getElementById("leftvalue").textContent = leftVal;
        document.getElementById("rightvalue").textContent = timeText;
        document.getElementById("mode").textContent = modeText;

        console.log("RSSI:", rssi);
      };
    }

    window.addEventListener("load", function () {
      initChart();
      initWebSocket();
    });
  </script>
</body>
</html>
)rawliteral";

// --------------------------------------------------
// Setup-Portal HTML
// --------------------------------------------------

static const char CONFIG_HTML[] PROGMEM = R"rawliteral(
<html lang="de">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <title>WLAN Setup</title>
  <style>
    body{
      --s: 25px;
      --l1: #0000 48%, #eee 0 52%, #0000 0;
      --l2: #0000 1.3%, #fff 0 32%, #0000 0;
      background:
          linear-gradient(-45deg, var(--l2)),
          linear-gradient(45deg, var(--l1)),
          linear-gradient(45deg, var(--l2)) calc(var(--s)/2) calc(var(--s)/2),
          linear-gradient(-45deg, var(--l1)) #fff;
      background-size: calc(2*var(--s)) var(--s), var(--s) var(--s);
      font-family:Arial,sans-serif;
      color: rgb(81,81,81);
      margin:0;
      display:flex;
      align-items:center;
      justify-content:center;
      min-height:100vh;
    }
    .card{
      width:100%;
      max-width:400px;
      background:rgb(235,235,235);
      padding:24px;
      border-radius:16px;
      box-shadow:0 10px 30px rgba(0,0,0,0.5);
    }
    h1{margin-top:0;font-size:24px;}
    p{color:#555;}
    label{display:block;margin-top:14px;margin-bottom:6px;font-weight:bold;}
    input{
      width:100%;
      box-sizing:border-box;
      padding:12px;
      border:1px solid #ccc;
      border-radius:10px;
      font-size:16px;
    }
    button{
      margin-top:18px;
      width:100%;
      padding:12px;
      border:none;
      border-radius:10px;
      background:orange;
      color:white;
      font-size:16px;
      font-weight: bold;
      cursor:pointer;
    }
    
    .logo {
      position: absolute;
      height: 65px;
      top: 5px;
      left: 5px;
    }

    .logobar {
      background-color: rgb(235,235,235);
      position: absolute;
      top: 0;
      right: 0;
      width: 100%;
      height: 75px;
      user-select: none;
      box-shadow: 0 5px 10px rgba(0,0,0,0.3);
      z-index: 10;
    }
  </style>
</head>
<body>
  <div class="logobar">
    <svg class="logo" xmlns="http://www.w3.org/2000/svg" id="uuid-de0efa03-f22a-4669-a768-7e1df74cd833" viewBox="0 0 1628 865"><defs><style>.uuid-a5a6110c-feb9-4fa0-a7e1-2d475c065a4b{stroke:#1d1d1b;stroke-miterlimit:10;stroke-width:.25px;fill:#1d1d1b}.uuid-e59c0813-d580-4185-9f05-e0fb25d0cb14{fill:#fff}.uuid-0bfd7670-24aa-41e3-894f-45a0a004ea7a{fill:#e00c19}.uuid-574c5493-fb09-47ac-90f2-a14b20e71e90{fill:none}</style></defs><path d="M0 .82h785.2V865H0z" class="uuid-0bfd7670-24aa-41e3-894f-45a0a004ea7a"/>
      <path d="M204.92 556.8v44.97H56.83V323.42h55.22V556.8zM401.01 381.3v221.05h-53.64v-20.57c-14.38 13.98-33.94 22.57-55.45 22.57-42.76 0-77.67-33.87-79.89-76.49a69 69 0 0 1-.13-4.32V381.31h52.99V525.6c0 21.97 18.47 39.79 41.24 39.79s37.19-17.05 41.24-39.79V381.31h53.64Z" class="uuid-e59c0813-d580-4185-9f05-e0fb25d0cb14"/><path d="M212.03 527.85h-.13v-4.32c0 1.44.03 2.89.13 4.32M495.63 550.34c0 3.34 2.72 6.06 6.08 6.06h28.33c.35 1.98.52 4 .52 6.08v39.29h-42.57v-.02c-.22.02-.44.02-.66.02-23.3 0-42.29-16.74-43.19-37.72-.03-.52-.05-1.05-.05-1.57V424.24h-30.12v-42.93h30.12v-57.88h51.54v57.88h34.93v42.93h-34.93z" class="uuid-e59c0813-d580-4185-9f05-e0fb25d0cb14"/><path d="M444.14 564.05h-.05v-1.57c0 .52.02 1.05.05 1.57M530.56 556.4v6.08c0-2.07-.17-4.1-.52-6.08zM696.7 298.58c-17.57 0-31.82 14.2-31.82 31.71s14.24 31.72 31.82 31.72 31.82-14.2 31.82-31.72-14.24-31.71-31.82-31.71m0 57.35c-14.21 0-25.72-11.48-25.72-25.64s11.51-25.64 25.72-25.64 25.74 11.48 25.74 25.64-11.53 25.64-25.74 25.64" class="uuid-e59c0813-d580-4185-9f05-e0fb25d0cb14"/><path d="M722.44 330.28c0 14.16-11.53 25.64-25.74 25.64s-25.72-11.48-25.72-25.64 11.51-25.64 25.72-25.64 25.74 11.48 25.74 25.64" class="uuid-574c5493-fb09-47ac-90f2-a14b20e71e90"/><path d="M722.44 330.28c0 14.16-11.53 25.64-25.74 25.64s-25.72-11.48-25.72-25.64 11.51-25.64 25.72-25.64 25.74 11.48 25.74 25.64" class="uuid-574c5493-fb09-47ac-90f2-a14b20e71e90"/><path d="M705.62 333.45v-1.12c4.48-1.12 7.8-5.37 7.8-10.44 0-5.95-4.57-10.77-10.19-10.77h-19.9v37.74h7.91v-15.41h5.78l7.59 15.41h8.87l-7.87-15.41Zm-14.37-6.15v-9.97h9.17c2.76 0 4.99 2.23 4.99 4.99s-2.23 4.98-4.99 4.98zM606.63 556.4h119.24v45.37H543.54v-44.25l1.04-1.12 123.1-132H544.45v-43.24h182.08v46.27l.2.18z" class="uuid-e59c0813-d580-4185-9f05-e0fb25d0cb14"/><path d="m544.58 556.4-1.04 1.12v-1.12zM543.54 557.52v.03l-.01-.02zM56.68 648.12h669.21v45.75H56.68zM95.53 735.88v13.13H83.64v51.81H68.26v-51.81H56.78v-13.13zM135.43 759.52v41.3h-14.04v-34.63c0-2.07-1.66-3.75-3.71-3.75-1.29 0-2.31.64-2.31.64s-1.3.88-1.31 1.53l-.08 36.21h-13.9v-64.53h13.9v19.5c1.82-2.87 5.68-4.87 10.16-4.87 5.78 0 10.54 3.31 11.18 7.57.06.33.09.68.09 1.02Z" class="uuid-e59c0813-d580-4185-9f05-e0fb25d0cb14"/><path d="M135.43 758.5v1.02q0-.525-.09-1.02zM141.46 791.33h-.03v-.71c0 .24.02.47.03.71" class="uuid-e59c0813-d580-4185-9f05-e0fb25d0cb14"/><path d="M155.14 778.61h22.46v-15.28c0-1.15-.22-2.28-.61-3.34-2.06-5.5-9.11-9.56-17.48-9.56-9.19 0-16.77 4.88-17.92 11.23-.11.55-.16 1.12-.16 1.68v27.29c0 .24.02.47.03.71.57 6.14 8.31 11.01 17.79 11.01s17.24-4.87 17.81-11.01c.02-.24.03-.47.03-.71v-6.69h-13.55s.04 5.05-.07 5.38c-.5 1.76-2.17 2.82-4.25 2.82-1.81 0-3.38-.99-4.08-2.4-.27-.52 0-11.12 0-11.12Zm0-14.16c0-2.26 2.01-4.1 4.49-4.1s4.49 1.84 4.49 4.1v5.1h-8.98z" class="uuid-e59c0813-d580-4185-9f05-e0fb25d0cb14"/><path d="M177.09 790.63v.71h-.03c.02-.24.03-.47.03-.71" class="uuid-e59c0813-d580-4185-9f05-e0fb25d0cb14"/><path d="M164.12 764.45v5.1h-8.98v-5.1c0-2.26 2.01-4.1 4.49-4.1s4.49 1.84 4.49 4.1" class="uuid-574c5493-fb09-47ac-90f2-a14b20e71e90"/><path d="M219.01 749.04v11.36h18.82v13.33h-18.82v26.9h-15.05v-64.35h35.74v12.76zM246.28 736.28h13.63v64.35h-13.63zM302.65 752.4v48.23h-13.96v-3.25c-2.1 3.02-5.64 4.99-9.63 4.99-6.03 0-11.01-4.51-11.61-10.3-.05-.38-.06-.77-.06-1.16V752.4h14.46v34.31c0 2.31 1.54 4.19 3.41 4.19s3.42-1.88 3.42-4.19V752.4h13.96Z" class="uuid-e59c0813-d580-4185-9f05-e0fb25d0cb14"/><path d="M267.45 792.07h-.06v-1.16c0 .39.02.79.06 1.16M310.57 735.95h14.42v11.64h-14.42zM310.57 752.41h14.42v48.22h-14.42zM353.59 736.28v20.09c-2.18-3.47-5.79-5.73-9.88-5.73-6.52 0-11.83 5.75-12.04 12.92-.02.16-.02.33-.02.49v24.58c0 .2 0 .39.02.6.28 7.11 5.56 12.81 12.04 12.81 4.08 0 7.69-2.26 9.88-5.73v4.33h13.73v-64.35h-13.73Zm-3.9 54.8c-2.17 0-3.91-2.86-3.91-6.39v-16.08c0-3.53 1.74-6.41 3.91-6.41s3.89 2.87 3.89 6.41v16.08c0 3.53-1.74 6.39-3.89 6.39" class="uuid-e59c0813-d580-4185-9f05-e0fb25d0cb14"/><path d="M331.67 763.55c-.02.16-.02.33-.02.49v-.49zM331.65 788.62c0 .2 0 .39.02.6h-.02zM452.49 735.9v64.81h-14.46l.99-48.63-1.18 6.51-8.18 42.1-12.34.02v-.11l-9.49-49 .57 49.11h-14.08v-64.43h21.83l7.34 39.55 7.3-39.93zM457.63 792.68c0 .11 0 .24.02.35h-.02zM457.66 783.83c-.02.22-.03.42-.03.64v-.64z" class="uuid-e59c0813-d580-4185-9f05-e0fb25d0cb14"/><path d="M494.66 761.83c-1.6-6.78-5.57-11.18-17.6-11.32-9.14-.11-17.33 4.62-18.85 11.48-.2.83-.3 1.68-.3 2.56 0 1.11.16 2.21.47 3.27h13.93v-2.92c0-2.72 1.95-4.92 4.35-4.92s4.35 2.2 4.35 4.92c0 2.37-1.46 4.33-3.41 4.82l-.06.02-13.66 5.56s.09.08.16.11c-3.61 1.68-6.12 4.81-6.38 8.43-.02.22-.03.42-.03.64v8.21c0 .11 0 .24.02.35.24 5.07 5.72 9.12 12.42 9.12 4.73 0 10.95-4.7 10.95-4.7h1.29v3.25H495v-36.17c0-.93-.11-1.84-.33-2.72Zm-19.04 30.02c-2.58.01-3.91-2.4-3.91-5.66 0 0 .02-2.92 1.92-4.67l3.43-2.14 3.95-1.8v10.6c-.11.16-1.15 3.65-5.39 3.67M577.96 761.77c-1.6-6.78-5.57-11.18-17.6-11.32-9.14-.11-17.33 4.62-18.85 11.48-.2.83-.3 1.68-.3 2.56 0 1.11.16 2.21.47 3.27h13.93v-2.92c0-2.72 1.95-4.92 4.35-4.92s4.35 2.2 4.35 4.92c0 2.37-1.46 4.33-3.41 4.82l-.06.02-13.66 5.56s.09.08.16.11c-3.61 1.68-6.12 4.81-6.38 8.43-.02.22-.03.42-.03.64v8.21c0 .11 0 .24.02.35.24 5.07 5.72 9.12 12.42 9.12 4.73 0 10.95-4.7 10.95-4.7h1.29v3.25h12.69v-36.17c0-.93-.11-1.84-.33-2.72Zm-19.03 30.02c-2.58.01-3.91-2.4-3.91-5.66 0 0 .02-2.92 1.92-4.67l3.43-2.14 3.95-1.8v10.6c-.11.16-1.15 3.65-5.39 3.67M638.92 778.5h22.46v-15.28c0-1.15-.22-2.28-.61-3.34-2.06-5.5-9.11-9.56-17.48-9.56-9.19 0-16.77 4.88-17.92 11.23-.11.55-.16 1.12-.16 1.68v27.29c0 .24.02.47.03.71.57 6.14 8.31 11.01 17.79 11.01s17.24-4.87 17.81-11.01c.02-.24.03-.47.03-.71v-6.69h-13.55s.04 5.05-.07 5.38c-.5 1.76-2.17 2.82-4.25 2.82-1.81 0-3.38-.99-4.08-2.4-.27-.52 0-11.12 0-11.12Zm0-14.16c0-2.26 2.01-4.1 4.49-4.1s4.49 1.84 4.49 4.1v5.1h-8.98zM535.39 800.57h-13.55l-.04-34.74c0-2.07-1.21-3.63-3.25-3.64-1.82-.01-3.58 1.14-3.59 3.24l-.1 35.14h-13.9l.08-47.89h13.9l-.08 2.86c1.82-2.87 5.69-5.02 10.16-4.87 5.87.2 9.79 3.32 10.43 7.57.06.33-.06 42.32-.06 42.32ZM606.25 752.25v4.27l-1.12.03c-2.01-3.53-5.64-5.89-9.8-5.89-5.95 0-10.06 4.48-11.43 10.3l-.23 26.19c0 1.02.13 2.03.36 2.97.71 2.97 2.45 5.51 4.84 7.19 1.92 1.37 4.24 2.17 6.75 2.17 3.88 0 7.3-1.87 9.47-4.84l.03 9.85c0 2.09-1.68 3.41-3.78 3.45-3.01.06-3.86-1.36-3.86-3.45v-1.19l-14.21.05c-.02.82 0 1.53 0 2.37 0 1.12.25 2.2.71 3.24 2.15 4.82 9 8.32 17.12 8.32 9.2 0 13.55-3.01 17.73-10.32.13-.22.11-.82.11-1.24v-53.46h-12.69Zm-1.12 32a3.826 3.826 0 0 1-7.65 0v-18.06c0-2.12 1.71-3.83 3.83-3.83s3.82 1.71 3.82 3.83zM690.36 751.16v15.37h-5.57c-2.43 0-4.4 1.96-4.4 4.4v29.85h-13.63V752.4h13.63v6.23c0-1.87.94-3.53 2.37-4.52l.05-.03.74-.42 4.4-2.5h2.42ZM682.76 754.12l-.08.04M698.82 778.24l3.83 1.9c-1.37-.49-2.65-1.13-3.83-1.9M727.04 765.92c0 .38-.02.74-.05 1.1v-2.2c.03.36.05.72.05 1.1M719.16 773.18l-2.48-1.07c.86.3 1.7.66 2.48 1.07M727 764.43v.39c-.02-.14-.02-.27-.03-.39zM712.19 784.88l-1.02-.5" class="uuid-e59c0813-d580-4185-9f05-e0fb25d0cb14"/><path d="M728.16 786.98c0 8.86-8.24 16.03-18.4 16.03-9.49 0-17.27-6.25-18.28-14.29v-3.16h13.63v3.16c.13 2.32 2.1 4.15 4.54 4.15s4.55-1.95 4.55-4.37c0-1.48-.75-2.78-1.93-3.58l-.08-.05c-.31-.2-.66-.38-1.02-.5l-8.51-4.24-3.83-1.9-.05-.03c-.93-.61-1.81-1.3-2.58-2.07-2.78-2.72-4.46-6.3-4.46-10.21 0-8.5 7.9-15.39 17.65-15.39 9.17 0 16.71 6.09 17.57 13.9.02.13.02.25.03.39v2.25h-13.3v-2.64c0-2.31-1.93-4.19-4.3-4.19s-4.29 1.88-4.29 4.19c0 1.96 1.37 3.6 3.2 4.05l.02.02 8.35 3.61 2.48 1.07.09.05c1.35.72 2.59 1.57 3.69 2.54 3.24 2.91 5.21 6.85 5.21 11.21ZM846.56 2.5h778.94v860H846.56z" class="uuid-e59c0813-d580-4185-9f05-e0fb25d0cb14"/><path d="M1623 5v855H849.06V5zm5-5H844.06v865H1628z" style="fill:#1d1d1b"/><path d="M1596.53 569.07c-9.47 0-17.15 7.65-17.15 17.08s7.68 17.09 17.15 17.09 17.15-7.65 17.15-17.09-7.68-17.08-17.15-17.08Zm0 30.9c-7.66 0-13.87-6.18-13.87-13.82s6.2-13.82 13.87-13.82 13.87 6.18 13.87 13.82-6.21 13.82-13.87 13.82Z" class="uuid-a5a6110c-feb9-4fa0-a7e1-2d475c065a4b"/><path d="M1601.34 587.86v-.6c2.41-.6 4.21-2.89 4.21-5.63 0-3.21-2.46-5.8-5.49-5.8h-10.73v20.33h4.27v-8.3h3.12l4.09 8.3h4.78l-4.24-8.3Zm-7.74-3.32v-5.37h4.94c1.49 0 2.69 1.2 2.69 2.69s-1.2 2.68-2.69 2.68zM995.47 383.16v207.63c0 6.94-5.61 12.55-12.55 12.55H859.67l31.12-59.33 45.13-.17c2.09-.02 4.13-1.55 4.12-3.71V383.15h55.43ZM1061.81 472.36c-.02.16-.03.32-.03.47s.02.32.03.47v21.01c0 2.59 2.1 4.68 4.68 4.68h54.44v36.51h-54.44a4.68 4.68 0 0 0-4.68 4.68v21.46c0 2.59 2.1 4.68 4.68 4.68h54.44v36.52h-99.65c-7.13 0-12.9-5.79-12.9-12.9V444.52c0-7.13 5.77-12.9 12.9-12.9h99.65v36.52h-54.49a4.65 4.65 0 0 0-4.64 4.19ZM1185.52 474.06v19.59c0 2.95 2.38 5.35 5.33 5.35h63.26c7.13 0 12.9 5.77 12.9 12.9v78.06c0 7.11-5.77 12.9-12.9 12.9h-121.68v-36.52h76.5c2.93 0 5.31-2.38 5.31-5.35v-20.14c0-2.95-2.38-5.35-5.31-5.35h-62.59c-7.13 0-12.9-5.77-12.9-12.9v-78.06c0-7.11 5.77-12.9 12.9-12.9h120.68v37.08h-76.16c-2.95 0-5.33 2.4-5.33 5.35ZM1334.77 473.51v87.48c0 2.96 2.38 5.36 5.33 5.36h55.8v36.51h-103.49c-7.13 0-12.9-5.79-12.9-12.9V444.54c0-7.11 5.77-12.9 12.9-12.9h103.49v36.52h-55.8c-2.95 0-5.33 2.38-5.33 5.35ZM1570.79 580.23v10.11c0 7.13-5.77 12.9-12.9 12.9h-136.81c-7.13 0-12.9-5.77-12.9-12.9v-145.8c0-7.11 5.77-12.9 12.9-12.9h88.76l-16.95 36.89h-26.4c-3.25 0-5.88 2.63-5.88 5.87v86.07c0 3.25 2.63 5.88 5.88 5.88h47.83c3.25 0 5.88-2.63 5.88-5.88v-22.71l47.53-101.29c.93 1.31 1.88 2.62 2.82 3.93.08 42 .14 83.98.22 125.96 0 4.62.02 9.26.02 13.88Z" class="uuid-a5a6110c-feb9-4fa0-a7e1-2d475c065a4b"/><path d="M1523.35 531.03h-59.88l-2.86 6.74h59.58zM1532.76 510.98h-59.49l-5.57 11.49h59.58zM1554.37 464.93h-59.83l-17.23 37.26h59.58zM1580.45 408.92h-59.9l-22.13 47.85H1558zM1612.7 340.73h-59.97l-27.77 60.03h59.59z" class="uuid-0bfd7670-24aa-41e3-894f-45a0a004ea7a"/>
    </svg>
  </div>

  <div class="card">
    <h1>WLAN konfigurieren</h1>
    <p>Bitte SSID und Passwort deines WLAN eingeben.</p>
    <form action="/save" method="POST">
      <label for="ssid">SSID:</label>
      <input id="ssid" name="ssid" type="text" required>

      <label for="password">Passwort:</label>
      <input id="password" name="password" type="password">

      <button type="submit">Speichern & Neustarten</button>
    </form>
  </div>
</body>
</html>
)rawliteral";

// --------------------------------------------------
// Seite nach Speichern mit Auto-Weiterleitung
// --------------------------------------------------

static const char SAVED_HTML[] PROGMEM = R"rawliteral(
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
    .hint{
      font-size:14px;
      color:#666;
      margin-top:12px;
    }
    a{
      color:#d97706;
      font-weight:bold;
      text-decoration:none;
    }
  </style>
</head>
<body>
  <div class="card">
    <h2>WLAN gespeichert</h2>
    <p>Der ESP32 startet jetzt neu und verbindet sich mit deinem WLAN.</p>
    <p>Danach wird versucht, das Dashboard zu öffnen:</p>
    <p><a href="http://C7750.local">http://C7750.local</a></p>
    <p class="hint">
      Falls es nicht automatisch klappt, bitte nach ein paar Sekunden manuell öffnen.
    </p>
  </div>

  <script>
    setTimeout(() => {
      window.location.href = "http://C7701.local";
    }, 8000);
  </script>
</body>
</html>
)rawliteral";

// --------------------------------------------------
// Preferences / WLAN-Daten
// --------------------------------------------------
String loadSSID() {
  preferences.begin("wifi", true);
  String ssid = preferences.getString("ssid", "");
  preferences.end();
  return ssid;
}

String loadPassword() {
  preferences.begin("wifi", true);
  String pass = preferences.getString("pass", "");
  preferences.end();
  return pass;
}

void saveCredentials(const String& ssid, const String& pass) {
  preferences.begin("wifi", false);
  preferences.putString("ssid", ssid);
  preferences.putString("pass", pass);
  preferences.end();
}

void clearCredentials() {
  preferences.begin("wifi", false);
  preferences.clear();
  preferences.end();
}

// --------------------------------------------------
// WLAN verbinden
// --------------------------------------------------

bool connectToSavedWifi() {
  String ssid = loadSSID();
  String pass = loadPassword();

  if (ssid.isEmpty()) {
    Serial.println("Keine gespeicherten WLAN-Daten gefunden.");
    return false;
  }

  Serial.print("Verbinde mit gespeichertem WLAN: ");
  Serial.println(ssid);
  Serial.println(pass);

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.begin(ssid.c_str(), pass.c_str());

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 20000) {
    Serial.print(".");
    Serial.println(WiFi.RSSI()); // Marker
    delay(500);
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WLAN verbunden.");
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    return true;
  }

  Serial.println("Verbindung fehlgeschlagen.");
  return false;
}

// --------------------------------------------------
// Access Point starten
// --------------------------------------------------

void startConfigAP() {
  wifiConnected = false;

  WiFi.disconnect(true, true);
  delay(300);

  WiFi.mode(WIFI_AP);
  delay(500);

  bool ok = WiFi.softAP(AP_SSID, AP_PASS, 6, 0, 4);

  if (ok) {
    Serial.println("Setup-AP gestartet.");
    Serial.print("AP SSID: ");
    Serial.println(WiFi.softAPSSID());
    Serial.print("AP IP: ");
    Serial.println(WiFi.softAPIP());
  } else {
    Serial.println("Fehler beim Starten des AP.");
  }
}

// --------------------------------------------------
// mDNS
// --------------------------------------------------

void setupMdns() {
  if (!wifiConnected) return;

  if (MDNS.begin(MDNS_NAME)) {
    Serial.println("mDNS responder started");
    MDNS.addService("http", "tcp", 80);
    Serial.print("Dashboard erreichbar unter: http://");
    Serial.print(MDNS_NAME);
    Serial.println(".local");
  } else {
    Serial.println("mDNS failed");
  }
}

// --------------------------------------------------
// WebSocket
// --------------------------------------------------

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
               AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    Serial.printf("WS Client connected: id=%u ip=%s\n",
                  client->id(),
                  client->remoteIP().toString().c_str());

    client->text("12345|22|03:15|Automatik|-61");
  } else if (type == WS_EVT_DISCONNECT) {
    Serial.printf("WS Client disconnected: id=%u\n", client->id());
  } else if (type == WS_EVT_ERROR) {
    Serial.printf("WS Client error: id=%u\n", client->id());
  }
}

// --------------------------------------------------
// Webserver
// --------------------------------------------------

void setupWebServer() {
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (wifiConnected) {
      request->send_P(200, "text/html; charset=utf-8", INDEX_HTML);
    } else {
      request->send_P(200, "text/html; charset=utf-8", CONFIG_HTML);
    }
  });

  server.on("/save", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (!request->hasParam("ssid", true)) {
      request->send(400, "text/plain", "SSID fehlt");
      return;
    }

    String ssid = request->getParam("ssid", true)->value();
    String pass = "";

    if (request->hasParam("password", true)) {
      pass = request->getParam("password", true)->value();
    }

    saveCredentials(ssid, pass);

    Serial.println("Neue WLAN-Daten gespeichert.");
    Serial.println("Neustart wird vorbereitet...");

    request->send_P(200, "text/html; charset=utf-8", SAVED_HTML);

    restartPending = true;
    restartAt = millis() + 2000;
  });

  server.on("/reset", HTTP_GET, [](AsyncWebServerRequest *request) {
    clearCredentials();
    request->send(200, "text/plain", "Gespeicherte WLAN-Daten geloescht. Neustart...");
    delay(1000);
    ESP.restart();
  });

  server.begin();
  Serial.println("HTTP/WebSocket server started.");
}

// --------------------------------------------------
// SPI
// --------------------------------------------------

void setupSpi() {
  slave.setDataMode(SPI_MODE0);
  slave.setQueueSize(1);
  slave.begin(VSPI, 18, 19, 23, 5);
  Serial.println("ESP32 SPI Slave ready");
}

// --------------------------------------------------
// Setup
// --------------------------------------------------

void setup() {
  Serial.begin(115200);
  delay(200);
  randomSeed(micros());

  setupSpi();

  wifiConnected = connectToSavedWifi();

  if (!wifiConnected) {
    startConfigAP();
  } else {
    setupMdns();
  }

  setupWebServer();
}

// --------------------------------------------------
// Loop
// --------------------------------------------------

void loop() {
  if (restartPending && millis() >= restartAt) {
    Serial.println("ESP startet jetzt neu...");
    delay(200);
    ESP.restart();
  }

  static unsigned long lastSend = 0;

  if (wifiConnected && millis() - lastSend >= 500) {
    lastSend = millis();

    int valveFlow = random(0, 50001);
    int leftValue = random(0, 50001);
    int hours = random(0, 24);
    int minutes = random(0, 60);

    String mode;
    int modeRand = random(0, 3);
    if (modeRand == 0) mode = "Automatik";
    else if (modeRand == 1) mode = "Manuell";
    else mode = "Standby";

    String timeText =
      String(hours < 10 ? "0" : "") + String(hours) + ":" +
      String(minutes < 10 ? "0" : "") + String(minutes);

    String msg = String(valveFlow) + "|" +
                 String(leftValue) + "|" +
                 timeText + "|" +
                 mode + "|" +
                 String(WiFi.RSSI());

    Serial.print("Sende WS: ");
    Serial.println(msg);

    ws.textAll(msg);
    ws.cleanupClients();
  }

  /*
  uint8_t rxBuf[RANGE] = {0};
  uint8_t txBuf[RANGE] = {0x05};

  size_t received = slave.transfer(txBuf, rxBuf, sizeof(rxBuf));

  if (received > 0) {
    uint32_t valveFlow = 0;
    for (int i = 0; i < received; i++) {
      valveFlow = (valveFlow << 8) | rxBuf[i];
    }

    String msg = String(valveFlow) + "|" + String(WiFi.RSSI());
    ws.textAll(msg);
    ws.cleanupClients();

    Serial.print("Valve flow: ");
    Serial.print(valveFlow);
    Serial.println(" g/h");
    Serial.println(received);
  }
  */
}
