// Library Imports
#include <ArduinoJson.h>
#include <DNSServer.h>
#include <HTTPClient.h>
#include <MD_MAX72xx.h>
#include <MD_Parola.h>
#include <Preferences.h>
#include <SPI.h>
#include <Update.h>
#include <WebServer.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <Wire.h>
#include <math.h>

// File imports
#include "FontSubs.h"
#include "holiday_easter_eggs.h"
#include "milestone_animations.h"
#include "milestone_helpers.h"

// Hardware config
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
// #define HARDWARE_TYPE MD_MAX72XX::GENERIC_HW
#define MAX_DEVICES 4
#define CS_PIN 5

#define AP_SSID_PREFIX "YouTubeCounter-Setup-"
#define WOKWI_GUEST_SSID "Wokwi-GUEST"
#define DNS_PORT 53
#define WIFI_TIMEOUT_MS 15000
#define WOKWI_SETUP_TIMEOUT_MS 8000

const bool ENABLE_WOKWI_SETUP = false;
const bool DISABLE_INTRO_AND_WIFI_INFO = false;

// Set true to simulate a WiFi drop after the first successful stats fetch.
const bool DEBUG_SIMULATE_WIFI_DROP_AFTER_FIRST_STATS_FETCH = false;

// Set true to skip milestone/holiday intro animations before themed sequences.
extern const bool DEBUG_DISABLE_ANIMATION_INTROS = false;

// Set true to preview one hardcoded milestone before any other boot preview.
const bool RUN_SINGLE_MILESTONE_PREVIEW_ON_BOOT = false;
const MilestoneAnimation SINGLE_MILESTONE_PREVIEW_BOOT =
    MilestoneAnimation::Hours1M;

// Set true to cycle milestone animations on boot (dev preview).
const bool PREVIEW_MILESTONES = false;
static const MilestoneAnimation MILESTONE_BOOT_PREVIEW[] = {
    MilestoneAnimation::Subs100,   MilestoneAnimation::Subs1K,
    MilestoneAnimation::Subs10K,   MilestoneAnimation::Subs100K,
    MilestoneAnimation::Subs1M,    MilestoneAnimation::Subs10M,
    MilestoneAnimation::Views10K,  MilestoneAnimation::Views100K,
    MilestoneAnimation::Views1M,   MilestoneAnimation::Views10M,
    MilestoneAnimation::Views100M, MilestoneAnimation::Hours100,
    MilestoneAnimation::Hours1K,   MilestoneAnimation::Hours10K,
    MilestoneAnimation::Hours100K, MilestoneAnimation::Hours1M,
    MilestoneAnimation::Hours10M,
};

MD_Parola Display = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);
WiFiClientSecure client;
WebServer server(80);
DNSServer dnsServer;
Preferences prefs;

const unsigned long DISPLAY_UPDATE_MS = 1000;
const float DEFAULT_STAT_CYCLE_SECONDS = 5.0f;
const unsigned int DEFAULT_SCROLL_SPEED_MS = 75;
const uint8_t STAT_RIGHT_PADDING_COLUMNS = 1;
const int16_t STAT_SCROLL_OFFSCREEN_MARGIN = 2;
const int16_t STAT_LABEL_NUMBER_GAP_COLUMNS = 6;
const uint8_t DIGIT_ROLL_FRAME_DELAY_MS = 28;
const unsigned int DEFAULT_REFRESH_MINUTES = 5;
const unsigned int MAX_REFRESH_MINUTES = 1440;
const unsigned int DEFAULT_HOLIDAY_REMINDER_MINUTES = 15;
const unsigned int MAX_HOLIDAY_REMINDER_MINUTES = 1440;
const uint8_t DEFAULT_DISPLAY_BRIGHTNESS = 0;
const double SECONDS_PER_28_DAYS = 28.0 * 24.0 * 60.0 * 60.0;
const int INTERVAL_PATTERN_LENGTH = 48;
const unsigned long WIFI_INDICATOR_FLASH_MS = 220;
const unsigned long WIFI_INDICATOR_PAUSE_MS = 1100;
const unsigned long WIFI_INDICATOR_PATTERN_MS =
    (WIFI_INDICATOR_FLASH_MS * 6) + WIFI_INDICATOR_PAUSE_MS;

const uint8_t STAT_SUBSCRIBERS = 1 << 0;
const uint8_t STAT_VIEWS = 1 << 1;
const uint8_t STAT_WATCH_HOURS = 1 << 2;
const uint8_t STAT_ALL = STAT_SUBSCRIBERS | STAT_VIEWS | STAT_WATCH_HOURS;

const int STAT_INDEX_SUBSCRIBERS = 0;
const int STAT_INDEX_VIEWS = 1;
const int STAT_INDEX_WATCH_HOURS = 2;
const int STAT_COUNT = 3;

const uint8_t STAT_MASKS[STAT_COUNT] = {STAT_SUBSCRIBERS, STAT_VIEWS,
                                        STAT_WATCH_HOURS};
const char *STAT_LABELS[STAT_COUNT] = {"Subs:", "Views:", "Hours:"};

unsigned long api_lasttime = 0;
unsigned long display_lasttime = 0;
unsigned long cycle_lasttime = 0;
unsigned long stats_fetched_at = 0;
double stat_baseline_values[STAT_COUNT] = {0, 0, 0};
double stat_increase_per_28_days[STAT_COUNT] = {0, 0, 0};
double stat_baseline_started_at_unix[STAT_COUNT] = {0, 0, 0};
double stats_as_of_unix = 0;
int current_stat_index = STAT_INDEX_SUBSCRIBERS;
bool statsLoaded = false;
StaticJsonDocument<1536> doc;

String saved_ssid, saved_pass, saved_endpoint;
uint8_t saved_stats = STAT_SUBSCRIBERS;
unsigned int saved_refresh_minutes = DEFAULT_REFRESH_MINUTES;
unsigned int saved_holiday_reminder_minutes = DEFAULT_HOLIDAY_REMINDER_MINUTES;
float saved_stat_cycle_seconds = DEFAULT_STAT_CYCLE_SECONDS;
unsigned int saved_scroll_speed_ms = DEFAULT_SCROLL_SPEED_MS;
uint8_t saved_display_brightness = DEFAULT_DISPLAY_BRIGHTNESS;
bool saved_animation_brightness_boost = false;
uint8_t saved_animation_brightness_boost_amount = 0;
bool configMode = false;
bool captivePortalActive = false;
char setupMacSuffix[5] = "";
char setupScrollBuffer[48] = "";

String stat_format(double value, int statIndex);
String getStatDisplayText(int statIndex, const String &formattedValue);
String getSetupApSsid();
void runBootAnimation();
void initSetupDisplay();
void updateSetupDisplay();
String html_escape(String value);
bool fetchStats();
void showProjectedStat();
void startStatDisplay();
void startStatScrollIn();
void startStatExitTransition();
bool tickStatScrollAnimation();
bool canRefreshStats();
bool isWiFiDisconnectedForDisplay();
void tickWiFiDisconnectedIndicator();
void renderRightAlignedStat(const char *text);
void renderRightAlignedStatRollingLastDigit(const char *oldText,
                                            const char *newText);
bool checkAndRunMilestoneAnimation();
double getProjectedStatValue(int statIndex, double currentUnixTimestamp);
double getProjectedWholeStatValue(double baselineStartedAtUnix,
                                  double startingValue,
                                  double increasePer28Days,
                                  double currentUnixTimestamp);
double getProjectedFractionalStatValue(double baselineStartedAtUnix,
                                       double startingValue,
                                       double increasePer28Days,
                                       double currentUnixTimestamp);

