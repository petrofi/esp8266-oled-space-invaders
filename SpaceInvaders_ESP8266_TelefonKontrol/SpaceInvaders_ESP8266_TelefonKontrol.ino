/*
  SPACE INVADERS | ESP8266 + 1.3-inch SH1106 OLED | Phone gamepad
  Parts: ESP8266 NodeMCU / Wemos D1 mini, 4-pin 128x64 SH1106 OLED only.
  Wiring: OLED VCC->3V3, GND->GND, SDA->D2(GPIO4), SCL->D1(GPIO5).
  WiFi: ESP8266-SpaceInvaders / space1234
  Browser: http://192.168.4.1
  Arduino libraries: Adafruit SH110X, Adafruit GFX Library, Adafruit BusIO.
  Core's bundled libraries: Wire, ESP8266WiFi, ESP8266WebServer.
*/

#include <Arduino.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

constexpr uint8_t OLED_SDA_PIN = 4; // D2 on NodeMCU / Wemos D1 mini
constexpr uint8_t OLED_SCL_PIN = 5; // D1 on NodeMCU / Wemos D1 mini
constexpr uint8_t OLED_I2C_ADDRESS = 0x3C; // If needed, try 0x3D
constexpr int W = 128;
constexpr int H = 64;
constexpr int PLAYER_Y = 55;
constexpr int PLAYER_WIDTH = 11;
constexpr int ROWS = 3;
constexpr int COLS = 6;
constexpr int ALIEN_W = 10;
constexpr int ALIEN_H = 7;
constexpr int DX = 17;
constexpr int DY = 10;
constexpr int FLEET_WIDTH = (COLS - 1) * DX + ALIEN_W;

const char* AP_SSID = "ESP8266-SpaceInvaders";
const char* AP_PASSWORD = "space1234"; // >=8 chars for WPA2

Adafruit_SH1106G oled(W, H, &Wire, -1);
ESP8266WebServer server(80);

