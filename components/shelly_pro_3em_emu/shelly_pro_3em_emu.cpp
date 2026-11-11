#include "shelly_pro_3em_emu.h"

#include <cmath>
#include <cctype>
#include <cstring>
#include <vector>

#include <esp_heap_caps.h>
#include <esp_log.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <esp_attr.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "esphome/components/api/api_server.h"
#include "esphome/components/logger/logger.h"
#include "esphome/components/wifi/wifi_component.h"
#include "esphome/core/application.h"
#include "web_ui.h"

namespace esphome::shelly_pro_3em_emu {

static constexpr uint32_t SETTINGS_MAGIC = 0x50523345;
static constexpr uint32_t CRASH_MARKER_MAGIC = 0x43523345;
static const char *const TAG = "shelly_pro_3em_emu";

struct CrashMarker {
  uint32_t magic;
  char method[40];
  char stage[24];
  uint32_t stack_free;
};

RTC_NOINIT_ATTR static CrashMarker rtc_crash_marker;
static CrashMarker previous_crash_marker{};

static void mark_crash_stage(const char *method, const char *stage) {
  rtc_crash_marker.magic = 0;
  strlcpy(rtc_crash_marker.method, method, sizeof(rtc_crash_marker.method));
  strlcpy(rtc_crash_marker.stage, stage, sizeof(rtc_crash_marker.stage));
  rtc_crash_marker.stack_free = uxTaskGetStackHighWaterMark(nullptr);
  rtc_crash_marker.magic = CRASH_MARKER_MAGIC;
}

static const char *reset_reason_name(esp_reset_reason_t reason) {
  switch (reason) {
    case ESP_RST_POWERON: return "power_on";
    case ESP_RST_EXT: return "external";
    case ESP_RST_SW: return "software";
    case ESP_RST_PANIC: return "panic";
    case ESP_RST_INT_WDT: return "interrupt_watchdog";
    case ESP_RST_TASK_WDT: return "task_watchdog";
    case ESP_RST_WDT: return "watchdog";
    case ESP_RST_DEEPSLEEP: return "deep_sleep";
    case ESP_RST_BROWNOUT: return "brownout";
    case ESP_RST_SDIO: return "sdio";
    case ESP_RST_USB: return "usb";
    case ESP_RST_JTAG: return "jtag";
    case ESP_RST_EFUSE: return "efuse";
    case ESP_RST_PWR_GLITCH: return "power_glitch";
    case ESP_RST_CPU_LOCKUP: return "cpu_lockup";
    default: return "unknown";
  }
}

static void mask_log_strings(JsonVariant value) {
  if (value.is<JsonObject>()) {
    for (JsonPair pair : value.as<JsonObject>())
      mask_log_strings(pair.value());
  } else if (value.is<JsonArray>()) {
    for (JsonVariant item : value.as<JsonArray>())
      mask_log_strings(item);
  } else if (value.is<const char *>()) {
    value.set("[redacted]");
  }
}

static String safe_log_json(JsonVariantConst value) {
  JsonDocument safe;
  safe.set(value);
  mask_log_strings(safe.as<JsonVariant>());
  String json;
  serializeJson(safe, json);
  if (json.length() > 700)
    json = json.substring(0, 700) + "...[truncated]";
  return json;
}

static void log_rpc_payload_debug(const char *channel, const char *direction, const String &method,
                                  const String &json) {
  constexpr size_t CHUNK_SIZE = 150;
  const size_t chunks = (json.length() + CHUNK_SIZE - 1) / CHUNK_SIZE;
  for (size_t i = 0; i < chunks; i++) {
    const String chunk = json.substring(i * CHUNK_SIZE, std::min((i + 1) * CHUNK_SIZE, static_cast<size_t>(json.length())));
    ESP_LOGD(TAG, "RPC %s %s %s %u/%u %s", channel, direction, method.c_str(), static_cast<unsigned>(i + 1),
             static_cast<unsigned>(chunks), chunk.c_str());
  }
}

static void log_rpc_exchange(const char *channel, const String &method, JsonVariantConst input,
                             JsonVariantConst output, int error) {
  if (logger::global_logger == nullptr)
    return;
  const uint8_t level = logger::global_logger->get_log_level();
  if (level != ESPHOME_LOG_LEVEL_WARN && level != ESPHOME_LOG_LEVEL_DEBUG)
    return;
  const String request = safe_log_json(input);
  const String response = safe_log_json(output);
  if (level == ESPHOME_LOG_LEVEL_WARN) {
    ESP_LOGW(TAG, "RPC %s %s RX %.55s -> TX error=%d %.75s", channel, method.c_str(), request.c_str(), error,
             response.c_str());
  } else {
    log_rpc_payload_debug(channel, "RX", method, request);
    log_rpc_payload_debug(channel, "TX", method, response);
  }
}

static void decode_query_value(char *value) {
  char *write = value;
  for (const char *read = value; *read; read++) {
    if (*read == '%' && std::isxdigit(static_cast<unsigned char>(read[1])) &&
        std::isxdigit(static_cast<unsigned char>(read[2]))) {
      const char hex[] = {read[1], read[2], 0};
      *write++ = static_cast<char>(strtoul(hex, nullptr, 16));
      read += 2;
    } else {
      *write++ = *read == '+' ? ' ' : *read;
    }
  }
  *write = 0;
}
static const char *const METHODS[] = {
    "Shelly.GetDeviceInfo", "Shelly.GetStatus", "Shelly.GetConfig", "Shelly.ListMethods",
    "Shelly.ListProfiles", "Shelly.GetComponents", "Shelly.Reboot", "EM.GetStatus", "EM.GetConfig",
    "EMData.GetStatus", "Mqtt.GetStatus", "Mqtt.GetConfig", "Mqtt.SetConfig",
    "MQTT.GetStatus", "MQTT.GetConfig", "MQTT.SetConfig", "Sys.GetStatus",
    "Sys.GetConfig", "WiFi.GetStatus", "WiFi.GetConfig", "WiFi.SetConfig", "WiFi.Scan",
    "Wifi.GetStatus", "Wifi.GetConfig", "Wifi.SetConfig", "Wifi.Scan",
};

bool ShellyPro3EMEmu::valid_(sensor::Sensor *value) {
  return api::global_api_server != nullptr && api::global_api_server->is_connected_with_state_subscription() &&
         value != nullptr && value->has_state() && std::isfinite(value->state);
}

float ShellyPro3EMEmu::value_(sensor::Sensor *value) { return valid_(value) ? value->state : 0.0f; }

void ShellyPro3EMEmu::copy_(char *target, size_t size, const String &value) {
  strlcpy(target, value.c_str(), size);
}

String ShellyPro3EMEmu::mac_() const {
  uint8_t bytes[6]{};
  esp_wifi_get_mac(WIFI_IF_STA, bytes);
  char mac[13];
  snprintf(mac, sizeof(mac), "%02X%02X%02X%02X%02X%02X", bytes[0], bytes[1], bytes[2], bytes[3], bytes[4],
           bytes[5]);
  return String(mac);
}

String ShellyPro3EMEmu::id_() const {
  String mac = mac_();
  mac.toLowerCase();
  return "shellypro3em-" + mac;
}

String ShellyPro3EMEmu::prefix_() const {
  return settings_.topic_prefix[0] ? String(settings_.topic_prefix) : id_();
}

bool ShellyPro3EMEmu::source_ready_() const { return valid_(total_power_); }

void ShellyPro3EMEmu::device_info_(JsonObject out) const {
  out["id"] = id_();
  out["mac"] = mac_();
  out["model"] = "SPEM-003CEBEU";
  out["gen"] = 2;
  out["fw_id"] = "20261112/1.0.0-emu";
  out["ver"] = "1.0.0-emu";
  out["app"] = "Pro3EM";
  out["auth_en"] = false;
  out["auth_domain"] = id_();
  out["discoverable"] = true;
  out["profile"] = "triphase";
}

void ShellyPro3EMEmu::em_status_(JsonObject out) const {
  out["id"] = 0;
  float total_current = 0.0f;
  float total_apparent = 0.0f;
  for (size_t i = 0; i < 3; i++) {
    const Phase &phase = phases_[i];
    const String prefix("abc"[i]);
    const bool voltage_valid = valid_(phase.voltage);
    const bool current_valid = valid_(phase.current);
    if (voltage_valid)
      out[(prefix + "_voltage").c_str()] = value_(phase.voltage);
    else
      out[(prefix + "_voltage").c_str()] = nullptr;
    if (current_valid)
      out[(prefix + "_current").c_str()] = value_(phase.current);
    else
      out[(prefix + "_current").c_str()] = nullptr;
    if (valid_(phase.power))
      out[(prefix + "_act_power").c_str()] = value_(phase.power);
    else
      out[(prefix + "_act_power").c_str()] = nullptr;
    if (valid_(phase.power_factor))
      out[(prefix + "_pf").c_str()] = value_(phase.power_factor);
    else
      out[(prefix + "_pf").c_str()] = nullptr;
    if (voltage_valid && current_valid) {
      const float apparent = value_(phase.voltage) * value_(phase.current);
      out[(prefix + "_aprt_power").c_str()] = apparent;
      total_current += value_(phase.current);
      total_apparent += apparent;
    } else {
      out[(prefix + "_aprt_power").c_str()] = nullptr;
    }
    out[(prefix + "_freq").c_str()] = nullptr;
    out[(prefix + "_errors").c_str()].to<JsonArray>();
    out[(prefix + "_flags").c_str()].to<JsonArray>();
  }
  out["n_current"] = nullptr;
  if (source_ready_()) {
    out["total_current"] = total_current;
    out["total_act_power"] = value_(total_power_);
    out["total_aprt_power"] = total_apparent;
  } else {
    out["total_current"] = nullptr;
    out["total_act_power"] = nullptr;
    out["total_aprt_power"] = nullptr;
  }
  out["user_calibrated_phase"].to<JsonArray>();
  out["errors"].to<JsonArray>();
}

void ShellyPro3EMEmu::emdata_status_(JsonObject out) const {
  out["id"] = 0;
  for (size_t i = 0; i < 3; i++) {
    const String prefix("abc"[i]);
    if (valid_(phases_[i].import_energy))
      out[(prefix + "_total_act_energy").c_str()] = value_(phases_[i].import_energy);
    else
      out[(prefix + "_total_act_energy").c_str()] = nullptr;
    if (valid_(phases_[i].export_energy))
      out[(prefix + "_total_act_ret_energy").c_str()] = value_(phases_[i].export_energy);
    else
      out[(prefix + "_total_act_ret_energy").c_str()] = nullptr;
  }
  if (valid_(import_energy_))
    out["total_act"] = value_(import_energy_);
  else
    out["total_act"] = nullptr;
  if (valid_(export_energy_))
    out["total_act_ret"] = value_(export_energy_);
  else
    out["total_act_ret"] = nullptr;
}

void ShellyPro3EMEmu::mqtt_config_(JsonObject out) const {
  out["enable"] = settings_.enabled != 0;
  if (settings_.server[0])
    out["server"] = settings_.server;
  else
    out["server"] = nullptr;
  if (settings_.client_id[0])
    out["client_id"] = settings_.client_id;
  else
    out["client_id"] = nullptr;
  if (settings_.user[0])
    out["user"] = settings_.user;
  else
    out["user"] = nullptr;
  out["ssl_ca"] = nullptr;
  if (settings_.topic_prefix[0])
    out["topic_prefix"] = settings_.topic_prefix;
  else
    out["topic_prefix"] = nullptr;
  out["rpc_ntf"] = settings_.rpc_ntf != 0;
  out["status_ntf"] = settings_.status_ntf != 0;
  out["enable_rpc"] = true;
  out["enable_control"] = true;
}

void ShellyPro3EMEmu::mqtt_status_(JsonObject out) const { out["connected"] = broker_->is_connected(); }

void ShellyPro3EMEmu::sys_config_(JsonObject out) const {
  JsonObject device = out["device"].to<JsonObject>();
  device["name"] = "Virtueller Shelly Pro 3EM";
  device["mac"] = mac_();
  device["fw_id"] = "20261112/1.0.0-emu";
  device["eco_mode"] = false;
  device["profile"] = "triphase";
  device["discoverable"] = true;
  device["tls_check_cert_validity_time"] = true;
  device["enhanced_security"] = false;
  JsonObject location = out["location"].to<JsonObject>();
  location["tz"] = "Europe/Vienna";
  location["lat"] = nullptr;
  location["lon"] = nullptr;
  JsonObject debug = out["debug"].to<JsonObject>();
  debug["mqtt"]["enable"] = false;
  debug["websocket"]["enable"] = false;
  debug["udp"]["addr"] = nullptr;
  out["ui_data"].to<JsonObject>();
  JsonObject rpc_udp = out["rpc_udp"].to<JsonObject>();
  rpc_udp["dst_addr"] = nullptr;
  rpc_udp["listen_port"] = nullptr;
  out["sntp"]["server"] = "time.google.com";
  out["cfg_rev"] = 1;
}

void ShellyPro3EMEmu::sys_status_(JsonObject out) const {
  out["mac"] = mac_();
  out["uptime"] = millis() / 1000;
  out["ram_size"] = heap_caps_get_total_size(MALLOC_CAP_8BIT);
  out["ram_free"] = heap_caps_get_free_size(MALLOC_CAP_8BIT);
  out["restart_required"] = false;
}

void ShellyPro3EMEmu::wifi_status_(JsonObject out) const {
  char ip[network::IP_ADDRESS_BUFFER_SIZE]{};
  wifi::global_wifi_component->wifi_sta_ip_addresses()[0].str_to(ip);
  char ssid[wifi::SSID_BUFFER_SIZE]{};
  wifi::global_wifi_component->wifi_ssid_to(ssid);
  out["sta_ip"] = ip;
  out["status"] = wifi::global_wifi_component->is_connected() ? "got ip" : "disconnected";
  out["ssid"] = ssid;
  out["rssi"] = wifi::global_wifi_component->wifi_rssi();
}

void ShellyPro3EMEmu::wifi_config_(JsonObject out) const {
  JsonObject ap = out["ap"].to<JsonObject>();
  ap["ssid"] = "ShellyPro3EM-" + mac_();
  ap["is_open"] = true;
  ap["enable"] = pairing_active_.load();
  JsonObject sta = out["sta"].to<JsonObject>();
  char ssid[wifi::SSID_BUFFER_SIZE]{};
  wifi::global_wifi_component->wifi_ssid_to(ssid);
  sta["ssid"] = ssid;
  sta["is_open"] = false;
  sta["enable"] = true;
  sta["ipv4mode"] = "dhcp";
  sta["ip"] = nullptr;
  sta["netmask"] = nullptr;
  sta["gw"] = nullptr;
  sta["nameserver"] = nullptr;
}

int ShellyPro3EMEmu::wifi_scan_(JsonObject out) const {
  wifi_scan_config_t scan{};
  scan.show_hidden = false;
  std::vector<wifi_ap_record_t> networks;
  if (esp_wifi_scan_start(&scan, true) == ESP_OK) {
    uint16_t found = 0;
    if (esp_wifi_scan_get_ap_num(&found) == ESP_OK && found > 0) {
      uint16_t count = std::min<uint16_t>(found, 12);
      networks.resize(count);
      if (esp_wifi_scan_get_ap_records(&count, networks.data()) == ESP_OK)
        networks.resize(count);
      else
        networks.clear();
    }
  }
  wifi_ap_record_t connected{};
  if (esp_wifi_sta_get_ap_info(&connected) == ESP_OK) {
    bool found = false;
    for (const auto &network : networks)
      found |= memcmp(network.bssid, connected.bssid, sizeof(connected.bssid)) == 0;
    if (!found)
      networks.insert(networks.begin(), connected);
  }
  JsonArray results = out["results"].to<JsonArray>();
  for (const auto &network : networks) {
    JsonObject item = results.add<JsonObject>();
    item["ssid"] = reinterpret_cast<const char *>(network.ssid);
    char bssid[18];
    snprintf(bssid, sizeof(bssid), "%02x:%02x:%02x:%02x:%02x:%02x", network.bssid[0], network.bssid[1],
             network.bssid[2], network.bssid[3], network.bssid[4], network.bssid[5]);
    item["bssid"] = bssid;
    item["auth"] = static_cast<int>(network.authmode);
    item["channel"] = network.primary;
    item["rssi"] = network.rssi;
  }
  return 0;
}

int ShellyPro3EMEmu::wifi_set_config_(JsonVariantConst config, JsonObject out) {
  if (!config.is<JsonObjectConst>())
    return -32602;
  JsonVariantConst sta = config["sta"];
  if (!sta.isNull()) {
    if (!sta["enable"].isNull() && sta["enable"] == false)
      return -32602;
    if (!sta["ssid"].isNull()) {
      char ssid[wifi::SSID_BUFFER_SIZE]{};
      wifi::global_wifi_component->wifi_ssid_to(ssid);
      if (sta["ssid"].as<String>() != ssid)
        return -32602;
    }
  }
  JsonVariantConst ap = config["ap"];
  if (!ap.isNull()) {
    if (!ap["ssid"].isNull() && ap["ssid"].as<String>() != "ShellyPro3EM-" + mac_())
      return -32602;
    if (!ap["enable"].isNull() && ap["enable"] == false)
      pairing_stop_after_ = millis() + 2000;
  }
  out["restart_required"] = false;
  return 0;
}

void ShellyPro3EMEmu::start_pairing() {
  mark_crash_stage("pairing", "requested");
  pairing_deadline_ = millis() + 10 * 60 * 1000;
  pairing_stop_after_ = 0;
  last_ap_check_ = 0;
  pairing_active_ = true;
}

void ShellyPro3EMEmu::press_factory_reset_button() {
  const uint32_t now = millis();
  if (factory_reset_arm_deadline_ != 0 && static_cast<int32_t>(now - factory_reset_arm_deadline_) < 0) {
    factory_reset_arm_deadline_ = 0;
    factory_reset_pending_ = true;
    return;
  }
  factory_reset_arm_deadline_ = now + 10000;
  ESP_LOGW(TAG, "Factory reset armed; press again within 10 seconds to clear MQTT settings");
}

void ShellyPro3EMEmu::factory_reset_() {
  ProMqttSettings defaults{};
  defaults.magic = SETTINGS_MAGIC;
  if (!preference_.save(&defaults)) {
    ESP_LOGE(TAG, "Factory reset failed: could not save default settings");
    return;
  }
  global_preferences->sync();
  settings_ = defaults;
  apply_broker_();
  start_pairing();
  ESP_LOGW(TAG, "Factory reset complete: MQTT settings cleared; ESPHome Wi-Fi and API retained");
}

void ShellyPro3EMEmu::update_pairing_() {
  const uint32_t now = millis();
  if (pairing_active_ &&
      (static_cast<int32_t>(now - pairing_deadline_) >= 0 ||
       (pairing_stop_after_ != 0 && static_cast<int32_t>(now - pairing_stop_after_) >= 0))) {
    pairing_active_ = false;
    pairing_stop_after_ = 0;
  }
  if (!pairing_active_) {
    if (pairing_ap_started_) {
      esp_wifi_set_mode(WIFI_MODE_STA);
      pairing_ap_started_ = false;
    }
    return;
  }
  if (!wifi::global_wifi_component->is_connected() ||
      (last_ap_check_ != 0 && now - last_ap_check_ < 5000))
    return;
  last_ap_check_ = now;
  wifi_mode_t mode = WIFI_MODE_NULL;
  if (esp_wifi_get_mode(&mode) != ESP_OK)
    return;
  if (pairing_ap_started_ && mode == WIFI_MODE_APSTA)
    return;
  if (ap_netif_ == nullptr)
    ap_netif_ = esp_netif_create_default_wifi_ap();
  if (ap_netif_ == nullptr) {
    ESP_LOGE(TAG, "Could not create pairing AP interface");
    return;
  }
  if (esp_wifi_set_mode(WIFI_MODE_APSTA) != ESP_OK)
    return;
  wifi_config_t config{};
  const String ssid = "ShellyPro3EM-" + mac_();
  strlcpy(reinterpret_cast<char *>(config.ap.ssid), ssid.c_str(), sizeof(config.ap.ssid));
  config.ap.ssid_len = ssid.length();
  config.ap.authmode = WIFI_AUTH_OPEN;
  config.ap.max_connection = 4;
  uint8_t channel = 1;
  wifi_second_chan_t secondary;
  if (esp_wifi_get_channel(&channel, &secondary) != ESP_OK || channel == 0)
    channel = 1;
  config.ap.channel = channel;
  config.ap.beacon_interval = 100;
  if (esp_wifi_set_config(WIFI_IF_AP, &config) != ESP_OK)
    return;
  esp_netif_ip_info_t ip{};
  ip.ip.addr = ESP_IP4TOADDR(192, 168, 33, 1);
  ip.gw = ip.ip;
  ip.netmask.addr = ESP_IP4TOADDR(255, 255, 255, 0);
  const esp_err_t stop = esp_netif_dhcps_stop(ap_netif_);
  if (stop != ESP_OK && stop != ESP_ERR_ESP_NETIF_DHCP_ALREADY_STOPPED)
    return;
  if (esp_netif_set_ip_info(ap_netif_, &ip) != ESP_OK || esp_netif_dhcps_start(ap_netif_) != ESP_OK)
    return;
  pairing_ap_started_ = true;
  mark_crash_stage("pairing", "ap_active");
  ESP_LOGI(TAG, "Pairing AP active at 192.168.33.1");
}

String ShellyPro3EMEmu::dashboard_json_() const {
  JsonDocument doc;
  doc["model"] = "Shelly Pro 3EM Gen2";
  doc["id"] = id_();
  doc["ha_connected"] = api::global_api_server != nullptr &&
                        api::global_api_server->is_connected_with_state_subscription();
  doc["measurements_ready"] = source_ready_();
  const bool wifi_connected = wifi::global_wifi_component != nullptr &&
                              wifi::global_wifi_component->is_connected();
  doc["wifi_connected"] = wifi_connected;
  if (wifi_connected) {
    char ip[network::IP_ADDRESS_BUFFER_SIZE]{};
    wifi::global_wifi_component->wifi_sta_ip_addresses()[0].str_to(ip);
    doc["wifi_ip"] = ip;
    doc["wifi_rssi"] = wifi::global_wifi_component->wifi_rssi();
  } else {
    doc["wifi_ip"] = nullptr;
    doc["wifi_rssi"] = nullptr;
  }
  doc["mqtt_enabled"] = settings_.enabled != 0;
  doc["mqtt_connected"] = broker_->is_connected();
  doc["mqtt_server"] = settings_.server;
  doc["pairing_active"] = pairing_active_.load();
  doc["reset_reason"] = reset_reason_name(esp_reset_reason());
  doc["pairing_ssid"] = pairing_active_ ? "ShellyPro3EM-" + mac_() : "";
  wifi_mode_t mode = WIFI_MODE_NULL;
  esp_wifi_get_mode(&mode);
  JsonObject diagnostic = doc["diagnostic"].to<JsonObject>();
  diagnostic["ap_running"] = mode == WIFI_MODE_APSTA;
  diagnostic["publish_interval_ms"] = publish_interval_ms_;
  diagnostic["status_publish_successes"] = status_publish_successes_;
  diagnostic["status_publish_failures"] = status_publish_failures_;
  diagnostic["last_status_publish_uptime_s"] = last_status_publish_uptime_s_;
  diagnostic["announce_publish_successes"] = announce_publish_successes_;
  diagnostic["notification_publish_successes"] = notification_publish_successes_;
  diagnostic["notification_publish_failures"] = notification_publish_failures_;
  diagnostic["mqtt_rpc_received"] = mqtt_rpc_received_.load();
  diagnostic["mqtt_control_received"] = mqtt_control_received_.load();
  if (previous_crash_marker.magic == CRASH_MARKER_MAGIC) {
    diagnostic["previous_method"] = previous_crash_marker.method;
    diagnostic["previous_stage"] = previous_crash_marker.stage;
    diagnostic["previous_stack_free"] = previous_crash_marker.stack_free;
  }
  esp_netif_t *ap = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
  if (ap != nullptr && mode == WIFI_MODE_APSTA) {
    esp_netif_ip_info_t ip{};
    if (esp_netif_get_ip_info(ap, &ip) == ESP_OK) {
      char address[16];
      snprintf(address, sizeof(address), IPSTR, IP2STR(&ip.ip));
      diagnostic["ap_ip"] = address;
    }
    esp_netif_dhcp_status_t dhcp{};
    if (esp_netif_dhcps_get_status(ap, &dhcp) == ESP_OK)
      diagnostic["dhcp_running"] = dhcp == ESP_NETIF_DHCP_STARTED;
    wifi_sta_list_t clients{};
    if (esp_wifi_ap_get_sta_list(&clients) == ESP_OK)
      diagnostic["ap_clients"] = clients.num;
  }
  JsonArray requests = diagnostic["recent_requests"].to<JsonArray>();
  const size_t count = std::min(request_count_, recent_requests_.size());
  for (size_t i = 0; i < count; i++) {
    const auto &entry = recent_requests_[(request_head_ + recent_requests_.size() - count + i) % recent_requests_.size()];
    JsonObject item = requests.add<JsonObject>();
    item["path"] = entry.path;
    if (entry.method[0])
      item["method"] = entry.method;
    item["error"] = entry.error;
    item["count"] = entry.count;
    item["uptime_s"] = entry.uptime_s;
  }
  doc["uptime_s"] = millis() / 1000;
  auto set_number = [&](JsonObject object, const char *key, sensor::Sensor *sensor) {
    if (valid_(sensor))
      object[key] = sensor->state;
    else
      object[key] = nullptr;
  };
  JsonObject root = doc.as<JsonObject>();
  set_number(root, "power", total_power_);
  set_number(root, "import_wh", import_energy_);
  set_number(root, "export_wh", export_energy_);
  JsonArray phases = doc["phases"].to<JsonArray>();
  for (const Phase &phase : phases_) {
    JsonObject item = phases.add<JsonObject>();
    set_number(item, "power", phase.power);
    set_number(item, "voltage", phase.voltage);
    set_number(item, "current", phase.current);
    set_number(item, "pf", phase.power_factor);
    set_number(item, "import_wh", phase.import_energy);
    set_number(item, "export_wh", phase.export_energy);
  }
  String body;
  serializeJson(doc, body);
  return body;
}

void ShellyPro3EMEmu::record_request_(const String &path, const String &method, int error) {
  const size_t count = std::min(request_count_, recent_requests_.size());
  for (size_t i = 0; i < count; i++) {
    DiagnosticEntry &existing = recent_requests_[i];
    if (path == existing.path && method == existing.method && error == existing.error) {
      existing.count++;
      existing.uptime_s = millis() / 1000;
      return;
    }
  }
  DiagnosticEntry &entry = recent_requests_[request_head_];
  copy_(entry.path, sizeof(entry.path), path);
  copy_(entry.method, sizeof(entry.method), method);
  entry.error = error;
  entry.count = 1;
  entry.uptime_s = millis() / 1000;
  request_head_ = (request_head_ + 1) % recent_requests_.size();
  request_count_++;
}

void ShellyPro3EMEmu::apply_broker_() {
  broker_->disable();
  for (auto &topic : subscribed_topics_) {
    if (!topic.empty()) {
      broker_->unsubscribe(topic);
      topic.clear();
    }
  }
  if (!settings_.enabled || !settings_.server[0])
    return;
  std::string server(settings_.server);
  uint16_t port = 1883;
  const auto separator = server.rfind(':');
  if (separator != std::string::npos) {
    const int parsed = atoi(server.c_str() + separator + 1);
    if (parsed > 0 && parsed <= 65535) {
      port = parsed;
      server.resize(separator);
    }
  }
  broker_->set_broker_address(server);
  broker_->set_broker_port(port);
  broker_->set_username(settings_.user);
  broker_->set_password(settings_.password);
  broker_->set_client_id(settings_.client_id[0] ? settings_.client_id : id_().c_str());
  broker_->set_last_will(mqtt::MQTTMessage{std::string(prefix_().c_str()) + "/online", "false", 0, true});
  broker_->set_birth_message(mqtt::MQTTMessage{std::string(prefix_().c_str()) + "/online", "true", 0, true});
  subscribed_topics_[0] = std::string(prefix_().c_str()) + "/rpc";
  broker_->subscribe(subscribed_topics_[0], [this](const std::string &, const std::string &payload) {
    JsonDocument request;
    if (deserializeJson(request, payload) != DeserializationError::Ok)
      return;
    mqtt_rpc_received_++;
    const String reply = rpc_frame_(request.as<JsonVariantConst>(), false);
    const char *source = request["src"] | "";
    if (source[0])
      broker_->publish(std::string(source) + "/rpc", reply.c_str(), reply.length(), 1, false);
  });
  subscribed_topics_[1] = std::string(prefix_().c_str()) + "/command";
  subscribed_topics_[2] = "shellies/command";
  auto control = [this](const std::string &, const std::string &command) {
    mqtt_control_received_++;
    ESP_LOGD(TAG, "MQTT control command: %s", command.c_str());
    if (command == "announce")
      publish_announce_(true);
    else if (command == "status_update")
      publish_full_status_();
  };
  broker_->subscribe(subscribed_topics_[1], control);
  broker_->subscribe(subscribed_topics_[2], control);
  broker_->enable();
}

void ShellyPro3EMEmu::publish_announce_(bool broadcast) {
  JsonDocument announce;
  device_info_(announce.to<JsonObject>());
  String payload;
  serializeJson(announce, payload);
  if (broker_->publish(std::string(prefix_().c_str()) + "/announce", payload.c_str(), payload.length(), 1, false))
    announce_publish_successes_++;
  if (broadcast)
    broker_->publish("shellies/announce", payload.c_str(), payload.length(), 1, false);
}

void ShellyPro3EMEmu::publish_full_status_() {
  JsonDocument status;
  JsonObject out = status.to<JsonObject>();
  sys_status_(out["sys"].to<JsonObject>());
  wifi_status_(out["wifi"].to<JsonObject>());
  mqtt_status_(out["mqtt"].to<JsonObject>());
  em_status_(out["em:0"].to<JsonObject>());
  emdata_status_(out["emdata:0"].to<JsonObject>());
  String payload;
  serializeJson(status, payload);
  broker_->publish(std::string(prefix_().c_str()) + "/status", payload.c_str(), payload.length(), 1, false);
  publish_status_();
}

int ShellyPro3EMEmu::mqtt_set_config_(JsonVariantConst config, JsonObject out) {
  mark_crash_stage("Mqtt.SetConfig", "validate");
  if (!config.is<JsonObjectConst>())
    return -32602;
  if (!config["ssl_ca"].isNull())
    return -32000;
  ProMqttSettings next = settings_;
  if (!config["enable"].isNull())
    next.enabled = config["enable"].as<bool>();
  if (!config["server"].isNull())
    copy_(next.server, sizeof(next.server), config["server"].as<String>());
  if (!config["user"].isNull())
    copy_(next.user, sizeof(next.user), config["user"].as<String>());
  if (!config["pass"].isNull())
    copy_(next.password, sizeof(next.password), config["pass"].as<String>());
  if (!config["client_id"].isNull())
    copy_(next.client_id, sizeof(next.client_id), config["client_id"].as<String>());
  if (!config["topic_prefix"].isNull())
    copy_(next.topic_prefix, sizeof(next.topic_prefix), config["topic_prefix"].as<String>());
  if (!config["rpc_ntf"].isNull())
    next.rpc_ntf = config["rpc_ntf"].as<bool>();
  if (!config["status_ntf"].isNull())
    next.status_ntf = config["status_ntf"].as<bool>();
  if (next.enabled && !next.server[0])
    return -32602;
  settings_ = next;
  mark_crash_stage("Mqtt.SetConfig", "persist");
  preference_.save(&settings_);
  global_preferences->sync();
  // The RPC handler runs in the HTTP server task; MQTT belongs to the ESPHome loop task.
  mqtt_apply_after_ = millis() + 500;
  mark_crash_stage("Mqtt.SetConfig", "queued");
  out["restart_required"] = true;
  return 0;
}

int ShellyPro3EMEmu::dispatch_(const String &method, JsonVariantConst params, JsonObject out) {
  if (method == "Shelly.GetDeviceInfo") {
    device_info_(out);
  } else if (method == "Shelly.GetStatus") {
    sys_status_(out["sys"].to<JsonObject>());
    wifi_status_(out["wifi"].to<JsonObject>());
    mqtt_status_(out["mqtt"].to<JsonObject>());
    em_status_(out["em:0"].to<JsonObject>());
    emdata_status_(out["emdata:0"].to<JsonObject>());
  } else if (method == "Shelly.GetConfig") {
    mqtt_config_(out["mqtt"].to<JsonObject>());
    sys_config_(out["sys"].to<JsonObject>());
    wifi_config_(out["wifi"].to<JsonObject>());
  } else if (method == "Shelly.ListMethods") {
    JsonArray methods = out["methods"].to<JsonArray>();
    for (const char *method_name : METHODS)
      methods.add(method_name);
  } else if (method == "Shelly.ListProfiles") {
    out["profiles"].to<JsonArray>().add("triphase");
  } else if (method == "Shelly.GetComponents") {
    // The emulator has no dynamic components. HA requests this during discovery.
    out["components"].to<JsonArray>();
    out["cfg_rev"] = 1;
    out["offset"] = params["offset"] | 0;
    out["total"] = 0;
  } else if (method == "Shelly.Reboot") {
    this->set_timeout(200, []() { App.safe_reboot(); });
  } else if (method == "EM.GetStatus" || method == "EMData.GetStatus" || method == "EM.GetConfig") {
    if (!params["id"].isNull() && params["id"].as<int>() != 0)
      return -32602;
    if (method == "EM.GetStatus") {
      em_status_(out);
    } else if (method == "EMData.GetStatus") {
      emdata_status_(out);
    } else {
      out["id"] = 0;
      out["name"] = nullptr;
      out["blink_mode_selector"] = "active_energy";
      out["phase_selector"] = "all";
      out["monitor_phase_sequence"] = false;
      out["ct_type"] = "120A";
    }
  } else if (method == "Mqtt.GetStatus" || method == "MQTT.GetStatus") {
    mqtt_status_(out);
  } else if (method == "Mqtt.GetConfig" || method == "MQTT.GetConfig") {
    mqtt_config_(out);
  } else if (method == "Mqtt.SetConfig" || method == "MQTT.SetConfig") {
    return mqtt_set_config_(params["config"], out);
  } else if (method == "Sys.GetStatus") {
    sys_status_(out);
  } else if (method == "Sys.GetConfig") {
    sys_config_(out);
  } else if (method == "Wifi.GetStatus" || method == "WiFi.GetStatus") {
    wifi_status_(out);
  } else if (method == "Wifi.GetConfig" || method == "WiFi.GetConfig") {
    wifi_config_(out);
  } else if (method == "Wifi.SetConfig" || method == "WiFi.SetConfig") {
    return wifi_set_config_(params["config"], out);
  } else if (method == "Wifi.Scan" || method == "WiFi.Scan") {
    return wifi_scan_(out);
  } else {
    return -32601;
  }
  return 0;
}

String ShellyPro3EMEmu::rpc_frame_(JsonVariantConst frame, bool track_http) {
  JsonDocument response;
  response["id"] = frame["id"];
  response["src"] = id_();
  if (!frame["src"].isNull())
    response["dst"] = frame["src"];
  JsonObject result = response["result"].to<JsonObject>();
  const String method = frame["method"].as<String>();
  mark_crash_stage(method.c_str(), "dispatch");
  const int error = dispatch_(method, frame["params"], result);
  mark_crash_stage(method.c_str(), "result");
  if (track_http)
    record_request_("RPC", method, error);
  if (error) {
    response.remove("result");
    JsonObject fault = response["error"].to<JsonObject>();
    fault["code"] = error;
    fault["message"] = error == -32601 ? "Method not found" : error == -32602 ? "Invalid params" : "Unsupported setting";
  }
  log_rpc_exchange(track_http ? "HTTP/WS" : "MQTT", method, frame["params"], response.as<JsonVariantConst>(), error);
  mark_crash_stage(method.c_str(), "serialize");
  String body;
  serializeJson(response, body);
  return body;
}

String ShellyPro3EMEmu::rpc_direct_(const String &method, JsonVariantConst params) {
  JsonDocument response;
  JsonObject out = response.to<JsonObject>();
  mark_crash_stage(method.c_str(), "dispatch");
  const int error = dispatch_(method, params, out);
  mark_crash_stage(method.c_str(), "result");
  record_request_("RPC", method, error);
  if (error) {
    response.clear();
    response["code"] = error;
    response["message"] = error == -32601 ? "Method not found" : error == -32602 ? "Invalid params" : "Unsupported setting";
  }
  log_rpc_exchange("HTTP", method, params, response.as<JsonVariantConst>(), error);
  mark_crash_stage(method.c_str(), "serialize");
  String body;
  serializeJson(response, body);
  return body;
}

esp_err_t ShellyPro3EMEmu::send_json_(httpd_req_t *request, const String &body) {
  httpd_resp_set_type(request, "application/json");
  return httpd_resp_send(request, body.c_str(), body.length());
}

esp_err_t ShellyPro3EMEmu::get_handler_(httpd_req_t *request) {
  return static_cast<ShellyPro3EMEmu *>(request->user_ctx)->handle_get_(request);
}

esp_err_t ShellyPro3EMEmu::post_handler_(httpd_req_t *request) {
  return static_cast<ShellyPro3EMEmu *>(request->user_ctx)->handle_post_(request);
}

esp_err_t ShellyPro3EMEmu::websocket_handler_(httpd_req_t *request) {
  return static_cast<ShellyPro3EMEmu *>(request->user_ctx)->handle_websocket_(request);
}

esp_err_t ShellyPro3EMEmu::not_found_handler_(httpd_req_t *request) {
  auto *self = static_cast<ShellyPro3EMEmu *>(request->user_ctx);
  String path(request->uri);
  const int query = path.indexOf('?');
  if (query >= 0)
    path.remove(query);
  self->record_request_(path, request->method == HTTP_POST ? "POST" : "GET", 404);
  httpd_resp_send_err(request, HTTPD_404_NOT_FOUND, "Not found");
  return ESP_OK;
}

esp_err_t ShellyPro3EMEmu::handle_get_(httpd_req_t *request) {
  String uri(request->uri);
  const int query_start = uri.indexOf('?');
  if (query_start >= 0)
    uri.remove(query_start);
  if (uri != "/" && uri != "/emu/status")
    record_request_(uri, "GET", 0);
  if (uri == "/") {
    httpd_resp_set_type(request, "text/html; charset=utf-8");
    return httpd_resp_send(request, WEB_UI_HTML, HTTPD_RESP_USE_STRLEN);
  }
  if (uri == "/emu/status")
    return send_json_(request, dashboard_json_());
  if (uri == "/shelly") {
    JsonDocument response;
    device_info_(response.to<JsonObject>());
    String body;
    serializeJson(response, body);
    return send_json_(request, body);
  }
  JsonDocument params;
  const size_t query_length = httpd_req_get_url_query_len(request);
  if (query_length > 0 && query_length < 2048) {
    std::vector<char> query(query_length + 1);
    if (httpd_req_get_url_query_str(request, query.data(), query.size()) == ESP_OK) {
      char value[1024];
      if (httpd_query_key_value(query.data(), "id", value, sizeof(value)) == ESP_OK)
        params["id"] = atoi(value);
      if (httpd_query_key_value(query.data(), "config", value, sizeof(value)) == ESP_OK) {
        decode_query_value(value);
        deserializeJson(params["config"], value);
      }
    }
  }
  return send_json_(request, rpc_direct_(uri.substring(5), params.as<JsonVariantConst>()));
}

esp_err_t ShellyPro3EMEmu::handle_post_(httpd_req_t *request) {
  String uri(request->uri);
  const int query_start = uri.indexOf('?');
  if (query_start >= 0)
    uri.remove(query_start);
  record_request_(uri, "POST", 0);
  if (uri == "/emu/pairing") {
    char action[8]{};
    if (httpd_req_get_hdr_value_str(request, "X-Emulator-Action", action, sizeof(action)) != ESP_OK ||
        strcmp(action, "pair") != 0) {
      httpd_resp_send_err(request, HTTPD_403_FORBIDDEN, "Action header required");
      return ESP_OK;
    }
    start_pairing();
    return send_json_(request, dashboard_json_());
  }
  if (uri == "/emu/reboot") {
    char action[8]{};
    if (httpd_req_get_hdr_value_str(request, "X-Emulator-Action", action, sizeof(action)) != ESP_OK ||
        strcmp(action, "reboot") != 0) {
      httpd_resp_send_err(request, HTTPD_403_FORBIDDEN, "Action header required");
      return ESP_OK;
    }
    reboot_after_ = millis() + 750;
    return send_json_(request, "{\"accepted\":true}");
  }
  if (request->content_len == 0 || request->content_len > 4096) {
    httpd_resp_send_err(request, HTTPD_400_BAD_REQUEST, "Invalid JSON body");
    return ESP_FAIL;
  }
  std::vector<char> body(request->content_len + 1);
  size_t received = 0;
  while (received < request->content_len) {
    const int count = httpd_req_recv(request, body.data() + received, request->content_len - received);
    if (count <= 0)
      return ESP_FAIL;
    received += count;
  }
  JsonDocument json;
  if (deserializeJson(json, body.data(), received) != DeserializationError::Ok) {
    httpd_resp_send_err(request, HTTPD_400_BAD_REQUEST, "Invalid JSON body");
    return ESP_FAIL;
  }
  if (uri == "/emu/factory-reset") {
    char action[16]{};
    if (httpd_req_get_hdr_value_str(request, "X-Emulator-Action", action, sizeof(action)) != ESP_OK ||
        strcmp(action, "factory-reset") != 0 || json["confirm"] != "RESET") {
      httpd_resp_send_err(request, HTTPD_403_FORBIDDEN, "Confirmation required");
      return ESP_OK;
    }
    factory_reset_pending_ = true;
    return send_json_(request, "{\"accepted\":true}");
  }
  if (uri == "/rpc")
    return send_json_(request, rpc_frame_(json.as<JsonVariantConst>()));
  return send_json_(request, rpc_direct_(uri.substring(5), json.as<JsonVariantConst>()));
}

esp_err_t ShellyPro3EMEmu::handle_websocket_(httpd_req_t *request) {
  httpd_ws_frame_t frame{};
  if (httpd_ws_recv_frame(request, &frame, 0) != ESP_OK || frame.len > 4096)
    return ESP_FAIL;
  if (frame.len == 0)
    return ESP_OK;
  std::vector<uint8_t> body(frame.len + 1);
  frame.payload = body.data();
  if (httpd_ws_recv_frame(request, &frame, frame.len) != ESP_OK)
    return ESP_FAIL;
  if (frame.type != HTTPD_WS_TYPE_TEXT)
    return ESP_OK;
  JsonDocument json;
  if (deserializeJson(json, body.data(), frame.len) != DeserializationError::Ok)
    return ESP_FAIL;
  String reply = rpc_frame_(json.as<JsonVariantConst>());
  httpd_ws_frame_t response{};
  response.type = HTTPD_WS_TYPE_TEXT;
  response.payload = reinterpret_cast<uint8_t *>(reply.begin());
  response.len = reply.length();
  return httpd_ws_send_frame(request, &response);
}

void ShellyPro3EMEmu::setup() {
  ESP_LOGI(TAG, "Previous reset reason: %s", reset_reason_name(esp_reset_reason()));
  if (rtc_crash_marker.magic == CRASH_MARKER_MAGIC) {
    previous_crash_marker = rtc_crash_marker;
    previous_crash_marker.method[sizeof(previous_crash_marker.method) - 1] = 0;
    previous_crash_marker.stage[sizeof(previous_crash_marker.stage) - 1] = 0;
    ESP_LOGW(TAG, "Previous step: %s / %s, HTTP stack free %u", previous_crash_marker.method,
             previous_crash_marker.stage, static_cast<unsigned>(previous_crash_marker.stack_free));
  }
  mark_crash_stage("boot", "setup");
  preference_ = global_preferences->make_preference<ProMqttSettings>(SETTINGS_MAGIC);
  if (!preference_.load(&settings_) || settings_.magic != SETTINGS_MAGIC) {
    settings_ = ProMqttSettings{};
    settings_.magic = SETTINGS_MAGIC;
  }
  settings_.server[sizeof(settings_.server) - 1] = 0;
  settings_.user[sizeof(settings_.user) - 1] = 0;
  settings_.password[sizeof(settings_.password) - 1] = 0;
  settings_.client_id[sizeof(settings_.client_id) - 1] = 0;
  settings_.topic_prefix[sizeof(settings_.topic_prefix) - 1] = 0;
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 80;
  config.max_open_sockets = 4;
  config.stack_size = 8192;
  config.lru_purge_enable = true;
  config.max_uri_handlers = 12;
  config.uri_match_fn = httpd_uri_match_wildcard;
  if (httpd_start(&server_, &config) != ESP_OK) {
    mark_failed();
    return;
  }
  httpd_uri_t index{"/", HTTP_GET, get_handler_, this, false, false, nullptr};
  httpd_uri_t status{"/emu/status", HTTP_GET, get_handler_, this, false, false, nullptr};
  httpd_uri_t pairing{"/emu/pairing", HTTP_POST, post_handler_, this, false, false, nullptr};
  httpd_uri_t reboot{"/emu/reboot", HTTP_POST, post_handler_, this, false, false, nullptr};
  httpd_uri_t factory_reset{"/emu/factory-reset", HTTP_POST, post_handler_, this, false, false, nullptr};
  httpd_uri_t shelly{"/shelly", HTTP_GET, get_handler_, this, false, false, nullptr};
  httpd_uri_t rpc_post{"/rpc", HTTP_POST, post_handler_, this, false, false, nullptr};
  httpd_uri_t rpc_ws{"/rpc", HTTP_GET, websocket_handler_, this, true, false, nullptr};
  httpd_uri_t direct_get{"/rpc/*", HTTP_GET, get_handler_, this, false, false, nullptr};
  httpd_uri_t direct_post{"/rpc/*", HTTP_POST, post_handler_, this, false, false, nullptr};
  httpd_uri_t unknown_get{"/*", HTTP_GET, not_found_handler_, this, false, false, nullptr};
  httpd_uri_t unknown_post{"/*", HTTP_POST, not_found_handler_, this, false, false, nullptr};
  httpd_register_uri_handler(server_, &index);
  httpd_register_uri_handler(server_, &status);
  httpd_register_uri_handler(server_, &pairing);
  httpd_register_uri_handler(server_, &reboot);
  httpd_register_uri_handler(server_, &factory_reset);
  httpd_register_uri_handler(server_, &shelly);
  httpd_register_uri_handler(server_, &rpc_post);
  httpd_register_uri_handler(server_, &rpc_ws);
  httpd_register_uri_handler(server_, &direct_get);
  httpd_register_uri_handler(server_, &direct_post);
  httpd_register_uri_handler(server_, &unknown_get);
  httpd_register_uri_handler(server_, &unknown_post);
  initial_broker_apply_pending_ = true;
}

void ShellyPro3EMEmu::publish_status_() {
  if (!source_ready_())
    return;
  JsonDocument em;
  em_status_(em.to<JsonObject>());
  JsonDocument emdata;
  emdata_status_(emdata.to<JsonObject>());
  String payload;
  serializeJson(em, payload);
  const bool em_queued =
      broker_->publish(std::string(prefix_().c_str()) + "/status/em:0", payload.c_str(), payload.length(), 1, false);
  payload.clear();
  serializeJson(emdata, payload);
  const bool emdata_queued =
      broker_->publish(std::string(prefix_().c_str()) + "/status/emdata:0", payload.c_str(), payload.length(), 1, false);
  if (em_queued && emdata_queued) {
    status_publish_successes_++;
    last_status_publish_uptime_s_ = millis() / 1000;
  } else {
    status_publish_failures_++;
    ESP_LOGW(TAG, "Could not queue complete meter status to MQTT");
  }
}

void ShellyPro3EMEmu::publish_notification_(const char *component) {
  if (!settings_.rpc_ntf || !source_ready_() || clock_ == nullptr || !clock_->now().is_valid())
    return;
  JsonDocument notification;
  notification["src"] = id_();
  notification["dst"] = prefix_() + "/events";
  notification["method"] = "NotifyStatus";
  JsonObject params = notification["params"].to<JsonObject>();
  params["ts"] = clock_->now().timestamp;
  JsonObject value = params[component].to<JsonObject>();
  if (strcmp(component, "em:0") == 0) {
    em_status_(value);
    value.remove("user_calibrated_phase");
    value.remove("errors");
    for (const char phase : {'a', 'b', 'c'}) {
      const String key(phase);
      value.remove((key + "_errors").c_str());
      value.remove((key + "_flags").c_str());
    }
  } else {
    emdata_status_(value);
  }
  value.remove("id");
  String payload;
  serializeJson(notification, payload);
  if (broker_->publish(std::string(prefix_().c_str()) + "/events/rpc", payload.c_str(), payload.length(), 1, false))
    notification_publish_successes_++;
  else
    notification_publish_failures_++;
}

void ShellyPro3EMEmu::loop() {
  uint32_t reboot_after = reboot_after_.load();
  if (reboot_after != 0 && static_cast<int32_t>(millis() - reboot_after) >= 0 &&
      reboot_after_.compare_exchange_strong(reboot_after, 0))
    App.safe_reboot();
  if (initial_broker_apply_pending_ && wifi::global_wifi_component->is_connected()) {
    initial_broker_apply_pending_ = false;
    apply_broker_();
  }
  if (factory_reset_pending_.exchange(false)) {
    factory_reset_arm_deadline_ = 0;
    mqtt_apply_after_ = 0;
    factory_reset_();
  }
  uint32_t apply_after = mqtt_apply_after_.load();
  if (apply_after != 0 && static_cast<int32_t>(millis() - apply_after) >= 0 &&
      mqtt_apply_after_.compare_exchange_strong(apply_after, 0)) {
    mark_crash_stage("Mqtt.SetConfig", "apply");
    apply_broker_();
    mark_crash_stage("Mqtt.SetConfig", "applied");
  }
  update_pairing_();
  const bool connected = broker_->is_connected();
  if (connected && !was_connected_) {
    publish_announce_(false);
    last_publish_ = 0;
    last_energy_notification_ = 0;
  }
  was_connected_ = connected;
  const uint32_t now = millis();
  if (connected && source_ready_() &&
      (last_publish_ == 0 || now - last_publish_ >= publish_interval_ms_)) {
    if (settings_.status_ntf)
      publish_status_();
    publish_notification_("em:0");
    if (last_energy_notification_ == 0 || now - last_energy_notification_ >= 60000) {
      publish_notification_("emdata:0");
      last_energy_notification_ = now;
    }
    last_publish_ = now;
  }
}

}  // namespace esphome::shelly_pro_3em_emu
