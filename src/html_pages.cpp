#include "html_pages.h"

#include <WiFi.h>

#include "device_identity.h"

String buildIndexHtml() {
  String html = FPSTR(INDEX_HTML);
  html.replace("%DEVICE_NAME%", getDeviceName());
  html.replace("%DEVICE_IP%", WiFi.localIP().toString());
  return html;
}

String buildSavedHtml(const String& dashboardUrl, unsigned long redirectDelayMs) {
  String html = FPSTR(SAVED_HTML);
  String mdnsUrl = String("http://") + getDeviceMdnsName() + ".local";
  html.replace("%DEVICE_NAME%", getDeviceName());
  html.replace("%MDNS_LINK%", String("Optionaler lokaler Name: <a href=\"") + mdnsUrl + "\">" + mdnsUrl + "</a>");

  if (dashboardUrl.isEmpty()) {
    html.replace("%DASHBOARD_LINK%", "Router-IP noch nicht verfuegbar. Bitte die IP nach dem Neustart im Router pruefen.");
    html.replace("%DASHBOARD_URL%", "#");
    html.replace("%REDIRECT_SECONDS%", "0");
    html.replace("%REDIRECT_DELAY%", "0");
  } else {
    String redirectDelaySeconds = String(redirectDelayMs / 1000);
    html.replace("%DASHBOARD_LINK%", String("<a href=\"") + dashboardUrl + "\">" + dashboardUrl + "</a>");
    html.replace("%DASHBOARD_URL%", dashboardUrl);
    html.replace("%REDIRECT_SECONDS%", redirectDelaySeconds);
    html.replace("%REDIRECT_DELAY%", String(redirectDelayMs));
  }
  return html;
}

String buildWifiConnectWaitHtml(const String& ssid) {
  String html = FPSTR(WIFI_CONNECT_WAIT_HTML);
  String escapedSsid = ssid;
  escapedSsid.replace("&", "&amp;");
  escapedSsid.replace("<", "&lt;");
  escapedSsid.replace(">", "&gt;");
  escapedSsid.replace("\"", "&quot;");
  escapedSsid.replace("'", "&#39;");
  html.replace("%SSID%", escapedSsid);
  return html;
}

String buildWifiConnectFailedHtml(const String& ssid) {
  String html = FPSTR(WIFI_CONNECT_FAILED_HTML);
  String escapedSsid = ssid;
  escapedSsid.replace("&", "&amp;");
  escapedSsid.replace("<", "&lt;");
  escapedSsid.replace(">", "&gt;");
  escapedSsid.replace("\"", "&quot;");
  escapedSsid.replace("'", "&#39;");
  html.replace("%SSID%", escapedSsid);
  return html;
}
