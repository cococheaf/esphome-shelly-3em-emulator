# ESPHome Shelly Pro 3EM Emulator

ESPHome firmware for an ESP32-WROOM-32 DevKit that presents Home Assistant meter values through a Shelly Pro 3EM-compatible local API and MQTT interface. SunEnergyXT onboarding was verified on the target ESP32 with an iPhone. This repository supports **ESP32 only**.

Version 1.1.0 adds source-update age, a browser-only power chart, diagnostics, and a separate key for web actions to the tested onboarding and live meter path. It is not original Shelly firmware and does not implement Shelly Cloud, calibration, historical EMData records, or every RPC method. Do not use it for billing or safety-critical control.

## Install

Use [`shelly-pro-3em-esp32.yaml`](shelly-pro-3em-esp32.yaml) in the Home Assistant ESPHome Device Builder. The file loads the custom component from this repository's `main` branch. Set the Wi-Fi, ESPHome API, OTA, and `emulator_action_key` secrets in your local `/config/esphome/secrets.yaml`; use a unique random 20-96 character action key. Never commit credentials or a firmware binary built with them. See [`FLASHEN.md`](FLASHEN.md) for flashing and pairing steps.

The public YAML uses DHCP. Assign a fixed address in your router or configure `wifi.manual_ip` only in your private ESPHome copy. The SolarEdge entity IDs are examples; change the mappings for your Home Assistant installation.

## Meter data

The ESP32 receives HA states over the ESPHome native API; it does not poll HA. Missing measurements are returned as JSON `null`, never invented. The example maps:

| Shelly value | Home Assistant source |
| --- | --- |
| Total signed power | `sensor.solaredge_modbuszahler_ac_power_inverted` |
| Phase voltage and current | SolarEdge Modbus meter A/B/C voltage and current sensors |
| Phase signed power | SolarEdge A/B/C power sensors, sign inverted for Shelly |
| Phase power factor | SolarEdge A/B/C power-factor sensors, divided by 100 |
| Total and phase import/export | SolarEdge kWh counters, converted to Wh |

Check the exact mappings in the YAML before flashing. The web page at `http://<device-ip>/` is explicitly labeled as a Shelly Pro 3EM emulator and shows HA, Wi-Fi, MQTT, meter status, a browser-only 30-minute power trace, and an expandable diagnosis; `/emu/status` provides the JSON diagnostic view. The local Shelly-compatible endpoints include `/shelly`, `/rpc`, `/rpc/EM.GetStatus?id=0`, and `/rpc/EMData.GetStatus?id=0`.

Each imported sensor's age since its last HA update is shown in `/emu/status` and as a tooltip on the web page. `stale_after` (default 5 minutes, allowed 30 seconds to 60 minutes) marks power measurements that have not changed recently; a constant healthy reading can therefore be marked as unchanged. This is a display warning, not proof that the source is offline, and it does not interrupt MQTT publishing. The HA connection check still prevents publishing after the API disconnects. Cumulative energy counters are not marked stale merely because they stay unchanged.

## SunEnergyXT and MQTT

Pairing starts a temporary `ShellyPro3EM-<MAC>` access point while the ESP32 remains on its existing Wi-Fi. Start it with the HA **Shelly Pairing starten** button or the web page, connect the iPhone to that access point, then continue in SunEnergyXT. The app configures MQTT through `MQTT.SetConfig`; broker settings persist across OTA updates.

The emulator supports the Shelly MQTT RPC request/response channel, `announce` and `status_update` control commands, and QoS 1 `NotifyStatus` on `<topic_prefix>/events/rpc`. Power is published at `meter_publish_interval` (default 1 second, configurable from 1 to 60 seconds); energy counters are published every 60 seconds. Complete component status topics are sent only when `status_ntf` is enabled. HA time supplies the notification timestamp.

The HA **Shelly Werkseinstellungen (2x druecken)** button or confirmed web reset clears only the emulated broker settings and reopens pairing. It does not erase ESPHome Wi-Fi, API credentials, firmware, or sensor mappings. Do not reset an already paired device unless you intend to pair it again.

The HA **Shelly Emulator Neustart** button and the web page's confirmed **Neustarten** button reboot the ESP32 without clearing its saved broker settings.

## Security and diagnostics

The `/emu/pairing`, `/emu/reboot`, and `/emu/factory-reset` web actions require the configured action key. The page prompts for it on first use and keeps it only in that browser tab's memory. This protects those emulator-specific actions from accidental local use, but HTTP is unencrypted and the Shelly-compatible RPC API and temporary pairing AP remain unauthenticated for app compatibility. Keep the ESP32 on a trusted network and close pairing after use. MQTT credentials supplied by the app are stored on the device; the MQTT transport used here is unencrypted. Firmware images compiled with your HA secrets must stay private.

The release YAML uses `logger.level: INFO`. Set `WARN` or `DEBUG` temporarily to trace RPC method names and redacted payloads in ESPHome logs, then restore `INFO`. `/emu/status` reports connection state and publish queue counters; a queued MQTT message alone does not prove that a remote service accepted it.