// ─── Config portal HTML
// ───────────────────────────────────────────────────────
const char CONFIG_HTML[] PROGMEM = R"rawhtml(
<!DOCTYPE html><html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Stats Counter Setup</title>
<style>
  *{box-sizing:border-box;margin:0;padding:0}
  body{background:#111;color:#e8e8e8;font-family:system-ui,sans-serif;display:flex;align-items:center;justify-content:center;min-height:100vh;padding:1rem}
  .card{background:#1a1a1a;border:1px solid #2a2a2a;border-radius:14px;width:100%;max-width:360px;overflow:hidden}
  .header{background:#0d0d0d;padding:18px 22px;border-bottom:1px solid #222}
  .logo{display:flex;align-items:center;gap:10px}
  .dot{width:28px;height:28px;background:#22c55e;border-radius:6px;display:flex;align-items:center;justify-content:center;font-size:16px;font-weight:700;color:#0a2e18}
  h1{font-size:15px;font-weight:600;color:#f0f0f0}
  .sub{font-size:12px;color:#555;margin-top:3px}
  .body{padding:20px 22px 24px;display:flex;flex-direction:column;gap:13px}
  .status{background:#0a1a10;border:1px solid #1a3a20;border-radius:8px;padding:8px 12px;font-size:12px;color:#4ade80;display:flex;align-items:center;gap:8px}
  .dot-green{width:7px;height:7px;border-radius:50%;background:#22c55e;flex-shrink:0}
  .section{font-size:11px;font-weight:600;color:#555;text-transform:uppercase;letter-spacing:.05em}
  label{font-size:11px;font-weight:600;color:#777;text-transform:uppercase;letter-spacing:.04em;display:block;margin-bottom:4px}
  input[type=text],input[type=password],input[type=url],input[type=number]{width:100%;background:#1c1c1c;border:1px solid #2e2e2e;border-radius:8px;padding:9px 12px;font-size:13px;color:#e8e8e8;outline:none}
  input[type=text]:focus,input[type=password]:focus,input[type=url]:focus,input[type=number]:focus{border-color:#22c55e}
  .checks{display:flex;flex-direction:column;gap:7px}
  .check{display:flex;align-items:center;gap:8px;background:#1c1c1c;border:1px solid #2e2e2e;border-radius:8px;padding:9px 12px;font-size:13px;color:#e8e8e8}
  .check input{accent-color:#22c55e}
  .check span{font-size:13px;color:#e8e8e8;text-transform:none;letter-spacing:0}
  input::placeholder{color:#444}
  .hint{font-size:11px;color:#555;margin-top:3px}
  .divider{height:1px;background:#222}
  .btn{width:100%;background:#22c55e;color:#0a2e18;border:none;border-radius:8px;padding:11px;font-size:14px;font-weight:600;cursor:pointer;margin-top:4px}
  .btn:hover{background:#1daa50}
  .btn-ota{width:100%;background:#1c1c1c;color:#e8e8e8;border:1px solid #2e2e2e;border-radius:8px;padding:11px;font-size:14px;font-weight:600;cursor:pointer;margin-top:4px}
  .btn-ota:hover{background:#252525}
  .msg{font-size:13px;text-align:center;padding:8px;border-radius:8px;display:none}
  .msg.ok{background:#0a1a10;color:#4ade80;border:1px solid #1a3a20;display:block}
  .msg.err{background:#1a0a0a;color:#f87171;border:1px solid #3a1a1a;display:block}
  .msg.info{background:#0a0e1a;color:#60a5fa;border:1px solid #1a2a3a;display:block}
  .progress-wrap{background:#1c1c1c;border-radius:8px;height:8px;overflow:hidden;display:none}
  .progress-wrap.show{display:block}
  .progress-bar{height:100%;width:0%;background:#22c55e;border-radius:8px;transition:width .2s}
  input[type=file]{width:100%;background:#1c1c1c;border:1px solid #2e2e2e;border-radius:8px;padding:8px 12px;font-size:13px;color:#888;outline:none;cursor:pointer}
  input[type=file]::file-selector-button{background:#22c55e;color:#0a2e18;border:none;border-radius:6px;padding:4px 10px;font-size:12px;font-weight:600;cursor:pointer;margin-right:10px}
  .btn-scan{width:100%;background:#1c1c1c;color:#e8e8e8;border:1px solid #2e2e2e;border-radius:8px;padding:9px;font-size:13px;font-weight:600;cursor:pointer;display:flex;align-items:center;justify-content:center;gap:6px}
  .btn-scan:hover{background:#252525}
  .btn-scan.spinning span{display:inline-block;animation:spin .8s linear infinite}
  @keyframes spin{to{transform:rotate(360deg)}}
  .net-list{display:flex;flex-direction:column;gap:6px;display:none}
  .net-list.show{display:flex}
  .net-item{display:flex;align-items:center;justify-content:space-between;background:#1c1c1c;border:1px solid #2e2e2e;border-radius:8px;padding:9px 12px;cursor:pointer;font-size:13px;transition:border-color .15s}
  .net-item:hover{border-color:#22c55e}
  .net-item.active{border-color:#22c55e;background:#0a1a10}
  .net-name{color:#e8e8e8;font-weight:500}
  .net-meta{display:flex;align-items:center;gap:8px}
  .net-lock{font-size:11px;color:#555}
  .net-bars{display:flex;align-items:flex-end;gap:2px;height:14px}
  .net-bars span{width:3px;background:#555;border-radius:1px}
  .net-bars span.lit{background:#22c55e}
  .pw-wrap{position:relative}
  .pw-wrap input{padding-right:38px}
  .eye-btn{position:absolute;right:10px;top:50%;transform:translateY(-50%);background:none;border:none;cursor:pointer;color:#555;font-size:16px;line-height:1;padding:0}
  .eye-btn:hover{color:#aaa}
</style></head><body>
<div class="card">
  <div class="header">
    <div class="logo"><div class="dot">&#9654;</div><div><h1>Stats Counter</h1><div class="sub">Device setup</div></div></div>
  </div>
  <div class="body">
    <div class="status"><div class="dot-green"></div>Connected &mdash; IP_PLACEHOLDER</div>
    <div class="section">Wi-Fi</div>
    <button class="btn-scan" id="scan-btn" onclick="scanNetworks()"><span>&#8635;</span> Scan for networks</button>
    <div class="net-list" id="net-list"></div>
    <div><label>Network name (SSID)</label><input type="text" id="ssid" placeholder="Leave blank to keep saved" value="SSID_PLACEHOLDER"></div>
    <div><label>Password <span style="color:#555;font-weight:400;text-transform:none">(leave blank to keep saved)</span></label><div class="pw-wrap"><input type="password" id="pw" placeholder="&bull;&bull;&bull;&bull;&bull;&bull;&bull;&bull;"><button class="eye-btn" type="button" onclick="toggleEye('pw','eye-pw')"><span id="eye-pw">&#128065;</span></button></div></div>
    <div class="divider"></div>
    <div class="section">Stats API</div>
    <div>
      <label>Stats API endpoint</label>
      <input type="url" id="endpoint" placeholder="https://your-domain.example/api/stats" value="ENDPOINT_PLACEHOLDER">
      <div class="hint">Must return the documented stats JSON shape.</div>
    </div>
    <div>
      <label>Stats to show</label>
      <div class="checks">
        <label class="check"><input type="checkbox" id="stat-subs" SUBS_CHECKED><span>Subscribers</span></label>
        <label class="check"><input type="checkbox" id="stat-views" VIEWS_CHECKED><span>Views</span></label>
        <label class="check"><input type="checkbox" id="stat-hours" HOURS_CHECKED><span>Watch hours</span></label>
      </div>
      <div class="hint">Multiple selections cycle on the matrix.</div>
    </div>
    <div>
      <label>Scroll speed (ms)</label>
      <input type="number" id="scroll-speed" min="1" step="1" value="SCROLL_SPEED_PLACEHOLDER">
      <div class="hint">Milliseconds between scroll steps when stats animate onto the display.</div>
    </div>
    <div>
      <label>Value display time (seconds)</label>
      <input type="number" id="stat-cycle" step="any" value="STAT_CYCLE_PLACEHOLDER">
      <div class="hint">How long each stat is shown when cycling (scroll-in and value).</div>
    </div>
    <div class="divider"></div>
    <div class="section">Display</div>
    <div>
      <label>Brightness</label>
      <input type="number" id="brightness" min="0" max="15" step="1" value="BRIGHTNESS_PLACEHOLDER">
      <div class="hint">Baseline brightness for all text and animations.</div>
    </div>
    <div class="divider"></div>
    <div class="section">Animations</div>
    <div>
      <label class="check"><input type="checkbox" id="anim-boost" ANIM_BOOST_CHECKED><span>Animation brightness boost</span></label>
      <div class="hint">Animations start from the display baseline; enabling boost adds the amount below.</div>
    </div>
    <div>
      <label>Animation brightness boost amount</label>
      <input type="number" id="anim-boost-amount" min="0" max="15" step="1" value="ANIM_BOOST_AMOUNT_PLACEHOLDER">
      <div class="hint">Adds 0-15 brightness steps to normalized animation frames.</div>
    </div>
    <div>
      <label>Holiday reminder interval (minutes)</label>
      <input type="number" id="holiday-reminder" min="0" max="1440" step="1" value="HOLIDAY_REMINDER_PLACEHOLDER">
      <div class="hint">How often holiday messages replay on matching calendar days. Use 0 to disable holidays.</div>
    </div>
    <div>
      <label>Refresh rate (minutes)</label>
      <input type="number" id="refresh" min="1" max="1440" step="1" value="REFRESH_PLACEHOLDER">
      <div class="hint">The display projects values every second between API refreshes.</div>
    </div>
    <div id="msg" class="msg"></div>
    <button class="btn" onclick="save()">Save &amp; connect</button>
    <div class="divider"></div>
    <div class="section">Firmware update</div>
    <div>
      <label>Firmware .bin file</label>
      <input type="file" id="binfile" accept=".bin">
      <div class="hint">PlatformIO build output: .pio/build/esp32dev/firmware.bin</div>
    </div>
    <div class="progress-wrap" id="prog-wrap"><div class="progress-bar" id="prog-bar"></div></div>
    <div id="ota-msg" class="msg"></div>
    <button class="btn-ota" onclick="uploadFirmware()">Upload firmware</button>
  </div>
</div>
<script>
var SETUP_STORAGE_KEY='statsCounterSetup';
function persistSetupForm(){
  try{
    localStorage.setItem(SETUP_STORAGE_KEY,JSON.stringify({
      ssid:document.getElementById('ssid').value,
      endpoint:document.getElementById('endpoint').value,
      statSubs:document.getElementById('stat-subs').checked,
      statViews:document.getElementById('stat-views').checked,
      statHours:document.getElementById('stat-hours').checked,
      scrollSpeed:document.getElementById('scroll-speed').value,
      statCycle:document.getElementById('stat-cycle').value,
      brightness:document.getElementById('brightness').value,
      animationBoost:document.getElementById('anim-boost').checked,
      animationBoostAmount:document.getElementById('anim-boost-amount').value,
      holidayReminder:document.getElementById('holiday-reminder').value,
      refresh:document.getElementById('refresh').value
    }));
  }catch(e){}
}
function restoreSetupForm(){
  try{
    var raw=localStorage.getItem(SETUP_STORAGE_KEY);
    if(!raw)return;
    var d=JSON.parse(raw);
    if(d.ssid!=null)document.getElementById('ssid').value=d.ssid;
    if(d.endpoint!=null)document.getElementById('endpoint').value=d.endpoint;
    if(d.statSubs!=null)document.getElementById('stat-subs').checked=!!d.statSubs;
    if(d.statViews!=null)document.getElementById('stat-views').checked=!!d.statViews;
    if(d.statHours!=null)document.getElementById('stat-hours').checked=!!d.statHours;
    if(d.scrollSpeed!=null)document.getElementById('scroll-speed').value=d.scrollSpeed;
    if(d.statCycle!=null)document.getElementById('stat-cycle').value=d.statCycle;
    if(d.brightness!=null)document.getElementById('brightness').value=d.brightness;
    if(d.animationBoost!=null)document.getElementById('anim-boost').checked=!!d.animationBoost;
    if(d.animationBoostAmount!=null)document.getElementById('anim-boost-amount').value=d.animationBoostAmount;
    if(d.holidayReminder!=null)document.getElementById('holiday-reminder').value=d.holidayReminder;
    if(d.refresh!=null)document.getElementById('refresh').value=d.refresh;
  }catch(e){}
}
function bindSetupPersist(){
  ['ssid','endpoint','scroll-speed','stat-cycle','brightness','anim-boost-amount','holiday-reminder','refresh'].forEach(function(id){
    document.getElementById(id).addEventListener('input',persistSetupForm);
  });
  ['stat-subs','stat-views','stat-hours','anim-boost'].forEach(function(id){
    document.getElementById(id).addEventListener('change',persistSetupForm);
  });
}
function barsHtml(rssi){
  var lvl=rssi>-55?4:rssi>-65?3:rssi>-75?2:1;
  var h='<div class="net-bars">';
  var heights=[4,7,10,14];
  for(var i=0;i<4;i++){h+='<span style="height:'+heights[i]+'px"'+(i<lvl?' class="lit"':'')+' ></span>';}
  return h+'</div>';
}
function scanNetworks(){
  var btn=document.getElementById('scan-btn');
  var list=document.getElementById('net-list');
  btn.classList.add('spinning');
  btn.disabled=true;
  list.className='net-list';
  list.innerHTML='';
  fetch('/scan').then(r=>r.json()).then(nets=>{
    btn.classList.remove('spinning');
    btn.disabled=false;
    if(!nets.length){list.innerHTML='<div style="font-size:12px;color:#555;text-align:center">No networks found</div>';list.className='net-list show';return;}
    list.innerHTML=nets.map(function(n){
      return '<div class="net-item" onclick="pickNet(this,\''+n.ssid.replace(/'/g,"\\'")+'\')">'
        +'<span class="net-name">'+n.ssid+'</span>'
        +'<span class="net-meta">'+barsHtml(n.rssi)+(n.secure?'<span class="net-lock">&#128274;</span>':'')+'</span>'
        +'</div>';
    }).join('');
    list.className='net-list show';
  }).catch(function(){
    btn.classList.remove('spinning');
    btn.disabled=false;
  });
}
function pickNet(el,ssid){
  document.querySelectorAll('.net-item').forEach(function(x){x.classList.remove('active');});
  el.classList.add('active');
  document.getElementById('ssid').value=ssid;
  persistSetupForm();
  document.getElementById('pw').focus();
}
function toggleEye(inputId,iconId){
  var inp=document.getElementById(inputId);
  var ico=document.getElementById(iconId);
  if(inp.type==='password'){inp.type='text';ico.textContent='\uD83D\uDE48';}
  else{inp.type='password';ico.textContent='\uD83D\uDC41';}
}
function save(){
  var s=document.getElementById('ssid').value.trim();
  var p=document.getElementById('pw').value;
  var e=document.getElementById('endpoint').value.trim();
  var r=parseInt(document.getElementById('refresh').value,10);
  var scrollSpeed=parseInt(document.getElementById('scroll-speed').value,10);
  var statCycle=parseFloat(document.getElementById('stat-cycle').value);
  var brightness=parseInt(document.getElementById('brightness').value,10);
  var animBoost=document.getElementById('anim-boost').checked;
  var animBoostAmount=parseInt(document.getElementById('anim-boost-amount').value,10);
  var holidayReminder=parseInt(document.getElementById('holiday-reminder').value,10);
  var stats=0;
  if(document.getElementById('stat-subs').checked)stats|=1;
  if(document.getElementById('stat-views').checked)stats|=2;
  if(document.getElementById('stat-hours').checked)stats|=4;
  var msg=document.getElementById('msg');
  if(!e){msg.className='msg err';msg.textContent='Stats API endpoint is required.';return;}
  if(!/^https?:\/\//i.test(e)){msg.className='msg err';msg.textContent='Endpoint must start with http:// or https://';return;}
  if(!stats){msg.className='msg err';msg.textContent='Choose at least one stat to show.';return;}
  if(!scrollSpeed||scrollSpeed<=0){msg.className='msg err';msg.textContent='Scroll speed must be greater than 0.';return;}
  if(!statCycle||statCycle<=0){msg.className='msg err';msg.textContent='Value display time must be greater than 0.';return;}
  if(isNaN(brightness)||brightness<0||brightness>15){msg.className='msg err';msg.textContent='Brightness must be 0-15.';return;}
  if(isNaN(animBoostAmount)||animBoostAmount<0||animBoostAmount>15){msg.className='msg err';msg.textContent='Animation brightness boost amount must be 0-15.';return;}
  if(isNaN(holidayReminder)||holidayReminder<0){msg.className='msg err';msg.textContent='Holiday reminder interval must be 0 or greater.';return;}
  if(!r||r<1){msg.className='msg err';msg.textContent='Refresh rate must be at least 1 minute.';return;}
  persistSetupForm();
  msg.className='msg ok';msg.textContent='Saving\u2026 device will reboot and connect.';
  fetch('/save',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:'ssid='+encodeURIComponent(s)+'&pass='+encodeURIComponent(p)+'&endpoint='+encodeURIComponent(e)+'&stats='+stats+'&refresh='+r+'&scrollSpeed='+scrollSpeed+'&statCycle='+statCycle+'&brightness='+brightness+'&animBoost='+(animBoost?1:0)+'&animBoostAmount='+animBoostAmount+'&holidayReminder='+holidayReminder})
  .then(r=>r.text()).then(t=>{msg.textContent=t;});
}
function uploadFirmware(){
  var file=document.getElementById('binfile').files[0];
  var omsg=document.getElementById('ota-msg');
  var bar=document.getElementById('prog-bar');
  var wrap=document.getElementById('prog-wrap');
  if(!file){omsg.className='msg err';omsg.textContent='Select a .bin file first.';return;}
  if(!file.name.endsWith('.bin')){omsg.className='msg err';omsg.textContent='File must be a .bin';return;}
  var xhr=new XMLHttpRequest();
  xhr.open('POST','/update',true);
  xhr.upload.onprogress=function(e){
    if(e.lengthComputable){
      var pct=Math.round(e.loaded/e.total*100);
      wrap.className='progress-wrap show';
      bar.style.width=pct+'%';
      omsg.className='msg info';
      omsg.textContent='Uploading\u2026 '+pct+'%';
    }
  };
  xhr.onload=function(){
    if(xhr.status===200){
      omsg.className='msg ok';
      omsg.textContent='Update complete! Rebooting\u2026';
      bar.style.width='100%';
    } else {
      omsg.className='msg err';
      omsg.textContent='Update failed: '+xhr.responseText;
    }
  };
  xhr.onerror=function(){omsg.className='msg err';omsg.textContent='Upload error. Check connection.';};
  var fd=new FormData();
  fd.append('firmware',file,file.name);
  xhr.send(fd);
}
restoreSetupForm();
bindSetupPersist();
</script>
</body></html>
)rawhtml";

// ─── Helpers
// ──────────────────────────────────────────────────────────────────
void loadPrefs() {
  prefs.begin("ytcounter", true);
  saved_ssid = prefs.getString("ssid", "");
  saved_pass = prefs.getString("pass", "");
  saved_endpoint = prefs.getString("endpoint", "");
  saved_stats = prefs.getUChar("stats", STAT_SUBSCRIBERS) & STAT_ALL;
  saved_refresh_minutes = prefs.getUInt("refresh", DEFAULT_REFRESH_MINUTES);
  saved_holiday_reminder_minutes =
      prefs.getUInt("holidayMin", DEFAULT_HOLIDAY_REMINDER_MINUTES);
  saved_stat_cycle_seconds = prefs.getFloat("statCycle", 0);
  if (saved_stat_cycle_seconds <= 0) {
    unsigned long legacyStatCycle = prefs.getUInt("statCycle", 0);
    saved_stat_cycle_seconds = legacyStatCycle > 0 ? (float)legacyStatCycle
                                                   : DEFAULT_STAT_CYCLE_SECONDS;
  }
  saved_scroll_speed_ms = prefs.getUInt("scrollSpeed", 0);
  if (saved_scroll_speed_ms == 0) {
    saved_scroll_speed_ms = DEFAULT_SCROLL_SPEED_MS;
  }
  saved_display_brightness =
      prefs.getUChar("brightness", DEFAULT_DISPLAY_BRIGHTNESS);
  saved_animation_brightness_boost = prefs.getBool("animBoost", false);
  saved_animation_brightness_boost_amount = prefs.getUChar("animBoostAmt", 0);
  prefs.end();

  if (saved_stats == 0)
    saved_stats = STAT_SUBSCRIBERS;
  if (saved_refresh_minutes < 1)
    saved_refresh_minutes = DEFAULT_REFRESH_MINUTES;
  if (saved_refresh_minutes > MAX_REFRESH_MINUTES)
    saved_refresh_minutes = MAX_REFRESH_MINUTES;
  if (saved_holiday_reminder_minutes > MAX_HOLIDAY_REMINDER_MINUTES)
    saved_holiday_reminder_minutes = MAX_HOLIDAY_REMINDER_MINUTES;
  if (saved_display_brightness > 15)
    saved_display_brightness = 15;
  if (saved_animation_brightness_boost_amount > 15)
    saved_animation_brightness_boost_amount = 15;

  Serial.print("Config loaded: ssid=");
  Serial.print(saved_ssid.length() ? saved_ssid : "(empty)");
  Serial.print(", endpoint=");
  Serial.print(saved_endpoint.length() ? "set" : "empty");
  Serial.print(", stats=");
  Serial.print(saved_stats);
  Serial.print(", brightness=");
  Serial.print(saved_display_brightness);
  Serial.print(", animBoost=");
  Serial.print(saved_animation_brightness_boost ? "on" : "off");
  Serial.print(", animBoostAmount=");
  Serial.print(saved_animation_brightness_boost_amount);
  Serial.print(", holidayReminder=");
  Serial.println(saved_holiday_reminder_minutes);
}

void savePrefs(String ssid, String pass, String endpoint, uint8_t stats,
               unsigned int refreshMinutes, float statCycleSeconds,
               unsigned int scrollSpeedMs, uint8_t displayBrightness,
               bool animationBrightnessBoost,
               uint8_t animationBrightnessBoostAmount,
               unsigned int holidayReminderMinutes) {
  prefs.begin("ytcounter", false);
  prefs.putString("ssid", ssid);
  prefs.putString("pass", pass);
  prefs.putString("endpoint", endpoint);
  prefs.putUChar("stats", stats);
  prefs.putUInt("refresh", refreshMinutes);
  prefs.putFloat("statCycle", statCycleSeconds);
  prefs.putUInt("scrollSpeed", scrollSpeedMs);
  prefs.putUChar("brightness", displayBrightness);
  prefs.putBool("animBoost", animationBrightnessBoost);
  prefs.putUChar("animBoostAmt", animationBrightnessBoostAmount);
  prefs.putUInt("holidayMin", holidayReminderMinutes);
  prefs.end();
}

String html_escape(String value) {
  value.replace("&", "&amp;");
  value.replace("\"", "&quot;");
  value.replace("'", "&#39;");
  value.replace("<", "&lt;");
  value.replace(">", "&gt;");
  return value;
}

String buildPage() {
  String page = String(CONFIG_HTML);
  String ip =
      configMode ? WiFi.softAPIP().toString() : WiFi.localIP().toString();
  page.replace("IP_PLACEHOLDER", ip);
  page.replace("SSID_PLACEHOLDER", html_escape(saved_ssid));
  page.replace("ENDPOINT_PLACEHOLDER", html_escape(saved_endpoint));
  page.replace("SUBS_CHECKED",
               (saved_stats & STAT_SUBSCRIBERS) ? "checked" : "");
  page.replace("VIEWS_CHECKED", (saved_stats & STAT_VIEWS) ? "checked" : "");
  page.replace("HOURS_CHECKED",
               (saved_stats & STAT_WATCH_HOURS) ? "checked" : "");
  page.replace("REFRESH_PLACEHOLDER", String(saved_refresh_minutes));
  page.replace("STAT_CYCLE_PLACEHOLDER", String(saved_stat_cycle_seconds, 3));
  page.replace("SCROLL_SPEED_PLACEHOLDER", String(saved_scroll_speed_ms));
  page.replace("BRIGHTNESS_PLACEHOLDER", String(saved_display_brightness));
  page.replace("ANIM_BOOST_CHECKED",
               saved_animation_brightness_boost ? "checked" : "");
  page.replace("ANIM_BOOST_AMOUNT_PLACEHOLDER",
               String(saved_animation_brightness_boost_amount));
  page.replace("HOLIDAY_REMINDER_PLACEHOLDER",
               String(saved_holiday_reminder_minutes));
  return page;
}

// ─── Server handlers
// ──────────────────────────────────────────────────────────
void handleRoot() { server.send(200, "text/html", buildPage()); }

void handleSave() {
  if (!server.hasArg("endpoint")) {
    server.send(400, "text/plain", "Stats API endpoint is required.");
    return;
  }

  String endpoint = server.arg("endpoint");
  endpoint.trim();
  if (endpoint.length() == 0) {
    server.send(400, "text/plain", "Stats API endpoint is required.");
    return;
  }
  if (!endpoint.startsWith("http://") && !endpoint.startsWith("https://")) {
    server.send(400, "text/plain",
                "Endpoint must start with http:// or https://");
    return;
  }

  uint8_t stats = server.arg("stats").toInt() & STAT_ALL;
  if (stats == 0) {
    server.send(400, "text/plain", "Choose at least one stat to show.");
    return;
  }

  int refreshMinutes = server.arg("refresh").toInt();
  if (refreshMinutes < 1)
    refreshMinutes = 1;
  if (refreshMinutes > MAX_REFRESH_MINUTES)
    refreshMinutes = MAX_REFRESH_MINUTES;

  float statCycleSeconds = server.arg("statCycle").toFloat();
  if (statCycleSeconds <= 0) {
    server.send(400, "text/plain",
                "Value display time must be greater than 0.");
    return;
  }

  unsigned int scrollSpeedMs = server.arg("scrollSpeed").toInt();
  if (scrollSpeedMs <= 0) {
    server.send(400, "text/plain", "Scroll speed must be greater than 0.");
    return;
  }

  int displayBrightness = server.arg("brightness").toInt();
  if (displayBrightness < 0) {
    displayBrightness = 0;
  }
  if (displayBrightness > 15) {
    displayBrightness = 15;
  }

  bool animationBrightnessBoost = server.arg("animBoost").toInt() != 0;
  int animationBrightnessBoostAmount = server.arg("animBoostAmount").toInt();
  if (animationBrightnessBoostAmount < 0) {
    animationBrightnessBoostAmount = 0;
  }
  if (animationBrightnessBoostAmount > 15) {
    animationBrightnessBoostAmount = 15;
  }

  int holidayReminderMinutes = server.arg("holidayReminder").toInt();
  if (holidayReminderMinutes < 0) {
    holidayReminderMinutes = 0;
  }
  if (holidayReminderMinutes > MAX_HOLIDAY_REMINDER_MINUTES) {
    holidayReminderMinutes = MAX_HOLIDAY_REMINDER_MINUTES;
  }

  String new_ssid = server.arg("ssid");
  new_ssid.trim();
  if (new_ssid.length() == 0) {
    if (WiFi.status() == WL_CONNECTED && WiFi.SSID() == WOKWI_GUEST_SSID) {
      new_ssid = WOKWI_GUEST_SSID;
    } else {
      new_ssid = saved_ssid;
    }
  }
  String new_pass = server.arg("pass");
  if (new_pass.length() == 0)
    new_pass = saved_pass;
  savePrefs(new_ssid, new_pass, endpoint, stats, refreshMinutes,
            statCycleSeconds, scrollSpeedMs, (uint8_t)displayBrightness,
            animationBrightnessBoost, (uint8_t)animationBrightnessBoostAmount,
            (unsigned int)holidayReminderMinutes);
  server.send(200, "text/plain", "Saved! Rebooting now...");
  delay(1500);
  ESP.restart();
}

void handleUpdate() {
  HTTPUpload &upload = server.upload();

  if (upload.status == UPLOAD_FILE_START) {
    Serial.printf("OTA start: %s\n", upload.filename.c_str());
    Display.print("OTA...");
    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (Update.end(true)) {
      Serial.printf("OTA success: %u bytes\n", upload.totalSize);
      Display.print("Rebooting");
    } else {
      Update.printError(Serial);
      Display.print("OTA fail");
    }
  }
}

void handleScan() {
  int n = WiFi.scanNetworks();
  String json = "[";
  for (int i = 0; i < n; i++) {
    if (i > 0)
      json += ",";
    json += "{\"ssid\":\"" + WiFi.SSID(i) +
            "\","
            "\"rssi\":" +
            String(WiFi.RSSI(i)) +
            ","
            "\"secure\":" +
            (WiFi.encryptionType(i) != WIFI_AUTH_OPEN ? "true" : "false") + "}";
  }
  json += "]";
  WiFi.scanDelete();
  server.send(200, "application/json", json);
}

void handleUpdateFinish() {
  if (Update.hasError()) {
    server.send(500, "text/plain", Update.errorString());
  } else {
    server.send(200, "text/plain", "OK");
    delay(1000);
    ESP.restart();
  }
}

void registerRoutes() {
  server.on("/", handleRoot);
  server.on("/scan", HTTP_GET, handleScan);
  server.on("/save", HTTP_POST, handleSave);
  server.on("/update", HTTP_POST, handleUpdateFinish, handleUpdate);
  server.onNotFound([]() {
    server.sendHeader("Location", "/");
    server.send(302, "text/plain", "");
  });
}

bool isStatSelected(int index) {
  return (saved_stats & STAT_MASKS[index]) != 0;
}

int selectedStatsCount() {
  int count = 0;
  for (int i = 0; i < STAT_COUNT; i++) {
    if (isStatSelected(i))
      count++;
  }
  return count;
}

int firstSelectedStatIndex() {
  for (int i = 0; i < STAT_COUNT; i++) {
    if (isStatSelected(i))
      return i;
  }
  return STAT_INDEX_SUBSCRIBERS;
}

int nextSelectedStatIndex(int currentIndex) {
  for (int step = 1; step <= STAT_COUNT; step++) {
    int index = (currentIndex + step) % STAT_COUNT;
    if (isStatSelected(index))
      return index;
  }
  return firstSelectedStatIndex();
}

void ensureCurrentStatSelected() {
  if (!isStatSelected(current_stat_index)) {
    current_stat_index = firstSelectedStatIndex();
  }
}

double interval_weights[INTERVAL_PATTERN_LENGTH];
bool interval_weights_initialized = false;

double getDeterministicNoise(uint32_t incrementNumber) {
  uint32_t hash = (incrementNumber ^ 0x9e3779b9UL) * 0x85ebca6bUL;
  hash ^= hash >> 13;
  hash *= 0xc2b2ae35UL;
  hash ^= hash >> 16;

  return (double)hash / 4294967296.0;
}

double getIntervalWeight(int patternIndex) {
  double noise = getDeterministicNoise((uint32_t)patternIndex + 1);

  if (noise < 0.22) {
    return 0.08 + (noise / 0.22) * 0.17;
  }

  if (noise < 0.42) {
    return 0.34 + ((noise - 0.22) / 0.2) * 0.36;
  }

  if (noise < 0.82) {
    return 0.9 + ((noise - 0.42) / 0.4) * 0.45;
  }

  return 1.8 + ((noise - 0.82) / 0.18) * 1.6;
}

void ensureIntervalWeightsInitialized() {
  if (interval_weights_initialized)
    return;

  double rawWeights[INTERVAL_PATTERN_LENGTH];
  double rawWeightTotal = 0;

  for (int i = 0; i < INTERVAL_PATTERN_LENGTH; i++) {
    rawWeights[i] = getIntervalWeight(i);
    rawWeightTotal += rawWeights[i];
  }

  for (int i = 0; i < INTERVAL_PATTERN_LENGTH; i++) {
    interval_weights[i] =
        (rawWeights[i] * INTERVAL_PATTERN_LENGTH) / rawWeightTotal;
  }

  interval_weights_initialized = true;
}

uint64_t getJitteredIncrementCount(double elapsedSeconds,
                                   double secondsPerIncrement) {
  ensureIntervalWeightsInitialized();

  double cycleSeconds = secondsPerIncrement * INTERVAL_PATTERN_LENGTH;
  uint64_t completedCycles = (uint64_t)floor(elapsedSeconds / cycleSeconds);
  uint64_t incrementCount = completedCycles * INTERVAL_PATTERN_LENGTH;
  double remainingSeconds = elapsedSeconds - (completedCycles * cycleSeconds);

  for (int i = 0; i < INTERVAL_PATTERN_LENGTH; i++) {
    double intervalSeconds = interval_weights[i] * secondsPerIncrement;

    if (remainingSeconds < intervalSeconds) {
      break;
    }

    remainingSeconds -= intervalSeconds;
    incrementCount += 1;
  }

  return incrementCount;
}

double getProjectedWholeStatValue(double baselineStartedAtUnix,
                                  double startingValue,
                                  double increasePer28Days,
                                  double currentUnixTimestamp) {
  double elapsedSeconds =
      max(0.0, currentUnixTimestamp - baselineStartedAtUnix);
  double secondsPerIncrement = SECONDS_PER_28_DAYS / increasePer28Days;

  if (!isfinite(secondsPerIncrement) || secondsPerIncrement <= 0) {
    return startingValue;
  }

  return startingValue +
         getJitteredIncrementCount(elapsedSeconds, secondsPerIncrement);
}

double getProjectedFractionalStatValue(double baselineStartedAtUnix,
                                       double startingValue,
                                       double increasePer28Days,
                                       double currentUnixTimestamp) {
  double elapsedSeconds =
      max(0.0, currentUnixTimestamp - baselineStartedAtUnix);
  double increasePerSecond = increasePer28Days / SECONDS_PER_28_DAYS;

  if (!isfinite(increasePerSecond) || increasePerSecond <= 0) {
    return startingValue;
  }

  return startingValue + elapsedSeconds * increasePerSecond;
}

double getProjectedStatValue(int statIndex, double currentUnixTimestamp) {
  if (statIndex == STAT_INDEX_WATCH_HOURS) {
    return getProjectedFractionalStatValue(
        stat_baseline_started_at_unix[statIndex],
        stat_baseline_values[statIndex],
        stat_increase_per_28_days[statIndex], currentUnixTimestamp);
  }

  return getProjectedWholeStatValue(
      stat_baseline_started_at_unix[statIndex], stat_baseline_values[statIndex],
      stat_increase_per_28_days[statIndex], currentUnixTimestamp);
}

bool fetchStats() {
  HTTPClient http;
  bool beginOk = false;

  Serial.print("Fetching stats: ");
  Serial.println(saved_endpoint);

  if (saved_endpoint.startsWith("https://")) {
    client.setInsecure();
    beginOk = http.begin(client, saved_endpoint);
  } else {
    beginOk = http.begin(saved_endpoint);
  }

  if (!beginOk) {
    Serial.println("HTTP begin failed.");
    return false;
  }

  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK) {
    Serial.print("Stats endpoint returned HTTP ");
    Serial.println(httpCode);
    http.end();
    return false;
  }

  String payload = http.getString();
  DeserializationError error = deserializeJson(doc, payload);
  http.end();

  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return false;
  }

  JsonObject baseline = doc["baseline"].as<JsonObject>();
  JsonObject metrics28Days = doc["metrics28Days"].as<JsonObject>();
  JsonObject adjusted = doc["adjusted"].as<JsonObject>();
  if (baseline.isNull() || metrics28Days.isNull() || adjusted.isNull()) {
    Serial.println(
        "Stats response missing baseline, metrics28Days, or adjusted.");
    return false;
  }

  stat_baseline_started_at_unix[STAT_INDEX_SUBSCRIBERS] =
      baseline["subscribersStartedAtUnix"] | 0.0;
  stat_baseline_started_at_unix[STAT_INDEX_VIEWS] =
      baseline["totalViewsStartedAtUnix"] | 0.0;
  stat_baseline_started_at_unix[STAT_INDEX_WATCH_HOURS] =
      baseline["watchHoursStartedAtUnix"] | 0.0;
  double server_time_unix = doc["serverTimeUnix"] | 0.0;
  stats_as_of_unix = adjusted["asOfUnix"] | 0.0;

  if (server_time_unix <= 0 || stats_as_of_unix <= 0) {
    Serial.println("Stats response missing valid Unix timestamps.");
    return false;
  }

  for (int statIndex = 0; statIndex < STAT_COUNT; statIndex++) {
    if (stat_baseline_started_at_unix[statIndex] <= 0) {
      Serial.println("Stats response missing valid baseline timestamps.");
      return false;
    }
  }

  stat_baseline_values[STAT_INDEX_SUBSCRIBERS] = baseline["subscribers"] | 0.0;
  stat_baseline_values[STAT_INDEX_VIEWS] = baseline["totalViews"] | 0.0;
  stat_baseline_values[STAT_INDEX_WATCH_HOURS] = baseline["watchHours"] | 0.0;
  stat_increase_per_28_days[STAT_INDEX_SUBSCRIBERS] =
      metrics28Days["subscribers"] | 0.0;
  stat_increase_per_28_days[STAT_INDEX_VIEWS] = metrics28Days["views"] | 0.0;
  stat_increase_per_28_days[STAT_INDEX_WATCH_HOURS] =
      metrics28Days["watchHours"] | 0.0;

  stats_fetched_at = millis();
  holidayEasterEggsSetServerTime(server_time_unix);
  statsLoaded = true;
  ensureCurrentStatSelected();

  Serial.println("Stats updated.");
  return true;
}

static char statDisplayBuffer[20];
static char statCombinedBuffer[32];
static String lastDisplayedValue = "";
static bool statDisplayScrolling = false;

enum StatExitEffect {
  EXIT_DISSOLVE,
  EXIT_WIPE,
  EXIT_DROP,
  EXIT_RISE,
  EXIT_CURTAIN,
  EXIT_CENTER_BURST,
  EXIT_SHRINK,
  EXIT_ROW_WIPE,
  EXIT_DIAGONAL_WIPE,
  EXIT_PIXEL_RAIN,
  EXIT_SPARKLE,
  EXIT_VENETIAN_BLINDS,
  EXIT_GLITCH,
  EXIT_COMPRESS_DOWN,
  EXIT_TYPEWRITER_ERASE,
  EXIT_EFFECT_COUNT
};
enum StatScrollPhase { SCROLL_NONE, SCROLL_EXIT, SCROLL_IN, SCROLL_LABEL_OUT };
static StatScrollPhase statScrollPhase = SCROLL_NONE;
static StatExitEffect statExitEffect = EXIT_DISSOLVE;
static bool statExitFrame[8][32];
static uint16_t statExitColStart = 0;
static uint16_t statExitColEnd = 0;
static uint16_t statExitFrameWidth = 0;
static uint8_t statExitStep = 0;
static uint8_t statExitMaxSteps = 0;
static const uint8_t STAT_EXIT_DISSOLVE_FRAMES = 9;
static const uint8_t STAT_EXIT_DROP_FRAMES = 8;
static const uint8_t STAT_EXIT_RISE_FRAMES = 8;
static const uint8_t STAT_EXIT_CURTAIN_COLUMNS_PER_FRAME = 3;
static const uint8_t STAT_EXIT_CENTER_COLUMNS_PER_FRAME = 3;
static const uint8_t STAT_EXIT_WIPE_COLUMNS_PER_FRAME = 3;
static const uint8_t STAT_EXIT_ROW_WIPE_ROWS_PER_FRAME = 1;
static const uint8_t STAT_EXIT_DIAGONAL_COLUMNS_PER_FRAME = 4;
static const uint8_t STAT_EXIT_PIXEL_RAIN_FRAMES = 9;
static const uint8_t STAT_EXIT_SPARKLE_FRAMES = 9;
static const uint8_t STAT_EXIT_GLITCH_FRAMES = 8;
static const uint8_t STAT_EXIT_COMPRESS_FRAMES = 8;
static const uint8_t STAT_EXIT_TYPEWRITER_COLUMNS_PER_FRAME = 4;
static int16_t statScrollStep = 0;
static int16_t statScrollLabelWidth = 0;
static int16_t statScrollNumberWidth = 0;
static int16_t statScrollInStartCol = 0;
static int16_t statScrollInSteps = 0;
static int16_t statScrollLabelOutSteps = 0;
static unsigned long statScrollLastStepMs = 0;
static double lastMilestoneCheckValue[STAT_COUNT] = {0, 0, 0};
static bool hasLastMilestoneCheckValue[STAT_COUNT] = {false, false, false};
static bool wifiDisconnectedIndicatorWasActive = false;

struct MilestoneRule {
  double interval;
  MilestoneAnimation animation;
};

static const MilestoneRule SUBSCRIBER_MILESTONES[] = {
    {10000000.0, MilestoneAnimation::Subs10M},
    {1000000.0, MilestoneAnimation::Subs1M},
    {100000.0, MilestoneAnimation::Subs100K},
    {10000.0, MilestoneAnimation::Subs10K},
    {1000.0, MilestoneAnimation::Subs1K},
    {100.0, MilestoneAnimation::Subs100},
};

static const MilestoneRule VIEW_MILESTONES[] = {
    {100000000.0, MilestoneAnimation::Views100M},
    {10000000.0, MilestoneAnimation::Views10M},
    {1000000.0, MilestoneAnimation::Views1M},
    {100000.0, MilestoneAnimation::Views100K},
    {10000.0, MilestoneAnimation::Views10K},
};

static const MilestoneRule HOURS_MILESTONES[] = {
    {10000000.0, MilestoneAnimation::Hours10M},
    {1000000.0, MilestoneAnimation::Hours1M},
    {100000.0, MilestoneAnimation::Hours100K},
    {10000.0, MilestoneAnimation::Hours10K},
    {1000.0, MilestoneAnimation::Hours1K},
    {100.0, MilestoneAnimation::Hours100},
};

static bool isDigitChar(char c) { return c >= '0' && c <= '9'; }

bool canRefreshStats() {
  if (WiFi.status() != WL_CONNECTED) {
    return false;
  }

  return !(DEBUG_SIMULATE_WIFI_DROP_AFTER_FIRST_STATS_FETCH && statsLoaded);
}

bool isWiFiDisconnectedForDisplay() {
  return WiFi.status() != WL_CONNECTED ||
         (DEBUG_SIMULATE_WIFI_DROP_AFTER_FIRST_STATS_FETCH && statsLoaded);
}

static void drawWiFiDisconnectedIndicator(bool on) {
  MD_MAX72XX *matrix = Display.getGraphicObject();
  uint16_t colStart, colEnd;
  Display.getDisplayExtent(colStart, colEnd);
  // Matrix column 0 is the physical right edge on FC16 chains.
  uint16_t rightCol = colEnd;
  uint16_t leftCol = colEnd > colStart ? colEnd - 1 : colEnd;

  matrix->update(MD_MAX72XX::OFF);
  for (uint8_t row = 0; row < 2; row++) {
    for (uint16_t col = leftCol; col <= rightCol; col++) {
      matrix->setPoint(row, col, on);
    }
  }
  matrix->update(MD_MAX72XX::ON);
}

void tickWiFiDisconnectedIndicator() {
  if (!isWiFiDisconnectedForDisplay()) {
    if (wifiDisconnectedIndicatorWasActive) {
      drawWiFiDisconnectedIndicator(false);
      wifiDisconnectedIndicatorWasActive = false;

      if (statsLoaded && !statDisplayScrolling &&
          statScrollPhase == SCROLL_NONE) {
        lastDisplayedValue = "";
        showProjectedStat();
      }
    }
    return;
  }

  unsigned long phase = millis() % WIFI_INDICATOR_PATTERN_MS;
  bool flashWindow = phase < WIFI_INDICATOR_FLASH_MS * 6;
  bool indicatorOn =
      flashWindow && ((phase / WIFI_INDICATOR_FLASH_MS) % 2 == 0);
  drawWiFiDisconnectedIndicator(indicatorOn);
  wifiDisconnectedIndicatorWasActive = true;
}

static bool crossedInterval(double previousValue, double currentValue,
                            double interval) {
  return floor(previousValue / interval) < floor(currentValue / interval);
}

static bool findCrossedMilestoneAnimation(int statIndex, double previousValue,
                                          double currentValue,
                                          MilestoneAnimation &animation) {
  if (!isfinite(previousValue) || !isfinite(currentValue) ||
      currentValue <= previousValue) {
    return false;
  }

  const MilestoneRule *rules = SUBSCRIBER_MILESTONES;
  size_t ruleCount =
      sizeof(SUBSCRIBER_MILESTONES) / sizeof(SUBSCRIBER_MILESTONES[0]);

  if (statIndex == STAT_INDEX_VIEWS) {
    rules = VIEW_MILESTONES;
    ruleCount = sizeof(VIEW_MILESTONES) / sizeof(VIEW_MILESTONES[0]);
  } else if (statIndex == STAT_INDEX_WATCH_HOURS) {
    rules = HOURS_MILESTONES;
    ruleCount = sizeof(HOURS_MILESTONES) / sizeof(HOURS_MILESTONES[0]);
  }

  for (size_t i = 0; i < ruleCount; i++) {
    if (crossedInterval(previousValue, currentValue, rules[i].interval)) {
      animation = rules[i].animation;
      return true;
    }
  }

  return false;
}

bool checkAndRunMilestoneAnimation() {
  if (!statsLoaded)
    return false;

  double currentUnixTimestamp =
      stats_as_of_unix + ((millis() - stats_fetched_at) / 1000.0);

  for (int statIndex = 0; statIndex < STAT_COUNT; statIndex++) {
    if (!isStatSelected(statIndex))
      continue;

    double projected = getProjectedStatValue(statIndex, currentUnixTimestamp);

    if (!hasLastMilestoneCheckValue[statIndex]) {
      lastMilestoneCheckValue[statIndex] = projected;
      hasLastMilestoneCheckValue[statIndex] = true;
      continue;
    }

    MilestoneAnimation animation;
    if (findCrossedMilestoneAnimation(statIndex,
                                      lastMilestoneCheckValue[statIndex],
                                      projected, animation)) {
      lastMilestoneCheckValue[statIndex] = projected;
      Serial.print("Milestone crossed: ");
      Serial.println(STAT_LABELS[statIndex]);
      runMilestoneAnimation(Display, animation);
      lastDisplayedValue = "";
      statDisplayScrolling = false;
      statScrollPhase = SCROLL_NONE;
      return true;
    }

    lastMilestoneCheckValue[statIndex] = projected;
  }

  return false;
}

static bool shouldRollLastDigit(const String &oldValue,
                                const String &newValue) {
  if (oldValue.length() == 0 || oldValue.length() != newValue.length())
    return false;

  int lastIndex = newValue.length() - 1;
  return isDigitChar(oldValue[lastIndex]) && isDigitChar(newValue[lastIndex]) &&
         oldValue[lastIndex] != newValue[lastIndex];
}

static void drawGlyphWithYOffset(MD_MAX72XX *matrix, uint16_t rightCol, char c,
                                 int8_t yOffset) {
  uint8_t glyph[8];
  uint8_t glyphWidth = matrix->getChar(c, sizeof(glyph), glyph);

  for (uint8_t glyphCol = 0; glyphCol < glyphWidth; glyphCol++) {
    uint16_t matrixCol = rightCol - glyphCol;
    for (uint8_t row = 0; row < 8; row++) {
      int targetRow = (int)row + yOffset;
      if (targetRow < 0 || targetRow >= 8)
        continue;

      if ((glyph[glyphCol] & (1 << row)) != 0) {
        matrix->setPoint((uint8_t)targetRow, matrixCol, true);
      }
    }
  }
}

static void renderRightAlignedStatFrame(const char *newText, char oldLastDigit,
                                        int8_t rollOffset) {
  MD_MAX72XX *matrix = Display.getGraphicObject();
  uint8_t charSpacing = Display.getCharSpacing();
  uint16_t col = STAT_RIGHT_PADDING_COLUMNS;
  uint8_t glyph[8];
  int lastIndex = (int)strlen(newText) - 1;

  matrix->clear();

  for (int i = lastIndex; i >= 0; i--) {
    uint8_t glyphWidth = matrix->getChar(newText[i], sizeof(glyph), glyph);
    if (glyphWidth == 0)
      continue;

    uint16_t rightCol = col + glyphWidth - 1;
    if (i == lastIndex && rollOffset > 0) {
      drawGlyphWithYOffset(matrix, rightCol, oldLastDigit, -rollOffset);
      drawGlyphWithYOffset(matrix, rightCol, newText[i], 8 - rollOffset);
    } else {
      matrix->setChar(rightCol, newText[i]);
    }
    col += glyphWidth + charSpacing;
  }
}

static void renderTextAnchored(const char *text, int16_t firstCol) {
  MD_MAX72XX *matrix = Display.getGraphicObject();
  uint8_t charSpacing = Display.getCharSpacing();
  int16_t col = firstCol;
  uint8_t glyph[8];
  uint16_t startCol, endCol;
  Display.getDisplayExtent(startCol, endCol);

  for (int i = (int)strlen(text) - 1; i >= 0; i--) {
    uint8_t glyphWidth = matrix->getChar(text[i], sizeof(glyph), glyph);
    if (glyphWidth == 0)
      continue;

    int16_t charRight = col + (int16_t)glyphWidth - 1;
    int16_t charLeft = col;
    if (charRight >= (int16_t)startCol && charLeft <= (int16_t)endCol) {
      matrix->setChar((uint16_t)charRight, text[i]);
    }
    col += (int16_t)glyphWidth + (int16_t)charSpacing;
  }
}

static void renderTextToExitFrame(const char *text, int16_t firstCol) {
  MD_MAX72XX *matrix = Display.getGraphicObject();
  uint8_t charSpacing = Display.getCharSpacing();
  int16_t col = firstCol;
  uint8_t glyph[8];

  for (int i = (int)strlen(text) - 1; i >= 0; i--) {
    uint8_t glyphWidth = matrix->getChar(text[i], sizeof(glyph), glyph);
    if (glyphWidth == 0)
      continue;

    int16_t charRight = col + (int16_t)glyphWidth - 1;
    for (uint8_t glyphCol = 0; glyphCol < glyphWidth; glyphCol++) {
      int16_t matrixCol = charRight - (int16_t)glyphCol;
      if (matrixCol < (int16_t)statExitColStart ||
          matrixCol > (int16_t)statExitColEnd) {
        continue;
      }

      uint16_t localCol = (uint16_t)(matrixCol - (int16_t)statExitColStart);
      for (uint8_t row = 0; row < 8; row++) {
        if ((glyph[glyphCol] & (1 << row)) != 0) {
          statExitFrame[row][localCol] = true;
        }
      }
    }
    col += (int16_t)glyphWidth + (int16_t)charSpacing;
  }
}

static void captureStatExitFrame() {
  MD_MAX72XX *matrix = Display.getGraphicObject();
  Display.getDisplayExtent(statExitColStart, statExitColEnd);
  statExitFrameWidth = statExitColEnd - statExitColStart + 1;

  for (uint8_t row = 0; row < 8; row++) {
    for (uint16_t col = 0; col < statExitFrameWidth; col++) {
      statExitFrame[row][col] = false;
    }
  }

  if (statDisplayScrolling) {
    for (uint8_t row = 0; row < 8; row++) {
      for (uint16_t col = statExitColStart; col <= statExitColEnd; col++) {
        statExitFrame[row][col - statExitColStart] = matrix->getPoint(row, col);
      }
    }
    return;
  }

  renderTextToExitFrame(statDisplayBuffer, STAT_RIGHT_PADDING_COLUMNS);
}

static uint8_t ceilDiv16(uint16_t value, uint8_t divisor) {
  return (uint8_t)((value + divisor - 1) / divisor);
}

static uint8_t getStatExitMaxSteps(StatExitEffect effect) {
  switch (effect) {
  case EXIT_WIPE:
    return ceilDiv16(statExitFrameWidth, STAT_EXIT_WIPE_COLUMNS_PER_FRAME) + 1;
  case EXIT_DISSOLVE:
    return STAT_EXIT_DISSOLVE_FRAMES;
  case EXIT_DROP:
    return STAT_EXIT_DROP_FRAMES;
  case EXIT_RISE:
    return STAT_EXIT_RISE_FRAMES;
  case EXIT_CURTAIN:
    return ceilDiv16((statExitFrameWidth + 1) / 2,
                     STAT_EXIT_CURTAIN_COLUMNS_PER_FRAME) +
           1;
  case EXIT_CENTER_BURST:
    return ceilDiv16((statExitFrameWidth + 1) / 2,
                     STAT_EXIT_CENTER_COLUMNS_PER_FRAME) +
           1;
  case EXIT_SHRINK:
    return 7;
  case EXIT_ROW_WIPE:
    return ceilDiv16(8, STAT_EXIT_ROW_WIPE_ROWS_PER_FRAME) + 1;
  case EXIT_DIAGONAL_WIPE:
    return ceilDiv16(statExitFrameWidth + 7,
                     STAT_EXIT_DIAGONAL_COLUMNS_PER_FRAME) +
           1;
  case EXIT_PIXEL_RAIN:
    return STAT_EXIT_PIXEL_RAIN_FRAMES;
  case EXIT_SPARKLE:
    return STAT_EXIT_SPARKLE_FRAMES;
  case EXIT_VENETIAN_BLINDS:
    return 5;
  case EXIT_GLITCH:
    return STAT_EXIT_GLITCH_FRAMES;
  case EXIT_COMPRESS_DOWN:
    return STAT_EXIT_COMPRESS_FRAMES;
  case EXIT_TYPEWRITER_ERASE:
    return ceilDiv16(statExitFrameWidth,
                     STAT_EXIT_TYPEWRITER_COLUMNS_PER_FRAME) +
           1;
  case EXIT_EFFECT_COUNT:
    return STAT_EXIT_DISSOLVE_FRAMES;
  }

  return STAT_EXIT_DISSOLVE_FRAMES;
}

static void advanceStatExitDissolve() {
  bool anyRemaining = false;

  for (uint8_t row = 0; row < 8; row++) {
    for (uint16_t col = 0; col < statExitFrameWidth; col++) {
      if (!statExitFrame[row][col]) {
        continue;
      }
      if (random(100) < 40) {
        statExitFrame[row][col] = false;
      } else {
        anyRemaining = true;
      }
    }
  }

  if (!anyRemaining) {
    statExitStep = statExitMaxSteps;
  }
}

static void advanceStatExitDrop() {
  bool nextFrame[8][32] = {{false}};
  bool anyRemaining = false;

  for (uint8_t row = 0; row < 8; row++) {
    for (uint16_t col = 0; col < statExitFrameWidth; col++) {
      if (!statExitFrame[row][col]) {
        continue;
      }
      if (row < 7) {
        nextFrame[row + 1][col] = true;
        anyRemaining = true;
      }
    }
  }

  for (uint8_t row = 0; row < 8; row++) {
    for (uint16_t col = 0; col < statExitFrameWidth; col++) {
      statExitFrame[row][col] = nextFrame[row][col];
    }
  }

  if (!anyRemaining) {
    statExitStep = statExitMaxSteps;
  }
}

static void advanceStatExitRise() {
  bool nextFrame[8][32] = {{false}};
  bool anyRemaining = false;

  for (uint8_t row = 0; row < 8; row++) {
    for (uint16_t col = 0; col < statExitFrameWidth; col++) {
      if (!statExitFrame[row][col]) {
        continue;
      }
      if (row >= 1) {
        nextFrame[row - 1][col] = true;
        anyRemaining = true;
      }
    }
  }

  for (uint8_t row = 0; row < 8; row++) {
    for (uint16_t col = 0; col < statExitFrameWidth; col++) {
      statExitFrame[row][col] = nextFrame[row][col];
    }
  }

  if (!anyRemaining) {
    statExitStep = statExitMaxSteps;
  }
}

static void advanceStatExitPixelRain() {
  bool nextFrame[8][32] = {{false}};
  bool anyRemaining = false;

  for (uint8_t row = 0; row < 8; row++) {
    for (uint16_t col = 0; col < statExitFrameWidth; col++) {
      if (!statExitFrame[row][col] || random(100) < 12) {
        continue;
      }

      uint8_t nextRow = row + 1 + random(2);
      if (nextRow < 8) {
        nextFrame[nextRow][col] = true;
        anyRemaining = true;
      }
    }
  }

  for (uint8_t row = 0; row < 8; row++) {
    for (uint16_t col = 0; col < statExitFrameWidth; col++) {
      statExitFrame[row][col] = nextFrame[row][col];
    }
  }

  if (!anyRemaining) {
    statExitStep = statExitMaxSteps;
  }
}

static void advanceStatExitCompressDown() {
  bool nextFrame[8][32] = {{false}};
  bool anyRemaining = false;

  for (uint16_t col = 0; col < statExitFrameWidth; col++) {
    uint8_t litCount = 0;
    for (uint8_t row = 0; row < 8; row++) {
      if (statExitFrame[row][col]) {
        litCount++;
      }
    }

    if (litCount == 0) {
      continue;
    }

    if (litCount > 1 || random(100) < 45) {
      litCount--;
    }

    for (uint8_t i = 0; i < litCount; i++) {
      nextFrame[7 - i][col] = true;
      anyRemaining = true;
    }
  }

  for (uint8_t row = 0; row < 8; row++) {
    for (uint16_t col = 0; col < statExitFrameWidth; col++) {
      statExitFrame[row][col] = nextFrame[row][col];
    }
  }

  if (!anyRemaining) {
    statExitStep = statExitMaxSteps;
  }
}

static bool shouldHideStatExitPixel(uint8_t row, uint16_t localCol) {
  uint16_t amount;

  switch (statExitEffect) {
  case EXIT_WIPE:
    amount = (uint16_t)statExitStep * STAT_EXIT_WIPE_COLUMNS_PER_FRAME;
    return amount >= statExitFrameWidth ||
           localCol >= statExitFrameWidth - amount;
  case EXIT_CURTAIN:
    amount = (uint16_t)statExitStep * STAT_EXIT_CURTAIN_COLUMNS_PER_FRAME;
    return amount >= (statExitFrameWidth + 1) / 2 || localCol < amount ||
           localCol >= statExitFrameWidth - amount;
  case EXIT_CENTER_BURST: {
    int16_t center2 = (int16_t)statExitFrameWidth - 1;
    int16_t distance2 = abs((int16_t)(localCol * 2) - center2);
    amount = (uint16_t)statExitStep * STAT_EXIT_CENTER_COLUMNS_PER_FRAME;
    return distance2 <= (int16_t)(amount * 2);
  }
  case EXIT_SHRINK: {
    uint8_t rowInset = statExitStep;
    uint16_t colInset = (uint16_t)statExitStep * 4;
    if (rowInset >= 4 || colInset * 2 >= statExitFrameWidth) {
      return true;
    }
    return row < rowInset || row >= 8 - rowInset || localCol < colInset ||
           localCol >= statExitFrameWidth - colInset;
  }
  case EXIT_ROW_WIPE:
    amount = (uint16_t)statExitStep * STAT_EXIT_ROW_WIPE_ROWS_PER_FRAME;
    return row < amount;
  case EXIT_DIAGONAL_WIPE:
    amount = (uint16_t)statExitStep * STAT_EXIT_DIAGONAL_COLUMNS_PER_FRAME;
    return localCol + row < amount;
  case EXIT_VENETIAN_BLINDS:
    return (statExitStep >= 1 && (localCol % 2) == 0) ||
           (statExitStep >= 2 && (localCol % 2) == 1);
  case EXIT_TYPEWRITER_ERASE:
    amount = (uint16_t)statExitStep * STAT_EXIT_TYPEWRITER_COLUMNS_PER_FRAME;
    return amount >= statExitFrameWidth ||
           localCol >= statExitFrameWidth - amount;
  default:
    return false;
  }
}

static void renderStatExitFrame() {
  MD_MAX72XX *matrix = Display.getGraphicObject();

  matrix->update(MD_MAX72XX::OFF);
  matrix->clear();

  for (uint8_t row = 0; row < 8; row++) {
    for (uint16_t localCol = 0; localCol < statExitFrameWidth; localCol++) {
      bool pixelOn = statExitFrame[row][localCol];
      if (statExitEffect == EXIT_SPARKLE && !pixelOn && random(100) < 4) {
        pixelOn = true;
      }

      if (!pixelOn || shouldHideStatExitPixel(row, localCol)) {
        continue;
      }

      uint16_t matrixCol = statExitColStart + localCol;

      if (statExitEffect == EXIT_GLITCH) {
        if (random(100) < statExitStep * 12) {
          continue;
        }

        int16_t glitchRow = (int16_t)row + (int16_t)random(-1, 2);
        int16_t glitchCol = (int16_t)matrixCol + (int16_t)random(-1, 2);
        if (glitchRow < 0 || glitchRow >= 8 ||
            glitchCol < (int16_t)statExitColStart ||
            glitchCol > (int16_t)statExitColEnd) {
          continue;
        }

        matrix->setPoint((uint8_t)glitchRow, (uint16_t)glitchCol, true);
      } else {
        matrix->setPoint(row, matrixCol, true);
      }
    }
  }

  matrix->update(MD_MAX72XX::ON);
}

void startStatExitTransition() {
  if (selectedStatsCount() <= 1 || !statsLoaded ||
      statScrollPhase != SCROLL_NONE) {
    return;
  }

  statDisplayScrolling = false;
  Display.displayClear();
  captureStatExitFrame();

  statExitEffect = (StatExitEffect)random((long)EXIT_EFFECT_COUNT);
  statExitStep = 0;
  statExitMaxSteps = getStatExitMaxSteps(statExitEffect);

  statScrollPhase = SCROLL_EXIT;
  statScrollLastStepMs = millis();
  renderStatExitFrame();
}

void renderRightAlignedStat(const char *text) {
  MD_MAX72XX *matrix = Display.getGraphicObject();

  matrix->update(MD_MAX72XX::OFF);
  matrix->clear();
  renderTextAnchored(text, STAT_RIGHT_PADDING_COLUMNS);
  matrix->update(MD_MAX72XX::ON);
}

static void renderStatScrollFrame() {
  MD_MAX72XX *matrix = Display.getGraphicObject();

  matrix->update(MD_MAX72XX::OFF);
  matrix->clear();

  if (statScrollPhase == SCROLL_IN) {
    int16_t numberFirstCol = statScrollInStartCol + statScrollStep;
    int16_t labelFirstCol =
        numberFirstCol + statScrollNumberWidth + STAT_LABEL_NUMBER_GAP_COLUMNS;
    renderTextAnchored(statDisplayBuffer, numberFirstCol);
    renderTextAnchored(STAT_LABELS[current_stat_index], labelFirstCol);
  } else if (statScrollPhase == SCROLL_LABEL_OUT) {
    renderTextAnchored(statDisplayBuffer, STAT_RIGHT_PADDING_COLUMNS);
    int16_t labelFirstCol = (int16_t)STAT_RIGHT_PADDING_COLUMNS +
                            statScrollNumberWidth +
                            STAT_LABEL_NUMBER_GAP_COLUMNS + statScrollStep;
    renderTextAnchored(STAT_LABELS[current_stat_index], labelFirstCol);
  }

  matrix->update(MD_MAX72XX::ON);
}

void renderRightAlignedStatRollingLastDigit(const char *oldText,
                                            const char *newText) {
  MD_MAX72XX *matrix = Display.getGraphicObject();
  char oldLastDigit = oldText[strlen(oldText) - 1];

  matrix->update(MD_MAX72XX::OFF);
  for (int8_t offset = 1; offset <= 8; offset++) {
    renderRightAlignedStatFrame(newText, oldLastDigit, offset);
    matrix->update();
    delay(DIGIT_ROLL_FRAME_DELAY_MS);
  }
  matrix->update(MD_MAX72XX::ON);
}

static bool tickOverflowStatScroll() {
  if (!statDisplayScrolling || statScrollPhase != SCROLL_NONE) {
    return false;
  }

  if (!Display.displayAnimate()) {
    return true;
  }

  if (selectedStatsCount() > 1) {
    statDisplayScrolling = false;
    lastDisplayedValue = "";
    current_stat_index = nextSelectedStatIndex(current_stat_index);
    cycle_lasttime = millis();
    display_lasttime = cycle_lasttime;
    startStatScrollIn();
  } else {
    Display.displayReset();
  }

  return true;
}

void startStatScrollIn() {
  if (selectedStatsCount() <= 1 || !statsLoaded)
    return;

  ensureCurrentStatSelected();
  statDisplayScrolling = false;
  lastDisplayedValue = "";

  double currentUnixTimestamp =
      stats_as_of_unix + ((millis() - stats_fetched_at) / 1000.0);
  double projected =
      getProjectedStatValue(current_stat_index, currentUnixTimestamp);
  String formatted = stat_format(projected, current_stat_index);
  String displayText = getStatDisplayText(current_stat_index, formatted);
  displayText.toCharArray(statDisplayBuffer, sizeof(statDisplayBuffer));

  uint16_t startCol, endCol;
  Display.getDisplayExtent(startCol, endCol);
  uint16_t displayWidth = endCol - startCol + 1;
  statScrollNumberWidth = Display.getTextColumns(statDisplayBuffer);

  if (statScrollNumberWidth + STAT_RIGHT_PADDING_COLUMNS > displayWidth) {
    statScrollPhase = SCROLL_NONE;
    statDisplayScrolling = true;
    Display.displayScroll(statDisplayBuffer, PA_RIGHT, PA_SCROLL_LEFT,
                          saved_scroll_speed_ms);
    lastDisplayedValue = displayText;
    cycle_lasttime = millis();
    Serial.println(formatted);
    return;
  }

  snprintf(statCombinedBuffer, sizeof(statCombinedBuffer), "%s %s",
           STAT_LABELS[current_stat_index], statDisplayBuffer);

  statScrollLabelWidth =
      Display.getTextColumns(STAT_LABELS[current_stat_index]);
  int16_t combinedWidth = statScrollNumberWidth +
                          STAT_LABEL_NUMBER_GAP_COLUMNS + statScrollLabelWidth;
  statScrollInStartCol = -(int16_t)combinedWidth - STAT_SCROLL_OFFSCREEN_MARGIN;
  statScrollInSteps =
      (int16_t)STAT_RIGHT_PADDING_COLUMNS - statScrollInStartCol;
  statScrollLabelOutSteps = statScrollLabelWidth + 4;
  statScrollStep = 0;
  statScrollPhase = SCROLL_IN;
  statScrollLastStepMs = millis();
  renderStatScrollFrame();
  Serial.println(statCombinedBuffer);
}

bool tickStatScrollAnimation() {
  if (statScrollPhase == SCROLL_NONE)
    return false;

  unsigned long now = millis();
  if (now - statScrollLastStepMs < saved_scroll_speed_ms)
    return true;

  statScrollLastStepMs = now;

  if (statScrollPhase == SCROLL_EXIT) {
    statExitStep++;
    if (statExitStep >= statExitMaxSteps) {
      MD_MAX72XX *matrix = Display.getGraphicObject();
      matrix->update(MD_MAX72XX::OFF);
      matrix->clear();
      matrix->update(MD_MAX72XX::ON);
      statScrollPhase = SCROLL_NONE;
      lastDisplayedValue = "";
      current_stat_index = nextSelectedStatIndex(current_stat_index);
      startStatScrollIn();
      return true;
    }
    if (statExitEffect == EXIT_DISSOLVE) {
      advanceStatExitDissolve();
    } else if (statExitEffect == EXIT_DROP) {
      advanceStatExitDrop();
    } else if (statExitEffect == EXIT_RISE) {
      advanceStatExitRise();
    } else if (statExitEffect == EXIT_PIXEL_RAIN) {
      advanceStatExitPixelRain();
    } else if (statExitEffect == EXIT_SPARKLE) {
      advanceStatExitDissolve();
    } else if (statExitEffect == EXIT_COMPRESS_DOWN) {
      advanceStatExitCompressDown();
    }
    renderStatExitFrame();
    return true;
  }

  if (statScrollPhase == SCROLL_IN) {
    statScrollStep++;
    if (statScrollStep > statScrollInSteps) {
      statScrollPhase = SCROLL_LABEL_OUT;
      statScrollStep = 0;
    }
  } else if (statScrollPhase == SCROLL_LABEL_OUT) {
    statScrollStep++;
    if (statScrollStep >= statScrollLabelOutSteps) {
      statScrollPhase = SCROLL_NONE;
      renderRightAlignedStat(statDisplayBuffer);
      lastDisplayedValue = String(statDisplayBuffer);
      cycle_lasttime = now;
      display_lasttime = now;
      return true;
    }
  }

  renderStatScrollFrame();
  return true;
}

void startStatDisplay() {
  if (selectedStatsCount() > 1) {
    startStatScrollIn();
  } else {
    lastDisplayedValue = "";
    showProjectedStat();
  }
}

void showProjectedStat() {
  if (!statsLoaded)
    return;

  ensureCurrentStatSelected();
  double currentUnixTimestamp =
      stats_as_of_unix + ((millis() - stats_fetched_at) / 1000.0);
  double projected =
      getProjectedStatValue(current_stat_index, currentUnixTimestamp);

  String formatted = stat_format(projected, current_stat_index);
  String displayText = getStatDisplayText(current_stat_index, formatted);
  if (displayText == lastDisplayedValue)
    return;

  String previousDisplayedValue = lastDisplayedValue;
  bool wasDisplayScrolling = statDisplayScrolling;
  displayText.toCharArray(statDisplayBuffer, sizeof(statDisplayBuffer));
  Serial.println(displayText);

  uint16_t startCol, endCol;
  Display.getDisplayExtent(startCol, endCol);
  uint16_t displayWidth = endCol - startCol + 1;
  uint16_t textWidth = Display.getTextColumns(statDisplayBuffer);

  if (textWidth + STAT_RIGHT_PADDING_COLUMNS <= displayWidth) {
    statDisplayScrolling = false;
    if (!wasDisplayScrolling &&
        shouldRollLastDigit(previousDisplayedValue, formatted)) {
      renderRightAlignedStatRollingLastDigit(previousDisplayedValue.c_str(),
                                             statDisplayBuffer);
    } else {
      renderRightAlignedStat(statDisplayBuffer);
    }
  } else {
    statDisplayScrolling = true;
    Display.displayScroll(statDisplayBuffer, PA_RIGHT, PA_SCROLL_LEFT,
                          saved_scroll_speed_ms);
  }
  lastDisplayedValue = displayText;
}

bool startWokwiConfigPortal() {
  Display.print(" Wokwi...");
  Serial.println("Trying Wokwi-GUEST for setup portal...");

  WiFi.mode(WIFI_STA);
  WiFi.begin(WOKWI_GUEST_SSID);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED &&
         millis() - start < WOKWI_SETUP_TIMEOUT_MS) {
    delay(250);
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wokwi-GUEST not found.");
    return false;
  }

  configMode = true;
  registerRoutes();
  server.begin();

  Serial.println("Wokwi setup portal ready.");
  Serial.println("Open http://localhost:8180 in your browser.");
  Serial.print("ESP IP: ");
  Serial.println(WiFi.localIP());

  initSetupDisplay();
  return true;
}

void getSetupMacSuffix(char *suffix, size_t size) {
  uint8_t mac[6];
  WiFi.macAddress(mac);
  snprintf(suffix, size, "%02X%02X", mac[4], mac[5]);
}

String getSetupApSsid() {
  char suffix[5];
  getSetupMacSuffix(suffix, sizeof(suffix));
  return String(AP_SSID_PREFIX) + suffix;
}

static void bootShowCenteredText(const char *text, uint16_t holdMs) {
  MD_MAX72XX *matrix = Display.getGraphicObject();
  matrix->control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
  matrix->clear();
  matrix->update();
  Display.displayClear();
  Display.setTextAlignment(PA_CENTER);
  Display.setIntensity(animationDisplayIntensity(0));
  Display.print(text);
  delay(holdMs);
  matrix->control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);
}

static void bootAnimRain(MD_MAX72XX *matrix, uint16_t colStart, uint16_t colEnd,
                         int width, int height) {
  auto drawPixel = [matrix](uint16_t col, uint8_t row, bool on) {
    matrix->setPoint(row, col, on);
  };

  uint8_t dropHead[32];
  uint8_t dropSpeed[32];
  for (int c = 0; c < width; c++) {
    dropHead[c] = random(height + 4);
    dropSpeed[c] = 1 + random(3);
  }

  for (int frame = 0; frame < 42; frame++) {
    matrix->clear();
    for (int c = 0; c < width; c++) {
      int col = colStart + c;
      if (frame % dropSpeed[c] == c % dropSpeed[c]) {
        dropHead[c] = (dropHead[c] + 1) % (height + 6);
      }
      for (int trail = 0; trail < 6; trail++) {
        int row = (int)dropHead[c] - trail;
        if (row >= 0 && row < height) {
          drawPixel(col, row, trail < 2);
        }
      }
    }
    Display.setIntensity(animationDisplayIntensity(frame / 3));
    matrix->update();
    delay(26);
  }
}

static void bootAnimPlasma(MD_MAX72XX *matrix, uint16_t colStart, int width,
                           int height) {
  auto drawPixel = [matrix](uint16_t col, uint8_t row, bool on) {
    matrix->setPoint(row, col, on);
  };

  for (int frame = 0; frame < 48; frame++) {
    matrix->clear();
    float t = frame * 0.21f;
    float threshold = -0.35f + (frame / 48.0f) * 0.25f;

    for (int c = 0; c < width; c++) {
      int col = colStart + c;
      float cx = c - width * 0.5f;
      for (int row = 0; row < height; row++) {
        float cy = row - height * 0.5f;
        float dist = sqrtf(cx * cx + cy * cy);
        float v = sinf(c * 0.38f + t) + sinf(row * 0.62f - t * 1.35f) +
                  sinf(dist * 0.48f - t * 1.8f) +
                  sinf((c + row) * 0.28f + t * 0.55f);
        if (v > threshold) {
          drawPixel(col, row, true);
        }
      }
    }
    Display.setIntensity(animationDisplayIntensity(frame / 4));
    matrix->update();
    delay(28);
  }
}

static void bootAnimSpectrum(MD_MAX72XX *matrix, uint16_t colStart, int width,
                             int height) {
  auto drawPixel = [matrix](uint16_t col, uint8_t row, bool on) {
    matrix->setPoint(row, col, on);
  };

  uint8_t barHeight[32];
  int8_t barDelta[32];
  for (int c = 0; c < width; c++) {
    barHeight[c] = 1 + random(height);
    barDelta[c] = random(2) ? 1 : -1;
  }

  for (int frame = 0; frame < 38; frame++) {
    matrix->clear();
    for (int c = 0; c < width; c++) {
      int col = colStart + c;
      if (frame % 2 == c % 2) {
        barHeight[c] += barDelta[c];
        if (barHeight[c] >= height || barHeight[c] <= 1) {
          barDelta[c] = -barDelta[c];
          barHeight[c] = constrain(barHeight[c], 1, height);
        }
      }
      for (int row = height - barHeight[c]; row < height; row++) {
        drawPixel(col, row, true);
      }
    }
    Display.setIntensity(animationDisplayIntensity(0));
    matrix->update();
    delay(32);
  }
}

static void bootAnimFinale(MD_MAX72XX *matrix, uint16_t colStart,
                           uint16_t colEnd, int width, int height) {
  auto drawPixel = [matrix](uint16_t col, uint8_t row, bool on) {
    matrix->setPoint(row, col, on);
  };

  for (int sweep = -2; sweep <= width + 3; sweep++) {
    matrix->clear();
    for (int row = 0; row < height; row++) {
      for (int c = 0; c < width; c++) {
        int col = colStart + c;
        bool bg = ((c + row * 2 + sweep) % 5) < 2;
        drawPixel(col, row, bg);
      }
    }
    for (int echo = 0; echo < 5; echo++) {
      int sweepCol = colStart + sweep - echo;
      if (sweepCol >= colStart && sweepCol <= colEnd) {
        for (int row = 0; row < height; row++) {
          drawPixel(sweepCol, row, echo < 2);
        }
      }
    }
    Display.setIntensity(animationDisplayIntensity(
        max(0, 11 - abs(sweep - width / 2) / 3)));
    matrix->update();
    delay(20);
  }

  for (int col = colStart; col <= colEnd; col++) {
    for (int row = 0; row < height; row++) {
      drawPixel(col, row, true);
    }
  }
  matrix->update();
  Display.setIntensity(animationDisplayIntensity(12));
  delay(45);

  for (int frame = 0; frame < 18; frame++) {
    for (int col = colStart; col <= colEnd; col++) {
      for (int row = 0; row < height; row++) {
        if (random(100) < frame * 7) {
          drawPixel(col, row, false);
        }
      }
    }
    matrix->update();
    Display.setIntensity(animationDisplayIntensity(max(0, 11 - frame)));
    delay(22);
  }
}

void runBootAnimation() {
  MD_MAX72XX *matrix = Display.getGraphicObject();
  uint16_t colStart, colEnd;
  Display.getDisplayExtent(colStart, colEnd);
  const int width = colEnd - colStart + 1;
  const int height = 8;

  matrix->control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);

  bootAnimRain(matrix, colStart, colEnd, width, height);
  bootAnimPlasma(matrix, colStart, width, height);
  bootAnimSpectrum(matrix, colStart, width, height);
  bootShowCenteredText("YouTube", 600);
  bootShowCenteredText("Stats", 600);
  bootShowCenteredText("Counter!", 1300);

  bootAnimFinale(matrix, colStart, colEnd, width, height);

  matrix->clear();
  matrix->update();
  matrix->control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
  Display.setIntensity(displayBaselineIntensity());
  Display.displayClear();
}

void initSetupDisplay() {
  getSetupMacSuffix(setupMacSuffix, sizeof(setupMacSuffix));
  snprintf(setupScrollBuffer, sizeof(setupScrollBuffer),
           "Connect to hotspot %s%s", AP_SSID_PREFIX, setupMacSuffix);
  Display.displayScroll(setupScrollBuffer, PA_LEFT, PA_SCROLL_LEFT, 80);
}

void updateSetupDisplay() {
  if (Display.displayAnimate()) {
    Display.displayScroll(setupScrollBuffer, PA_LEFT, PA_SCROLL_LEFT, 80);
  }
}

void startConfigPortal() {
  configMode = true;

  WiFi.mode(WIFI_AP);
  String setupApSsid = getSetupApSsid();
  WiFi.softAP(setupApSsid.c_str());
  delay(500);

  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
  captivePortalActive = true;
  registerRoutes();
  server.begin();

  Serial.println("Config portal started.");
  Serial.print("AP SSID: ");
  Serial.println(setupApSsid);
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());

  initSetupDisplay();
}

// ─── Setup ───────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(9600);

  Display.begin();
  Display.setIntensity(0);
  Display.setFont(fontSubs);
  Display.setTextAlignment(PA_CENTER);

  loadPrefs();
  animationBrightnessConfigure(saved_display_brightness,
                               saved_animation_brightness_boost,
                               saved_animation_brightness_boost_amount);
  Display.setIntensity(displayBaselineIntensity());

  randomSeed(esp_random());
  if (!DISABLE_INTRO_AND_WIFI_INFO) {
    runBootAnimation();
  }
  if (RUN_SINGLE_MILESTONE_PREVIEW_ON_BOOT) {
    runMilestoneAnimation(Display, SINGLE_MILESTONE_PREVIEW_BOOT);
  }
  if (PREVIEW_MILESTONES) {
    for (MilestoneAnimation anim : MILESTONE_BOOT_PREVIEW) {
      runMilestoneAnimation(Display, anim);
    }
  }
  if (PREVIEW_HOLIDAYS) {
    runHolidayPreviewCycle(Display);
  } else if (RUN_HOLIDAY_PREVIEW_ON_BOOT) {
    runHolidayEasterEgg(Display, HOLIDAY_PREVIEW_BOOT);
  }

  bool hasCredentials =
      (saved_ssid.length() > 0 && saved_endpoint.length() > 0 &&
       (saved_stats & STAT_ALL) != 0);

  if (hasCredentials) {
    Display.print(" WiFi...");
    Serial.print("Connecting to WiFi: ");
    Serial.println(saved_ssid);

    WiFi.mode(WIFI_STA);
    WiFi.begin(saved_ssid.c_str(), saved_pass.c_str());

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED &&
           millis() - start < WIFI_TIMEOUT_MS) {
      Serial.print(".");
      delay(500);
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("");
      Serial.print("Connected! IP: ");
      Serial.println(WiFi.localIP());

      if (!DISABLE_INTRO_AND_WIFI_INFO) {
        // Scroll IP across matrix then pause on last frame for 2 seconds
        String ip = WiFi.localIP().toString();
        Display.displayScroll(ip.c_str(), PA_LEFT, PA_SCROLL_LEFT, 80);
        while (!Display.displayAnimate()) {
          delay(10);
        }
        delay(2000);
      }

      holidayEasterEggsInit();

      registerRoutes();
      server.begin();

      Display.displayClear();
      Display.print("Get Data");
      delay(250);

      client.setInsecure();
    } else {
      Serial.println("WiFi failed. Starting config portal.");
      if (!ENABLE_WOKWI_SETUP || !startWokwiConfigPortal()) {
        startConfigPortal();
      }
    }
  } else {
    Serial.println("No credentials. Starting config portal.");
    if (!ENABLE_WOKWI_SETUP || !startWokwiConfigPortal()) {
      startConfigPortal();
    }
  }
}

// ─── Loop ────────────────────────────────────────────────────────────────────
void loop() {
  if (configMode) {
    if (captivePortalActive) {
      dnsServer.processNextRequest();
    }
    server.handleClient();
    updateSetupDisplay();
    return;
  }

  server.handleClient();

  unsigned long now = millis();
  unsigned long refreshInterval = (unsigned long)saved_refresh_minutes * 60000UL;

  if (canRefreshStats() &&
      (!statsLoaded || api_lasttime == 0 ||
       now - api_lasttime >= refreshInterval)) {
    bool hadStats = statsLoaded;
    if (fetchStats()) {
      if (!hadStats) {
        startStatDisplay();
        display_lasttime = now;
        cycle_lasttime = now;
      }
    } else if (!statsLoaded) {
      Display.print("API err");
    }
    api_lasttime = now;
  }

  if (statsLoaded) {
    if (checkAndRunMilestoneAnimation()) {
      now = millis();
      if (selectedStatsCount() > 1) {
        startStatScrollIn();
      } else {
        showProjectedStat();
      }
      cycle_lasttime = now;
      display_lasttime = now;
    }

    if (checkAndRunHolidayEasterEgg(Display, saved_holiday_reminder_minutes)) {
      now = millis();
      if (selectedStatsCount() > 1) {
        startStatScrollIn();
      } else {
        showProjectedStat();
      }
      cycle_lasttime = now;
      display_lasttime = now;
    }

    if (selectedStatsCount() > 1) {
      if (tickOverflowStatScroll()) {
        // Wide values use Parola's native scroll and advance when complete.
      } else if (tickStatScrollAnimation()) {
        // Label/number scroll animation in progress.
      } else if (now - cycle_lasttime >=
                 (unsigned long)(saved_stat_cycle_seconds * 1000.0f)) {
        startStatExitTransition();
        display_lasttime = now;
      } else if (now - display_lasttime >= DISPLAY_UPDATE_MS) {
        showProjectedStat();
        display_lasttime = now;
      }
    } else if (tickOverflowStatScroll()) {
      // Wide single-stat values continuously loop their native scroll.
    } else if (now - display_lasttime >= DISPLAY_UPDATE_MS) {
      showProjectedStat();
      display_lasttime = now;
    }
  }

  tickWiFiDisconnectedIndicator();
}

String getStatDisplayText(int statIndex, const String &formattedValue) {
  if (statIndex == STAT_INDEX_SUBSCRIBERS) {
    if (formattedValue.toInt() >= 1000000L) {
      return formattedValue;
    }
    return String("*") + formattedValue;
  }
  return formattedValue;
}

String stat_format(double value, int statIndex) {
  if (statIndex == STAT_INDEX_WATCH_HOURS) {
    char buf[20];
    if (value >= 1000000.0) {
      snprintf(buf, sizeof(buf), "%.0f", value);
    } else {
      snprintf(buf, sizeof(buf), "%.1f", value);
    }
    return String(buf);
  }
  return String((long)(value + 0.5));
}
