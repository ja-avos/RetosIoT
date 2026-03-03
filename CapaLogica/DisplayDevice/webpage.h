#ifndef WEBPAGE_H
#define WEBPAGE_H

const char WEBPAGE_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Display Control Panel</title>
  <style>
    * { box-sizing: border-box; margin: 0; padding: 0; }
    body { background: #0d1117; color: #e6edf3; font-family: -apple-system, BlinkMacSystemFont, sans-serif; padding: 16px; max-width: 480px; margin: 0 auto; }
    h2 { text-align: center; margin-bottom: 16px; font-size: 1.3em; }
    .card { background: #161b22; border: 1px solid #30363d; border-radius: 10px; padding: 16px; margin-bottom: 12px; }
    .card h4 { margin-bottom: 12px; color: #8b949e; font-size: 0.85em; text-transform: uppercase; letter-spacing: 1px; }
    .status { text-align: center; padding: 10px; border-radius: 8px; font-weight: 600; font-size: 0.9em; }
    .row { display: flex; justify-content: space-between; padding: 8px 0; border-bottom: 1px solid #21262d; }
    .row:last-child { border-bottom: none; }
    .row .label { color: #8b949e; }
    .row .value { font-weight: 600; }
    input[type="text"] { width: 100%; padding: 10px 12px; background: #0d1117; border: 1px solid #30363d; color: #e6edf3; border-radius: 6px; font-size: 14px; margin-bottom: 10px; }
    input[type="text"]:focus { outline: none; border-color: #58a6ff; }
    .btn-group { display: flex; gap: 8px; }
    .btn { flex: 1; padding: 12px; border: none; border-radius: 6px; font-size: 14px; font-weight: 600; cursor: pointer; transition: opacity 0.2s; }
    .btn:hover { opacity: 0.85; }
    .btn:active { transform: scale(0.98); }
    .btn-blue { background: #1f6feb; color: white; }
    .btn-red { background: #da3633; color: white; }
    .btn-grey { background: #30363d; color: #e6edf3; margin-top: 8px; width: 100%; }
    .mode-badge { display: inline-block; padding: 2px 8px; border-radius: 4px; font-size: 0.75em; font-weight: 600; }
    .toast { position: fixed; top: 16px; left: 50%; transform: translateX(-50%); background: #238636; color: white; padding: 10px 24px; border-radius: 8px; font-size: 0.9em; display: none; z-index: 10; }
  </style>
</head>
<body>
  <h2>Display Control Panel</h2>
  <div id="toast" class="toast">Sent!</div>

  <div class="card">
    <div id="status" class="status" style="background:#30363d;">Connecting...</div>
  </div>

  <div class="card">
    <h4>Live Sensor Data</h4>
    <div class="row"><span class="label">Temperature</span><span class="value" id="temp">-- &deg;C</span></div>
    <div class="row"><span class="label">Humidity</span><span class="value" id="hum">-- %</span></div>
    <div class="row"><span class="label">Luminosity</span><span class="value" id="lux">-- lux</span></div>
    <div class="row"><span class="label">Display Mode</span><span class="value" id="mode">--</span></div>
  </div>

  <div class="card">
    <h4>Send Test Message</h4>
    <input type="text" id="msg" placeholder="Type a message..." maxlength="60">
    <div class="btn-group">
      <button class="btn btn-blue" onclick="send('normal')">Normal Message</button>
      <button class="btn btn-red" onclick="send('alert')">Send Alert</button>
    </div>
  </div>

  <div class="card" style="text-align:center;">
    <button class="btn btn-grey" onclick="restart()">Restart Device</button>
  </div>

<script>
  function showToast(text, color) {
    var t = document.getElementById('toast');
    t.innerText = text;
    t.style.background = color || '#238636';
    t.style.display = 'block';
    setTimeout(function() { t.style.display = 'none'; }, 2500);
  }

  function send(type) {
    var msg = document.getElementById('msg').value.trim();
    if (!msg) { alert('Please enter a message'); return; }
    fetch('/send?type=' + type + '&message=' + encodeURIComponent(msg))
      .then(function(r) { return r.text(); })
      .then(function(t) {
        document.getElementById('msg').value = '';
        showToast(t, type === 'alert' ? '#da3633' : '#1f6feb');
      })
      .catch(function(e) { showToast('Error: ' + e, '#da3633'); });
  }

  function restart() {
    if (confirm('Restart the display device?')) {
      fetch('/reset').then(function() {
        showToast('Restarting...', '#30363d');
        setTimeout(function() { location.reload(); }, 10000);
      });
    }
  }

  setInterval(function() {
    fetch('/data').then(function(r) { return r.json(); }).then(function(d) {
      document.getElementById('temp').innerHTML = d.temp.toFixed(1) + ' &deg;C';
      document.getElementById('hum').innerText = d.hum.toFixed(0) + ' %';
      document.getElementById('lux').innerText = d.lux + ' lux';

      var modeEl = document.getElementById('mode');
      var colors = { normal: '#238636', message: '#1f6feb', alert: '#da3633' };
      var c = colors[d.mode] || '#30363d';
      modeEl.innerHTML = '<span class="mode-badge" style="background:' + c + '">' + d.mode.toUpperCase() + '</span>';

      var s = document.getElementById('status');
      s.innerText = d.mqtt ? 'MQTT Connected' : 'MQTT Disconnected';
      s.style.background = d.mqtt ? '#238636' : '#da3633';
    }).catch(function() {
      var s = document.getElementById('status');
      s.innerText = 'Device Offline';
      s.style.background = '#da3633';
    });
  }, 2000);
</script>
</body>
</html>
)rawliteral";

#endif
