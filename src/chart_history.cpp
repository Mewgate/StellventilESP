#include "chart_history.h"

#include <time.h>

static const size_t CHART_HISTORY_SIZE = 5760;  // 24h at one value every 15 seconds
static const size_t CHART_HISTORY_BATCH_SIZE = 180;
static const unsigned long CHART_HISTORY_SAMPLE_INTERVAL_MS = 15000;

struct ChartHistoryPoint {
  uint16_t value;
  char label[6];
};

static ChartHistoryPoint chartHistory[CHART_HISTORY_SIZE];
static size_t chartHistoryWriteIndex = 0;
static size_t chartHistoryCount = 0;
static unsigned long chartSampleStartedAt = 0;
static long long chartSampleSum = 0;
static size_t chartSampleCount = 0;

static uint16_t clampChartValue(int value) {
  if (value < 0) return 0;
  if (value > 65535) return 65535;
  return value;
}

static void makeChartLabel(char* label, size_t labelSize) {
  struct tm timeinfo;

  if (getLocalTime(&timeinfo, 0)) {
    snprintf(label, labelSize, "%02d:%02d",
             timeinfo.tm_hour,
             timeinfo.tm_min);
    return;
  }

  unsigned long seconds = millis() / 1000;
  snprintf(label, labelSize, "%02lu:%02lu",
           (seconds / 3600) % 24,
           (seconds / 60) % 60);
}

static void storeChartHistoryPoint(int value, char* storedLabel, size_t storedLabelSize) {
  chartHistory[chartHistoryWriteIndex].value = clampChartValue(value);
  makeChartLabel(chartHistory[chartHistoryWriteIndex].label,
                 sizeof(chartHistory[chartHistoryWriteIndex].label));

  if (storedLabel != nullptr && storedLabelSize > 0) {
    snprintf(storedLabel, storedLabelSize, "%s", chartHistory[chartHistoryWriteIndex].label);
  }

  chartHistoryWriteIndex = (chartHistoryWriteIndex + 1) % CHART_HISTORY_SIZE;
  if (chartHistoryCount < CHART_HISTORY_SIZE) {
    chartHistoryCount++;
  }
}

void storeChartHistoryValue(int value) {
  storeChartHistoryPoint(value, nullptr, 0);
}

bool storeChartHistoryValueFromMessage(const String& message, String* chartMessage) {
  int separatorIndex = message.indexOf('|');
  String firstValue = separatorIndex >= 0 ? message.substring(0, separatorIndex) : message;

  if (firstValue.length() == 0) return false;

  for (size_t i = 0; i < firstValue.length(); i++) {
    if (!isDigit(firstValue[i])) return false;
  }

  unsigned long now = millis();
  if (chartSampleStartedAt == 0) {
    chartSampleStartedAt = now;
  }

  chartSampleSum += firstValue.toInt();
  chartSampleCount++;

  if (now - chartSampleStartedAt < CHART_HISTORY_SAMPLE_INTERVAL_MS) {
    return false;
  }

  int averageValue = (chartSampleSum + (long long)(chartSampleCount / 2)) / chartSampleCount;
  char label[6];
  storeChartHistoryPoint(averageValue, label, sizeof(label));

  chartSampleStartedAt = now;
  chartSampleSum = 0;
  chartSampleCount = 0;

  if (chartMessage != nullptr) {
    *chartMessage = "chart|";
    *chartMessage += label;
    *chartMessage += "|";
    *chartMessage += String(averageValue);
  }

  return true;
}

void sendChartHistory(AsyncWebSocketClient *client) {
  size_t startIndex = chartHistoryCount == CHART_HISTORY_SIZE ? chartHistoryWriteIndex : 0;
  String batch = "chart_batch|";
  size_t batchCount = 0;

  client->text("chart_begin");

  for (size_t i = 0; i < chartHistoryCount; i++) {
    size_t index = (startIndex + i) % CHART_HISTORY_SIZE;

    if (batchCount > 0) {
      batch += ";";
    }

    batch += chartHistory[index].label;
    batch += ",";
    batch += String(chartHistory[index].value);
    batchCount++;

    if (batchCount >= CHART_HISTORY_BATCH_SIZE) {
      client->text(batch);
      batch = "chart_batch|";
      batchCount = 0;
    }
  }

  if (batchCount > 0) {
    client->text(batch);
  }

  client->text("chart_end");
}