const char HTML_PAGE[] PROGMEM = R"WEBPAGE(
<!doctype html><html lang="tr"><head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no">
<meta name="theme-color" content="#101b2b"><title>ESP8266 Space Invaders</title>
<style>
*{box-sizing:border-box}html,body{margin:0;min-height:100%;background:#101b2b;color:#f0f7ff;font:16px system-ui,-apple-system,Arial,sans-serif}
main{width:min(520px,100%);margin:auto;padding:22px 17px 32px;text-align:center}
h1{font-size:25px;letter-spacing:2px;margin:4px 0 3px}p{color:#a9bdd0;margin:5px 0 15px}
.panel{padding:18px 15px;border:1px solid #30445a;border-radius:18px;background:#19293b}
.status{display:flex;justify-content:space-between;gap:5px;font-size:13px;color:#b9d0e6;margin-bottom:16px;flex-wrap:wrap}
.status b{color:#fff}.screen{margin:0 auto 22px;padding:13px 5px;border:3px solid #52627a;border-radius:10px;background:#0a131e;max-width:340px}
.screen .aliens{color:#91f6ba;letter-spacing:8px;font:22px monospace;margin:12px auto}
.screen .ship{font:28px monospace;color:white;margin:10px auto 0}.screen .caption{font:12px monospace;color:#b1c2d6}
.pad{display:grid;grid-template-columns:1fr 1fr 1fr;gap:10px}
.key{color:#fff;font-size:17px;font-weight:800;border:none;border-radius:15px;min-height:105px;touch-action:none;user-select:none;-webkit-user-select:none;-webkit-tap-highlight-color:transparent;background:#2b4667;box-shadow:inset 0 -5px #1b314c;cursor:pointer}
.key.fire{background:#b74242;box-shadow:inset 0 -5px #792a2a}.key.active{transform:translateY(3px);filter:brightness(1.45);box-shadow:inset 0 -2px #182838}
.start{width:100%;background:#48af83;color:#092618;border:0;border-radius:12px;font-size:17px;font-weight:800;margin:16px 0 8px;min-height:48px}
.small{font-size:12px;color:#8fa9c3;line-height:1.5;margin-top:12px}
</style></head><body><main>
<h1>SPACE INVADERS</h1><p>ESP8266 &bull; Telefon Gamepad</p>
<section class="panel">
<div class="status"><span>Wi-Fi: <b id="wifi">Baglandi</b></span><span>SKOR: <b id="score">0</b></span><span>CAN: <b id="lives">3</b></span><span>DALGA: <b id="wave">1</b></span></div>
<div class="screen"><div class="caption">OYUN ESP8266 OLED EKRANINDA</div><div class="aliens">▟ ▟ ▟ ▟</div><div class="ship">▲</div></div>
<div class="pad">
<button class="key" data-key="l" aria-label="Sol">&#9664;<br>SOL</button>
<button class="key" data-key="r" aria-label="Sag">&#9654;<br>SAG</button>
<button class="key fire" data-key="f" aria-label="Ates">&#9678;<br>ATES</button>
</div>
<button class="start" id="start">OYUNU BASLAT / YENIDEN BASLAT</button>
<div id="state" class="small">Telefondaki dugmeleri basili tut. Oyun OLED'de gorunur.</div>
</section><p class="small">Internet gerekmez. Sayfa acilmazsa mobil veriyi gecici kapat ve http://192.168.4.1 adresine git.</p>
</main><script>
const hold={l:0,r:0,f:0};
let busy=false;
function sendControls(){
 const params=new URLSearchParams(hold);
 fetch('/input?'+params.toString(),{cache:'no-store'}).catch(()=>{});
}
document.querySelectorAll('[data-key]').forEach(btn=>{
 const key=btn.dataset.key;
 function change(val){if(hold[key]!==val){hold[key]=val;btn.classList.toggle('active',!!val);sendControls();}}
 btn.addEventListener('pointerdown',e=>{e.preventDefault();btn.setPointerCapture(e.pointerId);change(1);});
 btn.addEventListener('pointerup',e=>{e.preventDefault();change(0);});
 btn.addEventListener('pointercancel',()=>change(0));
 btn.addEventListener('lostpointercapture',()=>change(0));
 btn.addEventListener('contextmenu',e=>e.preventDefault());
});
function releaseAll(){Object.keys(hold).forEach(k=>hold[k]=0);document.querySelectorAll('.key').forEach(b=>b.classList.remove('active'));sendControls();}
window.addEventListener('blur',releaseAll);
document.addEventListener('visibilitychange',()=>{if(document.hidden)releaseAll();});
setInterval(()=>{if(hold.l||hold.r||hold.f)sendControls();},150);
document.getElementById('start').addEventListener('click',()=>fetch('/start',{cache:'no-store'}).catch(()=>{}));
const labels=['BASLAMAK ICIN BASLAT','OYUN DEVAM EDIYOR','YENI DALGA','OYUN BITTI - TEKRAR BASLAT'];
async function status(){
 if(busy)return;busy=true;
 try{const r=await fetch('/status',{cache:'no-store'});const s=await r.json();
 ['score','lives','wave'].forEach(k=>document.getElementById(k).textContent=s[k]);
 document.getElementById('state').textContent=labels[s.mode]||'';
 document.getElementById('wifi').textContent='Bagli';
 }catch(e){document.getElementById('wifi').textContent='Baglanti yok';}
 busy=false;
}
setInterval(status,900);status();
</script></body></html>
)WEBPAGE";

enum Mode : uint8_t { START_SCREEN=0, PLAYING=1, WAVE_SCREEN=2, GAME_OVER=3 };
Mode mode = START_SCREEN;

struct Bullet { bool active; int16_t x; int16_t y; };
Bullet shot = {false,0,0};
Bullet enemyShot = {false,0,0};
bool alive[ROWS][COLS];

bool leftPressed=false, rightPressed=false, firePressed=false;
uint32_t lastInputMs=0;
int16_t playerX=(W-PLAYER_WIDTH)/2;
int16_t fleetX=16, fleetY=15;
int8_t fleetDir=1;
uint8_t aliveCount=ROWS*COLS, lives=3, wave=1;
uint32_t score=0, highScore=0; // High score survives until power-off
uint32_t lastPlayerMove=0, lastFleetMove=0, lastShotMove=0, lastEnemyShotMove=0;
uint32_t lastFireMs=0, nextEnemyFireMs=0, lastRenderMs=0;
uint32_t waveScreenMs=0, lastHitMs=0;

bool hitbox(int16_t ax,int16_t ay,int16_t aw,int16_t ah,
            int16_t bx,int16_t by,int16_t bw,int16_t bh){
  return ax<bx+bw && ax+aw>bx && ay<by+bh && ay+ah>by;
}

void centerText(const char* s,int16_t y,uint8_t size=1){
  oled.setTextSize(size);
  const int16_t px=(int16_t)strlen(s)*6*size;
  oled.setCursor((W-px)/2,y);
  oled.print(s);
}

void waveInit(uint32_t now){
  for(int r=0;r<ROWS;r++)for(int c=0;c<COLS;c++)alive[r][c]=true;
  aliveCount=ROWS*COLS;
  fleetX=16;fleetY=15;fleetDir=1;
  playerX=(W-PLAYER_WIDTH)/2;
  shot.active=false;enemyShot.active=false;
  lastPlayerMove=lastFleetMove=lastShotMove=lastEnemyShotMove=now;
  lastFireMs=now-400UL;
  nextEnemyFireMs=now+1100;
  lastHitMs=now-1500UL;
  mode=PLAYING;
}

void newGame(uint32_t now){
  wave=1;lives=3;score=0;
  waveInit(now);
}

void endGame(){
  mode=GAME_OVER;
  if(score>highScore)highScore=score;
  shot.active=false;
  enemyShot.active=false;
}

void renderAlien(int16_t x,int16_t y,int row){
  oled.fillRect(x+2,y+1,6,3,SH110X_WHITE);
  oled.drawPixel(x+1,y,SH110X_WHITE);
  oled.drawPixel(x+8,y,SH110X_WHITE);
  oled.drawPixel(x,y+3,SH110X_WHITE);
  oled.drawPixel(x+9,y+3,SH110X_WHITE);
  oled.drawPixel(x+2,y+5,SH110X_WHITE);
  oled.drawPixel(x+7,y+5,SH110X_WHITE);
  if(row==1){oled.drawPixel(x+1,y+6,SH110X_WHITE);oled.drawPixel(x+8,y+6,SH110X_WHITE);}
  else{oled.drawPixel(x+3,y+6,SH110X_WHITE);oled.drawPixel(x+6,y+6,SH110X_WHITE);}
  oled.drawPixel(x+3,y+2,SH110X_BLACK);
  oled.drawPixel(x+6,y+2,SH110X_BLACK);
}

void render(){
  oled.clearDisplay();oled.setTextColor(SH110X_WHITE);
  if(mode==START_SCREEN){
    centerText("SPACE",3,2);centerText("INVADERS",24,2);
    centerText("TELEFONDAN OYNA",46);
    centerText("192.168.4.1",56);
  }else if(mode==GAME_OVER){
    centerText("GAME OVER",4,2);
    oled.setTextSize(1);oled.setCursor(14,32);oled.print("SKOR: ");oled.print(score);
    oled.setCursor(14,43);oled.print("REKOR:");oled.print(highScore);
    centerText("TELEFONDAN BASLAT",56);
  }else if(mode==WAVE_SCREEN){
    centerText("DALGA TAMAM!",13);
    oled.setTextSize(2);oled.setCursor(52,28);oled.print(wave);
    centerText("SIRADAKI DALGA...",52);
  }else{
    oled.setTextSize(1);oled.setCursor(0,0);oled.print("S:");oled.print(score);
    oled.setCursor(63,0);oled.print("C:");oled.print(lives);
    oled.setCursor(92,0);oled.print("D:");oled.print(wave);
    oled.drawFastHLine(0,10,W,SH110X_WHITE);
    for(int r=0;r<ROWS;r++)for(int c=0;c<COLS;c++){
      if(alive[r][c])renderAlien(fleetX+c*DX,fleetY+r*DY,r);
    }
    if(millis()-lastHitMs>=1100 || ((millis()/120)%2)){
      oled.fillTriangle(playerX+5,PLAYER_Y-3,playerX,PLAYER_Y+7,playerX+10,PLAYER_Y+7,SH110X_WHITE);
      oled.fillRect(playerX+3,PLAYER_Y+2,5,5,SH110X_WHITE);
    }
    if(shot.active)oled.fillRect(shot.x,shot.y,2,4,SH110X_WHITE);
    if(enemyShot.active)oled.fillRect(enemyShot.x,enemyShot.y,2,4,SH110X_WHITE);
  }
  oled.display();
}

void updateGame(uint32_t now){
  if(now-lastPlayerMove>=35){
    lastPlayerMove=now;
    if(leftPressed&&!rightPressed)playerX-=3;
    if(rightPressed&&!leftPressed)playerX+=3;
    if(playerX<0)playerX=0;
    if(playerX>W-PLAYER_WIDTH)playerX=W-PLAYER_WIDTH;
  }
  if(firePressed&&!shot.active && now-lastFireMs>=220){
    shot.active=true;shot.x=playerX+5;shot.y=PLAYER_Y-6;lastFireMs=now;
  }
  if(now-lastShotMove>=25){
    lastShotMove=now;
    if(shot.active){
      shot.y-=3;
      if(shot.y<11){shot.active=false;}
      else{
        bool found=false;
        for(int r=ROWS-1;r>=0&&!found;r--){
          for(int c=0;c<COLS;c++){
            if(!alive[r][c])continue;
            int16_t ex=fleetX+c*DX, ey=fleetY+r*DY;
            if(hitbox(shot.x,shot.y,2,4,ex,ey,ALIEN_W,ALIEN_H)){
              alive[r][c]=false;aliveCount--;shot.active=false;
              score+=10+wave*5;found=true;break;
            }
          }
        }
        if(aliveCount==0){mode=WAVE_SCREEN;waveScreenMs=now;return;}
      }
    }
  }
  const uint32_t fleetInterval=(wave>8)?60:(165-(wave-1)*13);
  if(now-lastFleetMove>=fleetInterval){
    lastFleetMove=now;
    fleetX+=2*fleetDir;
    if(fleetX<=1){fleetX=1;fleetDir=1;fleetY+=3;}
    if(fleetX+FLEET_WIDTH>=W-1){fleetX=W-1-FLEET_WIDTH;fleetDir=-1;fleetY+=3;}
    // Any surviving alien reaching the player zone ends the game.
    for(int r=0;r<ROWS;r++)for(int c=0;c<COLS;c++){
      if(alive[r][c] && fleetY+r*DY+ALIEN_H>=PLAYER_Y){endGame();return;}
    }
  }
  if((int32_t)(now-nextEnemyFireMs)>=0 && !enemyShot.active && aliveCount>0){
    int pick=random(aliveCount);
    bool chosen=false;
    for(int c=0;c<COLS&&!chosen;c++)for(int r=ROWS-1;r>=0;r--){
      if(!alive[r][c])continue;
      if(pick--==0){
        enemyShot={true,(int16_t)(fleetX+c*DX+4),(int16_t)(fleetY+r*DY+ALIEN_H)};
        chosen=true;break;
      }
    }
    nextEnemyFireMs=now+(uint32_t)random(750,1400);
  }
  if(now-lastEnemyShotMove>=45){
    lastEnemyShotMove=now;
    if(enemyShot.active){
      enemyShot.y+=2+(wave>4?1:0);
      if(enemyShot.y>=H){enemyShot.active=false;}
      else if(hitbox(enemyShot.x,enemyShot.y,2,4,playerX,PLAYER_Y-3,PLAYER_WIDTH,11)){
        enemyShot.active=false;
        if(now-lastHitMs>=1100){
          lastHitMs=now;
          if(lives>0)lives--;
          if(lives==0){endGame();return;}
        }
      }
    }
  }
}

void setupWeb(){
  server.on("/",HTTP_GET,[]{
    server.sendHeader("Cache-Control","no-store");
    server.send_P(200,PSTR("text/html; charset=utf-8"),HTML_PAGE);
  });
  server.on("/input",HTTP_GET,[]{
    leftPressed=server.arg("l")=="1";
    rightPressed=server.arg("r")=="1";
    firePressed=server.arg("f")=="1";
    lastInputMs=millis();
    server.send(204,"text/plain","");
  });
  server.on("/start",HTTP_GET,[]{
    leftPressed=rightPressed=firePressed=false;
    lastInputMs=millis();
    newGame(millis());
    server.send(200,"text/plain","OK");
  });
  server.on("/status",HTTP_GET,[]{
    char json[130];
    snprintf(json,sizeof(json),"{\"score\":%lu,\"lives\":%u,\"wave\":%u,\"mode\":%u}",
            (unsigned long)score,(unsigned int)lives,(unsigned int)wave,(unsigned int)mode);
    server.sendHeader("Cache-Control","no-store");
    server.send(200,"application/json",json);
  });
  server.onNotFound([](){server.send(404,"text/plain","Not found");});
  server.begin();
}

void setup(){
  Serial.begin(115200);
  Wire.begin(OLED_SDA_PIN,OLED_SCL_PIN);
  Wire.setClock(400000);
  delay(150);
  if(!oled.begin(OLED_I2C_ADDRESS,true)){
    Serial.println("SH1106 baslatilamadi. Kablo ve I2C adresini kontrol edin.");
    while(true)delay(1000);
  }
  oled.setTextWrap(false);
  oled.setTextColor(SH110X_WHITE);
  oled.clearDisplay();
  centerText("WIFI ACILIYOR...",28);
  oled.display();

  WiFi.persistent(false);
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(IPAddress(192,168,4,1),IPAddress(192,168,4,1),IPAddress(255,255,255,0));
  bool ok=WiFi.softAP(AP_SSID,AP_PASSWORD);
  Serial.println(ok?"WiFi AP hazir":"WiFi AP baslatilamadi");
  Serial.print("IP: ");Serial.println(WiFi.softAPIP());
  setupWeb();
  randomSeed(micros());
  lastRenderMs=millis()-55;
}

void loop(){
  server.handleClient();
  uint32_t now=millis();
  if(now-lastInputMs>600){leftPressed=rightPressed=firePressed=false;}
  if(mode==PLAYING)updateGame(now);
  else if(mode==WAVE_SCREEN && now-waveScreenMs>=1300){
    if(wave<99)wave++;
    waveInit(now);
  }
  if(now-lastRenderMs>=50){lastRenderMs=now;render();}
  yield();
}
