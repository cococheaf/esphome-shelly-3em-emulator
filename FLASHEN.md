# ESP32 Shelly Pro 3EM Emulator flashen

Unterstuetzt wird ein ESP32-WROOM-32 DevKit mit USB. Die Firmware ist kein geeichter oder sicherheitsrelevanter Zaehler.

1. [`shelly-pro-3em-esp32.yaml`](shelly-pro-3em-esp32.yaml) in den ESPHome Device Builder von Home Assistant uebernehmen. In `/config/esphome/secrets.yaml` muessen `wifi_ssid`, `wifi_password`, `api_encryption_key` und `ota_password` gesetzt sein. Die oeffentliche YAML verwendet DHCP; eine feste Adresse nur in der privaten Kopie oder im Router einrichten. Die SolarEdge-Entitaeten vor Nutzung anpassen.
2. **Validate** und **Install** ausfuehren. Fuer den ersten Flash das DevKit per USB-Datenkabel anschliessen. Web Serial benoetigt einen unterstuetzten Browser und einen sicheren HA-Zugriff; alternativ das Factory-Binary aus dem Builder mit [ESPHome Web](https://web.esphome.io/) flashen. Spaetere Updates gehen ueber **Install > Im Netzwerk** (OTA).
3. Das neue ESPHome-Geraet in HA einbinden. Auf `http://<geraete-ip>/` muessen HA und die Messwerte bereit sein. `/shelly` muss `SPEM-003CEBEU` und `gen: 2` melden. `/rpc/EM.GetStatus?id=0` und `/rpc/EMData.GetStatus?id=0` mit dem eigenen Zaehler vergleichen.
4. **Shelly Pairing starten** in HA oder auf der lokalen Webseite druecken. Das iPhone mit `ShellyPro3EM-<MAC>` verbinden und in SunEnergyXT fortfahren. Die App richtet den MQTT-Broker ein; die erfolgreich getestete Kopplung benoetigt keine manuelle Broker-Konfiguration.

Die Weboberflaeche und der temporaere Access Point sind nicht authentifiziert. Nur in einem vertrauenswuerdigen Netz betreiben. Ein Firmware-Image mit echten WLAN- oder API-Zugangsdaten niemals in ein oeffentliches GitHub-Release hochladen. Bei Problemen zeigen `/emu/status` und die ESPHome-Logs Verbindungen und MQTT-Sendezaehler; fuer RPC-Diagnose `logger.level` voruebergehend auf `WARN` oder `DEBUG` setzen.

Die Webseite zeigt klar **Shelly Pro 3EM Emulator**. Der bestaetigte Knopf **Neustarten** sowie der HA-Button **Shelly Emulator Neustart** starten den ESP32 neu, ohne die SunEnergyXT-MQTT-Konfiguration zu loeschen.
