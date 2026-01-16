/* ================== UI HTML ================== */
const char INDEX_HTML[] PROGMEM = R"HTML(
<!doctype html><html><head><meta charset="utf-8">
<title>ESP32 MIDI → Multi Solenoids</title>
<style>
body{font-family:Arial;max-width:900px;margin:24px auto;background:#fafafa}
.card{background:#fff;border-radius:12px;box-shadow:0 2px 10px rgba(0,0,0,.06);padding:16px;margin:16px 0}
button{padding:10px 16px;border:0;border-radius:8px;margin:6px;cursor:pointer}
.play{background:#4CAF50;color:#fff}.stop{background:#f44336;color:#fff}
.upload{background:#2196F3;color:#fff}.tool{background:#795548;color:#fff}
.test{background:#9C27B0;color:#fff}
code{background:#f3f3f3;padding:2px 6px;border-radius:6px}
.small{font-size:12px;color:#666}
input[type=range]{width:260px}
.value{display:inline-block;min-width:52px;text-align:center}
.btn{min-width:56px}
table{border-collapse:collapse;width:100%}
th,td{border:1px solid #eee;padding:8px;text-align:left}
th{background:#fafafa}
.tag{display:inline-block;padding:2px 8px;border-radius:999px;background:#eee;margin:2px 4px}
</style></head><body>
<h2>🥁 ESP32 MIDI → Multi-Solenoids</h2>

<div class="card">
  <h3>Upload .mid</h3>
  <form method="POST" action="/upload" enctype="multipart/form-data">
    <input type="file" name="midi" accept=".mid,.midi" required>
    <button class="upload" type="submit">Upload to <code>/song.mid</code></button>
  </form>
  <p class="small">Play/Scan đọc cố định file <code>/song.mid</code> trong LittleFS.</p>
</div>

<div class="card">
  <h3>Playback</h3>
  <button class="play" onclick="fetch('/play')">Play (Mixer)</button>
  <button class="tool" onclick="fetch('/scan')">Scan Notes (no timing)</button>
  <button class="stop" onclick="fetch('/stop')">Stop</button>
  <p>Status: <span id="st">...</span></p>
</div>

<div class="card">
  <h3>Power (lực đập Drum)</h3>
  <div>
    <input id="pwr" type="range" min="0.5" max="3.0" step="0.1" value="1.0" oninput="pwrval.textContent=this.value" onchange="setPower(this.value)">
    <span class="value"><b id="pwrval">1.0</b>×</span>
    <button class="tool" onclick="setPower(document.getElementById('pwr').value)">Apply</button>
  </div>
</div>

<div class="card">
  <h3>Solenoid Test</h3>
  <div>
    <button class="test" onclick="fetch('/kick?v=0')">Kick sol0</button>
    <button class="test" onclick="fetch('/kick?v=1')">Kick sol1</button>
    <button class="test" onclick="fetch('/kick?v=2')">Kick sol2</button>
    <button class="test" onclick="fetch('/kick?v=3')">Kick sol3</button>
    <button class="test" onclick="fetch('/kick?v=4')">Kick sol4</button>
    <button class="test" onclick="fetch('/kick_all')">Kick ALL</button>
  </div>
</div>

<div class="card">
  <h3>Flute Test (bảy nốt: Do Re Mi Fa Sol La Si + giữ hơi 450ms)</h3>
  <div>
    <button class="test btn" id="btnC" onclick="fl(72)" title="C">Do</button>
    <button class="test btn" id="btnD" onclick="fl(74)" title="D">Re</button>
    <button class="test btn" id="btnE" onclick="fl(76)" title="E">Mi</button>
    <button class="test btn" id="btnF" onclick="fl(77)" title="F">Fa</button>
    <button class="test btn" id="btnG" onclick="fl(79)" title="G">Sol</button>
    <button class="test btn" id="btnA" onclick="fl(81)" title="A">La</button>
    <button class="test btn" id="btnB" onclick="fl(83)" title="B">Si</button>
  </div>
  <p class="small">Tooltip nút hiển thị GPIO đang đè cho mỗi nốt.</p>
</div>

<div class="card">
  <h3>Flute GPIO Map</h3>
  <table>
    <thead><tr><th>Hole</th><th>GPIO</th></tr></thead>
    <tbody id="flmap"></tbody>
  </table>
  <p class="small">Đọc trực tiếp từ firmware (SOL[]) nên luôn đúng với phần cứng.</p>
</div>

<div class="card">
  <h3>Flute Fingers (Note → Holes → GPIO)</h3>
  <table>
    <thead><tr><th>Note</th><th>Mask (b5..b0, 1=đè)</th><th>GPIO đang đè</th></tr></thead>
    <tbody id="flfingers"></tbody>
  </table>
</div>

<script>
async function tick(){
  try{
    const r=await fetch('/status'); 
    document.getElementById('st').innerText=await r.text();
  }catch(e){}
}
async function loadPower(){
  try{
    const r=await fetch('/getpower');
    const x=parseFloat(await r.text());
    const el=document.getElementById('pwr');
    el.value=isNaN(x)?1.0:x;
    document.getElementById('pwrval').textContent=el.value;
  }catch(e){}
}
async function setPower(x){ try{ await fetch('/power?x='+x); }catch(e){} }
async function fl(n){ try{ await fetch('/flute_note?n='+n); }catch(e){} }

async function loadFluteMap(){
  try{
    const r = await fetch('/flute_map'); const j = await r.json();
    const tb = document.getElementById('flmap'); tb.innerHTML='';
    for(let i=0;i<j.holes.length;i++){
      const tr=document.createElement('tr');
      tr.innerHTML = `<td>Hole ${i+1}</td><td><code>GPIO${j.holes[i]}</code></td>`;
      tb.appendChild(tr);
    }
    const tr=document.createElement('tr');
    tr.innerHTML = `<td><b>Air Valve</b></td><td><code>GPIO${j.air}</code></td>`;
    tb.appendChild(tr);
  }catch(e){}
}

async function loadFluteFingers(){
  try{
    const r = await fetch('/flute_fingers'); const arr = await r.json();
    const tb = document.getElementById('flfingers'); tb.innerHTML='';
    const btnMap = {72:'btnC',74:'btnD',76:'btnE',77:'btnF',79:'btnG',81:'btnA',83:'btnB'};
    for(const it of arr){
      const tags = it.pins.map(p=>`<span class="tag">GPIO${p}</span>`).join(' ');
      const mask = it.mask.toString(2).padStart(6,'0'); // b5..b0
      const tr = document.createElement('tr');
      tr.innerHTML = `<td><b>${it.name}</b> (${it.note})</td><td><code>${mask}</code></td><td>${tags}</td>`;
      tb.appendChild(tr);
      const btnId = btnMap[it.note];
      const btn = document.getElementById(btnId);
      if (btn) btn.title = `${it.name}: ${it.pins.map(p=>'GPIO'+p).join(', ')} (đè)`;
    }
  }catch(e){}
}

setInterval(tick,600);
tick(); loadPower(); loadFluteMap(); loadFluteFingers();
</script>
</body></html>
)HTML";