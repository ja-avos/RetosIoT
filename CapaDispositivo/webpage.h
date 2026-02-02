#ifndef WEBPAGE_H
#define WEBPAGE_H

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>IoT Dashboard</title>
  <link href="https://cdn.jsdelivr.net/npm/bootstrap@5.1.3/dist/css/bootstrap.min.css" rel="stylesheet">
  <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
  <style>
    body { background-color: #121212; color: #ffffff; font-family: sans-serif; }
    .card { background-color: #1e1e1e; border: none; margin-bottom: 20px; box-shadow: 0 4px 8px rgba(0,0,0,0.3); }
    
    /* Input Styling */
    input[type="color"] { border: none; width: 100%; height: 50px; cursor: pointer; border-radius: 5px; }
    .form-switch .form-check-input { width: 3em; height: 1.5em; cursor: pointer; }
    
    .status-badge { font-size: 0.9rem; padding: 10px; border-radius: 5px; display: inline-block; width: 100%; text-align: center;}
    
    /* Chart Height */
    canvas { max-height: 250px; }
  </style>
</head>
<body>
  <div class="container mt-4">
    
    <div class="d-flex justify-content-between align-items-center mb-4">
      <h2>IoT Control Center</h2>
      <div class="d-flex gap-2">
         <span id="connStatus" class="badge bg-secondary status-badge" style="min-width:100px;">Connecting...</span>
         <button class="btn btn-outline-danger btn-sm" onclick="resetDevice()">&#x21bb; Reset</button>
      </div>
    </div>
    
    <div class="row">
      <div class="col-12">
        <div class="card p-3">
          <div class="row align-items-center">
            <div class="col-md-6">
               <label class="form-label text-muted">LED Color</label>
               <input type="color" id="colorPicker" value="#000000" onchange="updateColor()">
            </div>
            <div class="col-md-6 mt-3 mt-md-0 d-flex justify-content-between align-items-center">
               <span class="fs-5">Master Power</span>
               <div class="form-check form-switch">
                 <input class="form-check-input" type="checkbox" id="ledToggle" onchange="toggleLed()">
               </div>
               <span id="ledStateText" class="text-muted" style="width: 40px;">OFF</span>
            </div>
          </div>
        </div>
      </div>
    </div>

    <div class="row">
      
      <div class="col-md-4">
        <div class="card p-3">
          <div class="d-flex justify-content-between">
            <h5 class="text-danger">Temperature</h5>
            <div><span id="temp" class="fs-4 fw-bold">--</span> <span class="unit">&deg;C</span></div>
          </div>
          <canvas id="tempChart"></canvas>
        </div>
      </div>

      <div class="col-md-4">
        <div class="card p-3">
          <div class="d-flex justify-content-between">
            <h5 class="text-info">Humidity</h5>
            <div><span id="hum" class="fs-4 fw-bold">--</span> <span class="unit">%</span></div>
          </div>
          <canvas id="humChart"></canvas>
        </div>
      </div>

      <div class="col-md-4">
        <div class="card p-3">
          <div class="d-flex justify-content-between">
            <h5 class="text-warning">Light</h5>
            <div><span id="lux" class="fs-4 fw-bold">--</span> <span class="unit">Lux</span></div>
          </div>
          <canvas id="luxChart"></canvas>
        </div>
      </div>

    </div>
  </div>

<script>
  // --- BASE CHART SETTINGS ---
  const baseConfig = {
    responsive: true,
    animation: false,
    plugins: { legend: { display: false } },
    scales: {
      x: { ticks: { display: false }, grid: { display: false } }
    }
  };

  // --- 1. TEMPERATURE CHART (-10 to 50) ---
  const ctxTemp = document.getElementById('tempChart').getContext('2d');
  const chartTemp = new Chart(ctxTemp, {
    type: 'line',
    data: { labels: [], datasets: [{ borderColor: '#ff6384', borderWidth: 2, fill: true, backgroundColor: 'rgba(255, 99, 132, 0.2)', data: [] }] },
    options: {
      ...baseConfig,
      scales: {
        ...baseConfig.scales,
        y: { 
          min: -10, 
          max: 50,
          ticks: { color: '#888' }, grid: { color: '#333' } 
        }
      }
    }
  });

  // --- 2. HUMIDITY CHART (0 to 100) ---
  const ctxHum = document.getElementById('humChart').getContext('2d');
  const chartHum = new Chart(ctxHum, {
    type: 'line',
    data: { labels: [], datasets: [{ borderColor: '#36a2eb', borderWidth: 2, fill: true, backgroundColor: 'rgba(54, 162, 235, 0.2)', data: [] }] },
    options: {
      ...baseConfig,
      scales: {
        ...baseConfig.scales,
        y: { 
          min: 0, 
          max: 100,
          ticks: { color: '#888' }, grid: { color: '#333' } 
        }
      }
    }
  });

  // --- 3. LIGHT CHART (Adaptive: 0 to 1000+) ---
  const ctxLux = document.getElementById('luxChart').getContext('2d');
  const chartLux = new Chart(ctxLux, {
    type: 'line',
    data: { labels: [], datasets: [{ borderColor: '#ffcd56', borderWidth: 2, fill: true, backgroundColor: 'rgba(255, 205, 86, 0.2)', data: [] }] },
    options: {
      ...baseConfig,
      scales: {
        ...baseConfig.scales,
        y: { 
          suggestedMin: 0, 
          suggestedMax: 1000, // Starts at 1000, but expands if brighter
          ticks: { color: '#888' }, grid: { color: '#333' } 
        }
      }
    }
  });

  // --- UI & LOGIC ---
  const statusEl = document.getElementById('connStatus');
  const toggleEl = document.getElementById('ledToggle');

  function updateColor() {
    const color = document.getElementById('colorPicker').value;
    fetch('/set-color?hex=' + color.substring(1)).catch(console.log);
    if(!toggleEl.checked) { toggleEl.checked = true; }
  }

  function toggleLed() {
    const state = toggleEl.checked;
    document.getElementById('ledStateText').innerText = state ? "ON" : "OFF";
    fetch('/set-state?state=' + (state ? '1' : '0')).catch(console.log);
  }
  
  function resetDevice() {
    if(confirm("Restart device?")) {
      fetch('/reset').then(() => {
        alert("Restarting... Reloading in 10s.");
        setTimeout(() => location.reload(), 10000);
      });
    }
  }

  function updateStatus(state) {
    statusEl.classList.remove('bg-secondary', 'bg-success', 'bg-warning', 'bg-danger', 'text-dark');
    statusEl.classList.add('text-white');
    if (state === 'ok') {
        statusEl.innerText = "MQTT\nOnline";
        statusEl.classList.add('bg-success');
    } else if (state === 'mqtt_err') {
        statusEl.innerText = "MQTT Error";
        statusEl.classList.add('bg-warning', 'text-dark');
    } else {
        statusEl.innerText = "Lost";
        statusEl.classList.add('bg-danger');
    }
  }

  // --- DATA LOOP ---
  setInterval(function() {
    fetch('/data')
      .then(r => r.ok ? r.json() : Promise.reject(r))
      .then(data => {
        // Update Text
        document.getElementById('temp').innerText = data.temp.toFixed(1);
        document.getElementById('hum').innerText = data.hum.toFixed(0);
        document.getElementById('lux').innerText = data.lux;
        
        // Update Visuals
        if(document.activeElement !== toggleEl) {
             toggleEl.checked = data.ledState;
             document.getElementById('ledStateText').innerText = data.ledState ? "ON" : "OFF";
        }
        updateStatus(data.mqtt ? 'ok' : 'mqtt_err');

        // Update Charts
        const time = new Date().toLocaleTimeString();
        const updateChart = (chart, val) => {
           if(chart.data.labels.length > 20) {
             chart.data.labels.shift();
             chart.data.datasets[0].data.shift();
           }
           chart.data.labels.push(time);
           chart.data.datasets[0].data.push(val);
           chart.update();
        };

        updateChart(chartTemp, data.temp);
        updateChart(chartHum, data.hum);
        updateChart(chartLux, data.lux);
      })
      .catch(e => updateStatus('lost'));
  }, 2000);
</script>
</body>
</html>
)rawliteral";

#endif