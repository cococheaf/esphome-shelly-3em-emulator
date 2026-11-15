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
.left-column{display:grid;align-content:start;gap:14px;min-width:0}
.panel{border:1px solid #34444a;border-radius:7px;background:#1b292e}
.overview{padding:22px;min-width:0}
.label{color:#aababe;font-size:12px;font-weight:650;text-transform:uppercase}
.power{font-size:62px;line-height:1.1;font-weight:650;font-variant-numeric:tabular-nums;margin:15px 0 4px;white-space:nowrap}
.power .unit{font-size:.43em;color:#aababe;margin-left:5px}
.direction{color:#aababe;font-size:13px;min-height:19px}
.source-age{color:#8fa2a5;font-size:12px;margin-top:8px}
.source-age.stale,.direction.stale{color:#efc47f}
.power.stale{color:#efc47f}
.stale-value{color:#efc47f!important}
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
.history,.diagnostics{margin-top:14px;padding:19px 20px}
.left-column .history{margin-top:0}
.history-head{display:flex;align-items:baseline;justify-content:space-between;gap:12px}
.history h2,.diagnostics summary{font-size:16px;font-weight:650;margin:0}
.history-head span,.history-note{color:#8fa2a5;font-size:12px}
.history canvas{display:block;width:100%;height:150px;margin-top:15px}
.history-note{margin-top:8px}
.diagnostics summary{cursor:pointer;list-style:none}
.diagnostics summary::-webkit-details-marker{display:none}
.diagnostics summary::after{content:'+';float:right;color:#a6e37c;font-size:20px;line-height:1}
.diagnostics[open] summary::after{content:'-'}
.diag-grid{display:grid;grid-template-columns:repeat(3,minmax(0,1fr));gap:15px 20px;margin-top:18px;padding-top:17px;border-top:1px solid #34444a}
.diag-grid div{min-width:0}.diag-grid dt{color:#9bb0b3;font-size:11px}.diag-grid dd{margin:4px 0 0;font-size:14px;overflow-wrap:anywhere;font-variant-numeric:tabular-nums}
footer{display:flex;justify-content:space-between;gap:14px;flex-wrap:wrap;margin-top:25px;color:#8fa2a5;font-size:11px}
@media(max-width:760px){.top{grid-template-columns:1fr}.phase-grid{grid-template-columns:1fr}.phase{display:grid;grid-template-columns:1fr 1fr;gap:0 14px}.phase-head,.phase-power{grid-column:1}.metric-grid{grid-column:2;grid-row:1 / span 2;padding:0 0 0 14px;border-top:0;border-left:1px solid #34444a}.phase-power{margin-bottom:0}.connections{padding:18px}.diag-grid{grid-template-columns:repeat(2,minmax(0,1fr))}}
@media(max-width:480px){.shell{padding-top:14px}.brand strong{font-size:13px}.sync{font-size:11px}h1{font-size:26px}.power{font-size:44px}.phase-power{font-size:26px}.phase{display:block}.phase-power{margin-bottom:13px}.metric-grid{padding:15px 0 0;border-left:0;border-top:1px solid #34444a}.totals strong{font-size:19px}.diag-grid{grid-template-columns:1fr 1fr}.history,.diagnostics{padding:17px}}
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
  <div class="left-column">
  <section class="panel overview">
    <div class="label">Netzleistung</div>
    <div class="power"><span id="total-power">--</span><span id="total-power-unit" class="unit">W</span></div>
    <div id="direction" class="direction">Warte auf Messwerte</div>
    <div id="source-age" class="source-age">Letzte Aktualisierung: --</div>
    <div class="totals">
      <div><div class="label">Bezug</div><strong id="import-energy">--</strong><span class="unit">kWh</span></div>
      <div><div class="label">Einspeisung</div><strong id="export-energy">--</strong><span class="unit">kWh</span></div>
    </div>
  </section>
  <section class="panel history">
    <div class="history-head"><h2>Netzleistung</h2><span>Bis zu 30 Minuten</span></div>
    <canvas id="power-history" role="img" aria-label="Fl&uuml;chtiger Verlauf der Netzleistung"></canvas>
    <div id="history-note" class="history-note">Verlauf beginnt mit dem &Ouml;ffnen dieser Seite.</div>
  </section>
  </div>
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
<details class="panel diagnostics">
  <summary>Diagnose</summary>
  <dl class="diag-grid">
    <div><dt>Laufzeit</dt><dd id="diag-uptime">--</dd></div>
    <div><dt>Letzter Neustart</dt><dd id="diag-reset">--</dd></div>
    <div><dt>MQTT-Statussendungen</dt><dd id="diag-status">--</dd></div>
    <div><dt>MQTT-Benachrichtigungen</dt><dd id="diag-notify">--</dd></div>
    <div><dt>Letzte Statussendung</dt><dd id="diag-last-status">--</dd></div>
    <div><dt>MQTT-RPC / Steuerung</dt><dd id="diag-rpc">--</dd></div>
    <div><dt>Messwertintervall</dt><dd id="diag-interval">--</dd></div>
    <div><dt>Vorheriger Fehlerpunkt</dt><dd id="diag-crash">--</dd></div>
  </dl>
</details>
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
  function age(value){return typeof value==='number'?(value<60?value+' s':Math.floor(value/60)+' min'):'--'}
  function sample(id,value,limit,markStale){
    var node=document.getElementById(id);
    node.title=typeof value==='number'?'Letzte Aktualisierung vor '+age(value):'Kein Messwert empfangen';
    node.classList.toggle('stale-value',!!markStale&&(value===null||value>limit));
  }
  var history=[],windowMs=30*60*1000,canvas=document.getElementById('power-history');
  function drawHistory(){
    var width=canvas.clientWidth,height=canvas.clientHeight,scale=window.devicePixelRatio||1;
    if(width<1||height<1)return;
    canvas.width=Math.round(width*scale);canvas.height=Math.round(height*scale);
    var ctx=canvas.getContext('2d');ctx.scale(scale,scale);
    var now=Date.now(),span=Math.min(windowMs,Math.max(60000,now-(history[0]?history[0].t:now)));
    var start=now-span,values=history.filter(function(x){return x.p!==null}).map(function(x){return x.p});
    var bound=Math.max(500,Math.ceil(Math.max.apply(null,values.map(Math.abs).concat([0]))/500)*500);
    var left=5,right=width-5,top=8,bottom=height-18,mid=(top+bottom)/2,range=(bottom-top)/2;
    ctx.lineWidth=1;ctx.strokeStyle='#46595e';ctx.beginPath();ctx.moveTo(left,mid);ctx.lineTo(right,mid);ctx.stroke();
    ctx.fillStyle='#8fa2a5';ctx.font='11px system-ui';ctx.fillText('+'+num(bound/1000,1)+' kW',left,top+10);ctx.fillText('-'+num(bound/1000,1)+' kW',left,bottom);
    ctx.strokeStyle='#a6e37c';ctx.lineWidth=2;ctx.lineJoin='round';ctx.beginPath();
    var active=false,lastPoint=null;
    history.forEach(function(point){
      if(point.t<start||point.p===null){active=false;return}
      var x=left+(point.t-start)/span*(right-left),y=mid-point.p/bound*range;
      if(active)ctx.lineTo(x,y);else ctx.moveTo(x,y);
      active=true;lastPoint={x:x,y:y};
    });
    ctx.stroke();
    if(lastPoint){ctx.fillStyle='#a6e37c';ctx.beginPath();ctx.arc(lastPoint.x,lastPoint.y,3,0,Math.PI*2);ctx.fill()}
  }
  function addHistory(power){
    var now=Date.now();
    if(history.length&&now-history[history.length-1].t<4000)return;
    history.push({t:now,p:typeof power==='number'&&isFinite(power)?power:null});
    while(history.length&&history[0].t<now-windowMs)history.shift();
    drawHistory();
  }
  window.addEventListener('resize',drawHistory);
  function render(data){
    put('model',data.model||'Emulator');
    put('device-id',data.id?' / '+data.id:'');
    var p=data.power;
    var stale=!!data.measurement_stale;
    if(typeof p==='number'&&isFinite(p)){
      var kw=Math.abs(p)>=1000;
      put('total-power',num(kw?p/1000:p,kw?2:0));
      put('total-power-unit',kw?'kW':'W');
      put('direction',stale?'Messwert unveraendert / Aktualitaet pruefen':p>0?'Bezug aus dem Netz':p<0?'Einspeisung ins Netz':'Kein Netzfluss');
    }else{put('total-power','--');put('total-power-unit','W');put('direction','Messwert nicht verfuegbar')}
    document.querySelector('.power').classList.toggle('stale',stale);
    document.getElementById('direction').classList.toggle('stale',stale);
    put('source-age','Letzte Leistungsaktualisierung: '+age(data.power_age_s));
    document.getElementById('source-age').classList.toggle('stale',stale);
    sample('total-power',data.power_age_s,data.stale_after_s,true);
    put('import-energy',energy(data.import_wh));
    put('export-energy',energy(data.export_wh));
    sample('import-energy',data.import_age_s,data.stale_after_s,false);
    sample('export-energy',data.export_age_s,data.stale_after_s,false);
    dot('ha-dot',data.ha_connected?(data.measurements_ready&&!stale?'ok':'warn'):'off');
    put('ha-status',data.ha_connected?(data.measurements_ready?(stale?'Verbunden, Leistung laenger unveraendert':'Verbunden, Messwerte aktiv'):'Verbunden, Messwerte fehlen'):'Nicht verbunden');
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
      sample('p'+i,x.power_age_s,data.stale_after_s,true);
      sample('v'+i,x.voltage_age_s,data.stale_after_s,true);
      sample('c'+i,x.current_age_s,data.stale_after_s,true);
      sample('f'+i,x.pf_age_s,data.stale_after_s,true);
      document.getElementById('e'+i).title='Bezug: '+age(x.import_age_s)+' / Abgabe: '+age(x.export_age_s)+' seit letzter Aktualisierung';
    }
    var d=data.diagnostic||{};
    put('diag-uptime',age(data.uptime_s));
    put('diag-reset',data.reset_reason||'--');
    put('diag-status',num(d.status_publish_successes,0)+' erfolgreich / '+num(d.status_publish_failures,0)+' Fehler');
    put('diag-notify',num(d.notification_publish_successes,0)+' erfolgreich / '+num(d.notification_publish_failures,0)+' Fehler');
    put('diag-last-status',d.last_status_publish_uptime_s?age(Math.max(0,data.uptime_s-d.last_status_publish_uptime_s))+' zuvor':'Noch keine');
    put('diag-rpc',num(d.mqtt_rpc_received,0)+' / '+num(d.mqtt_control_received,0));
    put('diag-interval',typeof d.publish_interval_ms==='number'?num(d.publish_interval_ms/1000,1)+' s':'--');
    put('diag-crash',d.previous_method?(d.previous_method+' / '+(d.previous_stage||'?')):'Keiner');
    addHistory(data.measurements_ready&&!stale?p:null);
    put('history-note',history.length<2?'Verlauf beginnt mit dem Oeffnen dieser Seite.':'Verlauf nur in diesem Browser, nicht gespeichert.');
    dot('sync-dot',stale?'warn':'ok');put('sync-text',stale?'Pruefen':'Live');
    put('updated','Aktualisiert '+new Date().toLocaleTimeString('de-AT'));
  }
  function offline(){
    dot('sync-dot','off');put('sync-text','Offline');put('updated','Verbindung zum Geraet unterbrochen');
    document.querySelector('.power').classList.add('stale');
    put('direction','Verbindung zum Emulator unterbrochen');
    document.getElementById('direction').classList.add('stale');
    addHistory(null);
  }
  var loading=false;
  var actionKey=null;
  function action(path,actionName,body){
    if(!actionKey)actionKey=window.prompt('Emulator-Aktionsschluessel eingeben:');
    if(!actionKey)return Promise.reject(Error('Kein Schluessel eingegeben'));
    var headers={'X-Emulator-Action':actionName,'X-Emulator-Key':actionKey};
    if(body)headers['Content-Type']='application/json';
    return fetch(path,{method:'POST',headers:headers,body:body?JSON.stringify(body):undefined})
      .then(function(r){if(r.status===403){actionKey=null;throw Error('Schluessel abgelehnt')}if(!r.ok)throw Error('HTTP '+r.status);return r.json()});
  }
  document.getElementById('pairing-button').addEventListener('click',function(){
    var button=this;
    button.disabled=true;
    action('/emu/pairing','pair')
      .then(render)
      .catch(function(e){put('pairing-status','Start fehlgeschlagen: '+e.message);button.disabled=false});
  });
  document.getElementById('reset-button').addEventListener('click',function(){
    if(window.prompt('MQTT-Verbindung wird geloescht. Zum Bestaetigen RESET eingeben:')!=='RESET')return;
    var button=this;
    button.disabled=true;
    action('/emu/factory-reset','factory-reset',{confirm:'RESET'})
      .then(function(){put('reset-status','Zurueckgesetzt. Pairing wird gestartet.');poll()})
      .catch(function(e){put('reset-status','Reset fehlgeschlagen: '+e.message);button.disabled=false});
  });
  document.getElementById('reboot-button').addEventListener('click',function(){
    if(!window.confirm('ESP32-Emulator jetzt neu starten?'))return;
    var button=this;
    button.disabled=true;
    action('/emu/reboot','reboot')
      .then(function(){put('reboot-status','Neustart laeuft ...')})
      .catch(function(e){put('reboot-status','Neustart fehlgeschlagen: '+e.message);button.disabled=false});
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
