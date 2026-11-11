#pragma once

static const char WEB_UI_HTML[] PROGMEM = R"HTML(<!doctype html>
<html lang="de">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<meta name="theme-color" content="#111a1e">
<title>Shelly Pro 3EM Emulator</title>
<style>
:root{color-scheme:dark;font-family:system-ui,-apple-system,BlinkMacSystemFont,"Segoe UI",sans-serif;font-size:16px;background:#10191d;color:#edf3f0}
*{box-sizing:border-box}
body{margin:0;min-width:280px}
button{font:inherit}
.shell{max-width:1200px;margin:auto;padding:24px clamp(16px,3vw,36px) 38px}
header{display:flex;align-items:center;justify-content:space-between;gap:16px;padding-bottom:18px;border-bottom:1px solid #334247}
.brand{display:flex;align-items:center;gap:12px;min-width:0}
.mark{display:grid;place-items:center;width:35px;height:35px;border-radius:7px;background:#a6e37c;color:#102014;font-weight:800}
.brand strong{font-size:15px;line-height:1.15}
.brand span{display:block;font-size:12px;color:#9bb0b3;margin-top:2px}
.sync{display:flex;align-items:center;gap:8px;color:#9bb0b3;font-size:12px;white-space:nowrap}
.dot{display:inline-block;width:8px;height:8px;border-radius:50%;background:#8a9b9d;flex:none}
.dot.ok{background:#a6e37c;box-shadow:0 0 0 4px #a6e37c1c}
.dot.warn{background:#efc47f}
.dot.off{background:#ed8e7e}
h1{font-size:32px;line-height:1.12;margin:28px 0 5px;font-weight:650}
.sub{font-size:14px;color:#9bb0b3;margin:0 0 24px}
.top{display:grid;grid-template-columns:minmax(0,1.65fr) minmax(280px,1fr);gap:14px}
.panel{border:1px solid #34444a;border-radius:7px;background:#1b292e}
.overview{padding:22px;min-width:0}
.label{color:#aababe;font-size:12px;font-weight:650;text-transform:uppercase}
.power{font-size:62px;line-height:1.1;font-weight:650;font-variant-numeric:tabular-nums;margin:15px 0 4px;white-space:nowrap}
.power .unit{font-size:.43em;color:#aababe;margin-left:5px}
.direction{color:#aababe;font-size:13px;min-height:19px}
.totals{display:grid;grid-template-columns:1fr 1fr;gap:16px;margin-top:25px;padding-top:19px;border-top:1px solid #34444a}
.totals strong{display:block;margin-top:5px;font-size:23px;font-variant-numeric:tabular-nums;font-weight:600}
.totals .unit{font-size:13px;color:#aababe}
.connections{padding:19px 20px}
.connections h2,.phases h2{margin:0;font-size:16px;font-weight:650}
.connection{display:flex;gap:12px;align-items:flex-start;padding:15px 0;border-bottom:1px solid #34444a}
.connection:last-child{border:0;padding-bottom:0}
.connection .dot{margin-top:6px}
.connection strong{display:block;font-size:14px;font-weight:600}
.connection span{display:block;margin-top:3px;color:#aababe;font-size:12px;overflow-wrap:anywhere}
.pairing-action{margin-top:10px;border:1px solid #7cb764;background:#263f36;color:#d9f3c9;border-radius:5px;padding:8px 12px;font-size:12px;font-weight:650;cursor:pointer;min-height:36px}
.pairing-action:hover:not(:disabled){background:#345345}
.pairing-action:disabled{opacity:.48;cursor:default}
.reboot-action{margin-top:10px;border:1px solid #5c8791;background:#26383d;color:#d5ebee;border-radius:5px;padding:8px 12px;font-size:12px;font-weight:650;cursor:pointer;min-height:36px}
.reboot-action:hover:not(:disabled){background:#32494f}
.reboot-action:disabled{opacity:.48;cursor:default}
.reset-action{margin-top:10px;border:1px solid #9a625b;background:#3a292a;color:#f4d4cf;border-radius:5px;padding:8px 12px;font-size:12px;font-weight:650;cursor:pointer;min-height:36px}
.reset-action:hover:not(:disabled){background:#523234}
.reset-action:disabled{opacity:.48;cursor:default}
.phases{margin-top:28px}
.phases h2{margin-bottom:13px}
.phase-grid{display:grid;grid-template-columns:repeat(3,minmax(0,1fr));gap:14px}
.phase{padding:19px;min-width:0}
.phase-head{display:flex;align-items:center;gap:9px}
.phase-letter{display:grid;place-items:center;width:27px;height:27px;border-radius:5px;font-size:12px;font-weight:700;background:#314c4a;color:#b4f0dc}
.phase:nth-child(2) .phase-letter{background:#50483c;color:#f4d4a3}
.phase:nth-child(3) .phase-letter{background:#4a424a;color:#f2cbdc}
.phase-head strong{font-size:14px}
.phase-power{font-size:30px;font-variant-numeric:tabular-nums;font-weight:600;margin:15px 0}
.phase-power small{font-size:13px;color:#aababe;font-weight:400}
.metric-grid{display:grid;grid-template-columns:1fr 1fr;gap:13px 8px;padding-top:15px;border-top:1px solid #34444a}
.metric-grid span{display:block;color:#9bb0b3;font-size:11px}
.metric-grid strong{display:block;margin-top:3px;font-size:14px;font-weight:550;font-variant-numeric:tabular-nums}
footer{display:flex;justify-content:space-between;gap:14px;flex-wrap:wrap;margin-top:25px;color:#8fa2a5;font-size:11px}
@media(max-width:760px){.top{grid-template-columns:1fr}.phase-grid{grid-template-columns:1fr}.phase{display:grid;grid-template-columns:1fr 1fr;gap:0 14px}.phase-head,.phase-power{grid-column:1}.metric-grid{grid-column:2;grid-row:1 / span 2;padding:0 0 0 14px;border-top:0;border-left:1px solid #34444a}.phase-power{margin-bottom:0}.connections{padding:18px}}
@media(max-width:480px){.shell{padding-top:14px}.brand strong{font-size:13px}.sync{font-size:11px}h1{font-size:26px}.power{font-size:44px}.phase-power{font-size:26px}.phase{display:block}.phase-power{margin-bottom:13px}.metric-grid{padding:15px 0 0;border-left:0;border-top:1px solid #34444a}.totals strong{font-size:19px}}
</style>
</head>
<body>
<div class="shell">
<header>
  <div class="brand"><div class="mark">3</div><div><strong>SHELLY-EMULATOR</strong><span id="model">ESPHome / Pro 3EM</span></div></div>
  <div class="sync"><i id="sync-dot" class="dot warn"></i><span id="sync-text">Verbinde ...</span></div>
</header>
<main>
<h1>Shelly Pro 3EM Emulator</h1>
<p class="sub">Netz&uuml;bersicht / ESPHome <span id="device-id"></span></p>
<div class="top">
  <section class="panel overview">
    <div class="label">Netzleistung</div>
    <div class="power"><span id="total-power">--</span><span id="total-power-unit" class="unit">W</span></div>
    <div id="direction" class="direction">Warte auf Messwerte</div>
    <div class="totals">
      <div><div class="label">Bezug</div><strong id="import-energy">--</strong><span class="unit">kWh</span></div>
      <div><div class="label">Einspeisung</div><strong id="export-energy">--</strong><span class="unit">kWh</span></div>
    </div>
  </section>
  <section class="panel connections">
    <h2>Verbindungen</h2>
    <div class="connection"><i id="ha-dot" class="dot"></i><div><strong>Home Assistant</strong><span id="ha-status">Pr&uuml;fe Verbindung</span></div></div>
    <div class="connection"><i id="mqtt-dot" class="dot"></i><div><strong>MQTT</strong><span id="mqtt-status">Pr&uuml;fe Verbindung</span></div></div>
    <div class="connection"><i id="wifi-dot" class="dot"></i><div><strong>WLAN</strong><span id="wifi-status">Pr&uuml;fe Verbindung</span></div></div>
    <div class="connection"><i id="pairing-dot" class="dot"></i><div><strong>Pairing</strong><span id="pairing-status">Inaktiv</span><button type="button" id="pairing-button" class="pairing-action" disabled>Pairing starten</button></div></div>
    <div class="connection"><i class="dot"></i><div><strong>Ger&auml;t</strong><span id="reboot-status">ESP32 neu starten</span><button type="button" id="reboot-button" class="reboot-action">Neustarten</button></div></div>
    <div class="connection"><i class="dot warn"></i><div><strong>Werkseinstellungen</strong><span id="reset-status">MQTT-Konfiguration entfernen und Pairing neu starten</span><button type="button" id="reset-button" class="reset-action">Zur&uuml;cksetzen</button></div></div>
  </section>
</div>
<section class="phases">
  <h2>Phasen</h2>
  <div class="phase-grid">
    <article class="panel phase"><div class="phase-head"><span class="phase-letter">L1</span><strong>Phase 1</strong></div><div class="phase-power"><span id="p0">--</span> <small>W</small></div><div class="metric-grid"><div><span>Spannung</span><strong id="v0">--</strong></div><div><span>Strom</span><strong id="c0">--</strong></div><div><span>Leistungsfaktor</span><strong id="f0">--</strong></div><div><span>Bezug / Abgabe</span><strong id="e0">--</strong></div></div></article>
    <article class="panel phase"><div class="phase-head"><span class="phase-letter">L2</span><strong>Phase 2</strong></div><div class="phase-power"><span id="p1">--</span> <small>W</small></div><div class="metric-grid"><div><span>Spannung</span><strong id="v1">--</strong></div><div><span>Strom</span><strong id="c1">--</strong></div><div><span>Leistungsfaktor</span><strong id="f1">--</strong></div><div><span>Bezug / Abgabe</span><strong id="e1">--</strong></div></div></article>
    <article class="panel phase"><div class="phase-head"><span class="phase-letter">L3</span><strong>Phase 3</strong></div><div class="phase-power"><span id="p2">--</span> <small>W</small></div><div class="metric-grid"><div><span>Spannung</span><strong id="v2">--</strong></div><div><span>Strom</span><strong id="c2">--</strong></div><div><span>Leistungsfaktor</span><strong id="f2">--</strong></div><div><span>Bezug / Abgabe</span><strong id="e2">--</strong></div></div></article>
  </div>
</section>
</main>
<footer><span>Nur Anzeige &middot; keine Messhardware</span><span id="updated">Noch nicht aktualisiert</span></footer>
</div>
<script>
(function(){
  function put(id,value){document.getElementById(id).textContent=value}
  function num(value,digits){return typeof value==='number'&&isFinite(value)?value.toLocaleString('de-AT',{minimumFractionDigits:digits,maximumFractionDigits:digits}):'--'}
  function energy(value){return typeof value==='number'&&isFinite(value)?num(value/1000,1):'--'}
  function withUnit(value,digits,unit){return typeof value==='number'&&isFinite(value)?num(value,digits)+' '+unit:'--'}
  function dot(id,state){document.getElementById(id).className='dot '+state}
  function render(data){
    put('model',data.model||'Emulator');
    put('device-id',data.id?' / '+data.id:'');
    var p=data.power;
    if(typeof p==='number'&&isFinite(p)){
      var kw=Math.abs(p)>=1000;
      put('total-power',num(kw?p/1000:p,kw?2:0));
      put('total-power-unit',kw?'kW':'W');
      put('direction',p>0?'Bezug aus dem Netz':p<0?'Einspeisung ins Netz':'Kein Netzfluss');
    }else{put('total-power','--');put('total-power-unit','W');put('direction','Messwert nicht verfuegbar')}
    put('import-energy',energy(data.import_wh));
    put('export-energy',energy(data.export_wh));
    dot('ha-dot',data.ha_connected?(data.measurements_ready?'ok':'warn'):'off');
    put('ha-status',data.ha_connected?(data.measurements_ready?'Verbunden, Messwerte aktiv':'Verbunden, Messwerte fehlen'):'Nicht verbunden');
    dot('mqtt-dot',!data.mqtt_enabled?'warn':data.mqtt_connected?'ok':'off');
    put('mqtt-status',!data.mqtt_enabled?'Nicht eingerichtet':data.mqtt_connected?'Verbunden mit '+(data.mqtt_server||'Broker'):'Getrennt von '+(data.mqtt_server||'Broker'));
    dot('wifi-dot',data.wifi_connected?'ok':'off');
    put('wifi-status',data.wifi_connected?(data.wifi_ip||'Verbunden')+' / '+(data.wifi_rssi===null?'--':data.wifi_rssi+' dBm'):'Nicht verbunden');
    dot('pairing-dot',data.pairing_active?'ok':'off');
    put('pairing-status',data.pairing_active?(data.pairing_ssid||'Aktiv')+' / 192.168.33.1':'Inaktiv');
    var pairButton=document.getElementById('pairing-button');
    pairButton.disabled=!!data.pairing_active;
    pairButton.textContent=data.pairing_active?'Pairing aktiv':'Pairing starten';
    for(var i=0;i<3;i++){
      var x=(data.phases||[])[i]||{};
      put('p'+i,num(x.power,0));put('v'+i,withUnit(x.voltage,1,'V'));
      put('c'+i,withUnit(x.current,2,'A'));put('f'+i,num(x.pf,2));
      put('e'+i,x.import_wh===null&&x.export_wh===null?'--':energy(x.import_wh)+' / '+energy(x.export_wh)+' kWh');
    }
    dot('sync-dot','ok');put('sync-text','Live');
    put('updated','Aktualisiert '+new Date().toLocaleTimeString('de-AT'));
  }
  function offline(){dot('sync-dot','off');put('sync-text','Offline');put('updated','Verbindung zum Geraet unterbrochen')}
  var loading=false;
  document.getElementById('pairing-button').addEventListener('click',function(){
    var button=this;
    button.disabled=true;
    fetch('/emu/pairing',{method:'POST',headers:{'X-Emulator-Action':'pair'}})
      .then(function(r){if(!r.ok)throw Error('HTTP '+r.status);return r.json()})
      .then(render)
      .catch(function(){put('pairing-status','Start fehlgeschlagen');button.disabled=false});
  });
  document.getElementById('reset-button').addEventListener('click',function(){
    if(window.prompt('MQTT-Verbindung wird geloescht. Zum Bestaetigen RESET eingeben:')!=='RESET')return;
    var button=this;
    button.disabled=true;
    fetch('/emu/factory-reset',{method:'POST',headers:{'Content-Type':'application/json','X-Emulator-Action':'factory-reset'},body:JSON.stringify({confirm:'RESET'})})
      .then(function(r){if(!r.ok)throw Error('HTTP '+r.status);return r.json()})
      .then(function(){put('reset-status','Zurueckgesetzt. Pairing wird gestartet.');poll()})
      .catch(function(){put('reset-status','Reset fehlgeschlagen');button.disabled=false});
  });
  document.getElementById('reboot-button').addEventListener('click',function(){
    if(!window.confirm('ESP32-Emulator jetzt neu starten?'))return;
    var button=this;
    button.disabled=true;
    fetch('/emu/reboot',{method:'POST',headers:{'X-Emulator-Action':'reboot'}})
      .then(function(r){if(!r.ok)throw Error('HTTP '+r.status);put('reboot-status','Neustart laeuft ...')})
      .catch(function(){put('reboot-status','Neustart fehlgeschlagen');button.disabled=false});
  });
  function poll(){
    if(loading)return;
    loading=true;
    fetch('/emu/status',{cache:'no-store'}).then(function(r){if(!r.ok)throw Error('HTTP '+r.status);return r.json()})
      .then(render).catch(offline).then(function(){loading=false});
  }
  poll();setInterval(poll,5000);
})();
</script>
</body>
</html>)HTML";
