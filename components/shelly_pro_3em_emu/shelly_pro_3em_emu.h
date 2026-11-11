#pragma once

#include <array>
#include <atomic>
#include <string>

#include <ArduinoJson.h>
#include <Arduino.h>
#include <esp_http_server.h>
#include <esp_netif.h>

#include "esphome/components/mqtt/mqtt_client.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/time/real_time_clock.h"
#include "esphome/core/component.h"
#include "esphome/core/preferences.h"

namespace esphome::shelly_pro_3em_emu {

struct ProMqttSettings {
  uint32_t magic{0};
  char server[96]{};
  char user[64]{};
  char password[96]{};
  char client_id[64]{};
  char topic_prefix[96]{};
  uint8_t enabled{0};
  uint8_t rpc_ntf{1};
  uint8_t status_ntf{0};
};

class ShellyPro3EMEmu : public Component {
 public:
  explicit ShellyPro3EMEmu(mqtt::MQTTClientComponent *broker) : broker_(broker) {}

  void set_total_power(sensor::Sensor *value) { total_power_ = value; }
  void set_import_energy(sensor::Sensor *value) { import_energy_ = value; }
  void set_export_energy(sensor::Sensor *value) { export_energy_ = value; }
  void set_publish_interval(uint32_t interval_ms) { publish_interval_ms_ = interval_ms; }
  void set_clock(time::RealTimeClock *clock) { clock_ = clock; }
  void set_voltage(size_t i, sensor::Sensor *value) { phases_[i].voltage = value; }
  void set_current(size_t i, sensor::Sensor *value) { phases_[i].current = value; }
  void set_power(size_t i, sensor::Sensor *value) { phases_[i].power = value; }
  void set_power_factor(size_t i, sensor::Sensor *value) { phases_[i].power_factor = value; }
  void set_phase_import_energy(size_t i, sensor::Sensor *value) { phases_[i].import_energy = value; }
  void set_phase_export_energy(size_t i, sensor::Sensor *value) { phases_[i].export_energy = value; }

  void setup() override;
  void loop() override;
  void start_pairing();
  void press_factory_reset_button();
  bool pairing_active() const { return pairing_active_; }
  float get_setup_priority() const override { return setup_priority::AFTER_CONNECTION; }

 protected:
  struct Phase {
    sensor::Sensor *voltage{nullptr};
    sensor::Sensor *current{nullptr};
    sensor::Sensor *power{nullptr};
    sensor::Sensor *power_factor{nullptr};
    sensor::Sensor *import_energy{nullptr};
    sensor::Sensor *export_energy{nullptr};
  };

  static bool valid_(sensor::Sensor *sensor);
  static float value_(sensor::Sensor *sensor);
  static void copy_(char *target, size_t size, const String &value);
  String mac_() const;
  String id_() const;
  String prefix_() const;
  bool source_ready_() const;
  void device_info_(JsonObject out) const;
  void em_status_(JsonObject out) const;
  void emdata_status_(JsonObject out) const;
  void mqtt_config_(JsonObject out) const;
  void mqtt_status_(JsonObject out) const;
  void sys_config_(JsonObject out) const;
  void sys_status_(JsonObject out) const;
  void wifi_status_(JsonObject out) const;
  void wifi_config_(JsonObject out) const;
  int wifi_scan_(JsonObject out) const;
  int wifi_set_config_(JsonVariantConst config, JsonObject out);
  void update_pairing_();
  void factory_reset_();
  String dashboard_json_() const;
  void record_request_(const String &path, const String &method, int error);
  void apply_broker_();
  void publish_announce_(bool broadcast);
  void publish_full_status_();
  int mqtt_set_config_(JsonVariantConst config, JsonObject out);
  int dispatch_(const String &method, JsonVariantConst params, JsonObject out);
  String rpc_frame_(JsonVariantConst frame, bool track_http = true);
  String rpc_direct_(const String &method, JsonVariantConst params);
  static esp_err_t get_handler_(httpd_req_t *request);
  static esp_err_t post_handler_(httpd_req_t *request);
  static esp_err_t websocket_handler_(httpd_req_t *request);
  static esp_err_t not_found_handler_(httpd_req_t *request);
  esp_err_t handle_get_(httpd_req_t *request);
  esp_err_t handle_post_(httpd_req_t *request);
  esp_err_t handle_websocket_(httpd_req_t *request);
  static esp_err_t send_json_(httpd_req_t *request, const String &body);
  void publish_status_();
  void publish_notification_(const char *component);

  mqtt::MQTTClientComponent *broker_;
  time::RealTimeClock *clock_{nullptr};
  httpd_handle_t server_{nullptr};
  esp_netif_t *ap_netif_{nullptr};
  sensor::Sensor *total_power_{nullptr};
  sensor::Sensor *import_energy_{nullptr};
  sensor::Sensor *export_energy_{nullptr};
  std::array<Phase, 3> phases_{};
  ProMqttSettings settings_{};
  ESPPreferenceObject preference_;
  std::array<std::string, 3> subscribed_topics_{};
  uint32_t last_publish_{0};
  uint32_t last_energy_notification_{0};
  uint32_t publish_interval_ms_{1000};
  uint32_t status_publish_successes_{0};
  uint32_t status_publish_failures_{0};
  uint32_t last_status_publish_uptime_s_{0};
  uint32_t announce_publish_successes_{0};
  uint32_t notification_publish_successes_{0};
  uint32_t notification_publish_failures_{0};
  std::atomic<uint32_t> mqtt_rpc_received_{0};
  std::atomic<uint32_t> mqtt_control_received_{0};
  bool was_connected_{false};
  bool initial_broker_apply_pending_{false};
  std::atomic<bool> pairing_active_{false};
  std::atomic<bool> factory_reset_pending_{false};
  std::atomic<uint32_t> reboot_after_{0};
  std::atomic<uint32_t> mqtt_apply_after_{0};
  uint32_t factory_reset_arm_deadline_{0};
  bool pairing_ap_started_{false};
  std::atomic<uint32_t> pairing_deadline_{0};
  std::atomic<uint32_t> last_ap_check_{0};
  std::atomic<uint32_t> pairing_stop_after_{0};
  struct DiagnosticEntry {
    char path[48]{};
    char method[48]{};
    int error{0};
    uint32_t count{0};
    uint32_t uptime_s{0};
  };
  std::array<DiagnosticEntry, 32> recent_requests_{};
  size_t request_head_{0};
  size_t request_count_{0};
};

}  // namespace esphome::shelly_pro_3em_emu
