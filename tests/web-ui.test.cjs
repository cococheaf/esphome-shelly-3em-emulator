const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const { chromium } = require('playwright');

const header = fs.readFileSync(path.join(__dirname, '../components/shelly_pro_3em_emu/web_ui.h'), 'utf8');
const html = header.match(/R"HTML\(([\s\S]*?)\)HTML"/)[1];

async function main() {
  const browser = await chromium.launch({ headless: true });
  try {
    for (const viewport of [
      { name: 'desktop', width: 1440, height: 900 },
      { name: 'ipad-8', width: 810, height: 1080 },
      { name: 'iphone', width: 390, height: 844 },
    ]) {
      const page = await browser.newPage({ viewport });
      let requests = 0;
      let actionKey = null;
      page.on('dialog', dialog => dialog.accept('test-action-key'));
      await page.route('**/*', route => {
        const request = route.request();
        const url = new URL(request.url());
        if (url.pathname === '/emu/status' || url.pathname === '/emu/pairing') {
          if (url.pathname === '/emu/pairing') actionKey = request.headers()['x-emulator-key'];
          requests++;
          const sample = {
            model: 'Shelly Pro 3EM Gen2', id: 'test', power: requests % 2 ? 1350 : -950,
            power_age_s: viewport.name === 'iphone' ? 400 : 3, import_wh: 1234500, import_age_s: 12,
            export_wh: 225000, export_age_s: 18, ha_connected: true,
            measurements_ready: true, measurement_stale: viewport.name === 'iphone', stale_after_s: 300,
            mqtt_enabled: true, mqtt_connected: true, mqtt_server: 'test-broker',
            wifi_connected: true, wifi_ip: '192.0.2.10', wifi_rssi: -49,
            pairing_active: url.pathname === '/emu/pairing', pairing_ssid: 'ShellyPro3EM-TEST',
            uptime_s: 3600, reset_reason: 'power_on',
            diagnostic: { status_publish_successes: 10, status_publish_failures: 0,
              notification_publish_successes: 20, notification_publish_failures: 1,
              last_status_publish_uptime_s: 3590, mqtt_rpc_received: 2,
              mqtt_control_received: 3, publish_interval_ms: 1000 },
            phases: Array.from({ length: 3 }, () => ({ power: 450, power_age_s: 3,
              voltage: 230, voltage_age_s: 3, current: 2, current_age_s: 3,
              pf: 0.98, pf_age_s: 3, import_wh: 411500, import_age_s: 12,
              export_wh: 75000, export_age_s: 18 })),
          };
          return route.fulfill({ status: 200, contentType: 'application/json', body: JSON.stringify(sample) });
        }
        return route.fulfill({ status: 200, contentType: 'text/html', body: html });
      });
      await page.goto('http://device.test/');
      await page.getByText(viewport.name === 'iphone' ?
        'Verbunden, Leistung laenger unveraendert' : 'Verbunden, Messwerte aktiv').waitFor();
      if (viewport.name === 'iphone')
        assert.equal(await page.locator('.power').evaluate(node => node.classList.contains('stale')), true);
      await page.locator('.diagnostics summary').click();
      assert.equal(await page.locator('#diag-rpc').textContent(), '2 / 3');
      assert.equal(await page.evaluate(() => document.documentElement.scrollWidth <= window.innerWidth), true,
        `${viewport.name}: horizontal overflow`);
      if (viewport.name === 'desktop') {
        await page.locator('#pairing-button').click();
        assert.equal(actionKey, 'test-action-key');
        await page.getByText('ShellyPro3EM-TEST / 192.168.33.1').waitFor();
        assert.equal(await page.locator('#pairing-status').textContent(), 'ShellyPro3EM-TEST / 192.168.33.1');
        await page.getByText('Verlauf nur in diesem Browser, nicht gespeichert.').waitFor();
      }
      await page.screenshot({ path: `/tmp/shelly-emulator-${viewport.name}.png`, fullPage: true });
      await page.close();
    }
  } finally {
    await browser.close();
  }
}

main().catch(error => { console.error(error); process.exitCode = 1; });
