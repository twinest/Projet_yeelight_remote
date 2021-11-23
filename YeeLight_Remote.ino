#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager

#define FLOAT_TO_INT(x) ((x)>=0?(int)((x)+0.5):(int)((x)-0.5))


#define led_rouge 5 //Pin for the notificationLed
#define led_verte_1 16
#define led_verte_2 4
#define potar 0
#define button 12 // Pin for the button
#define button_mode 13
#define button_led 14

#define nbBandes 9
int bandes[nbBandes] = {380,420,440,490,510,580,645,700,780};

WiFiManager wifiManager;


const char* H0 = "192.168.1.78"; // Ip from first light
//const char* H1 = "192.168.1.107"; //Ip from second light if you are using one light comment this line and lines 47,48,49
int port =55443; // Lights port 

int val_potar = 0;
int old_val_potar = 0;

int remote_mode = 0;

float a_brig = 0.0966796875;
int b_brig = 1;

float a_temp = 4.6875;
int b_temp = 1700;

int led_choice = 0;

void setup() {
  
  pinMode(led_rouge, OUTPUT);
  digitalWrite(led_rouge, LOW);
  pinMode(led_verte_1, OUTPUT);
  digitalWrite(led_verte_1, LOW);
  pinMode(led_verte_2, OUTPUT);
  digitalWrite(led_verte_2, LOW);
  
  pinMode(button, INPUT_PULLUP);
  pinMode(button_mode, INPUT_PULLUP);
  pinMode(button_led, INPUT_PULLUP);
  pinMode(potar, INPUT);
  
  Serial.begin(115200);
  
  wifiManager.setConfigPortalTimeout(180); 
  wifiManager.autoConnect("LightSwitch");
    
  Serial.println("Started!");
  
  digitalWrite(led_rouge, HIGH);
}

void loop() {
  
  int tt = 0;

  val_potar = analogRead(potar);
  
  while (digitalRead(button) == LOW && tt < 40) //This is function with option to change the WiFi network if you press the button for ~8 sec you will enter into congif mode.
  {                                             //If you press the button for less of 8 sec you will send toggle to led light.
      Serial.println(WiFi.status());
      Serial.println("Reading Seconds!");
      delay(100);
      tt++;
  }
  
  if(tt > 0 )
  {
    if (tt < 40)
    {
      Serial.println("ShortTime");
      set_toggle();
    }
    else
    {
      Serial.println("LongTime");
      OnDemandConfig();
    }
  } 

  if( (val_potar >= (old_val_potar + 10) ) || (val_potar <= (old_val_potar - 10) ) )
  {
    Serial.println("Check Mode");
    Serial.print( "val potar : ");
    Serial.print(val_potar);
    Serial.print( "old_val potar : ");
    Serial.print(old_val_potar);
    if( remote_mode == 0 )//brightness
    {
      int brig = a_brig*val_potar + b_brig;
      set_brig(brig);
    }

    if( remote_mode == 1 )// temperature
    {
      int temp = a_temp*val_potar + b_temp;
      set_temp(temp);
    }

    if( remote_mode == 2)// rgb
    {
      int wl = potarToWl( val_potar);
      int tab_rgb[3] ;
      wavelength2RGB(wl,tab_rgb);
      set_rgb(*tab_rgb, *(tab_rgb+1), *(tab_rgb+2));
    }

    old_val_potar = val_potar;
  }

  if( digitalRead(button_mode) == LOW )
  {
    ++remote_mode;
    if( remote_mode > 2 )
    {
      remote_mode = 0;
    }
    delay(200);
    Serial.print("Remote mode ");
    Serial.println(remote_mode);
    if( remote_mode == 0 )
    {
      digitalWrite(led_verte_1, LOW);
      digitalWrite(led_verte_2, LOW);
    }
    if( remote_mode == 1 )
    {
      digitalWrite(led_verte_1, HIGH);
      digitalWrite(led_verte_2, LOW);
    }
    if( remote_mode == 2 )
    {
      digitalWrite(led_verte_1, LOW);
      digitalWrite(led_verte_2, HIGH);
    }
    
  }
  if( digitalRead(button_led) == LOW )
  {
    ++led_choice;
    if( led_choice > 2 )
    {
      led_choice = 0;
    }
    delay(200);
    Serial.print("Led choice ");
    Serial.println(led_choice);
  }
}

//void loop()
//{
//  val_potar = analogRead(potar);
//  int wl = potarToWl( val_potar);
//  Serial.println(val_potar);
//  Serial.print("    ");
//  Serial.print(wl);
//  int tab_rgb[3] ;
//  wavelength2RGB(wl,tab_rgb);
//  
//  Serial.print("    R:");
//  Serial.print(*tab_rgb);
//  Serial.print(" G:");
//  Serial.print(*(tab_rgb+1));
//  Serial.print(" B:");
//  Serial.println(*(tab_rgb+2));
//
//  set_rgb(*tab_rgb, *(tab_rgb+1), *(tab_rgb+2));
//}

void set_rgb(int R, int G, int B)
{
  String cmd_set_rgb = "{\"id\":1,\"method\":\"set_rgb\",\"params\":[$,\"sudden\",0]}\r\n";
  int res = R*65536 + G*256 + B;
  cmd_set_rgb.replace("$", String(res));
  WiFiClient client;
  client.connect(H0,port);
  client.print(cmd_set_rgb);
  client.stop();
  delay(100);
}

void set_temp(int temp)
{
  if( temp < 1700 )
  {
    temp = 1700;
  }

  if( temp > 6500 )
  {
    temp = 6500;
  }
  
  String cmd_set_temp = "{\"id\":1,\"method\":\"set_ct_abx\",\"params\":[$,\"sudden\",0]}\r\n";
  cmd_set_temp.replace("$", String(temp));
  WiFiClient client;
  client.connect(H0,port);
  client.print(cmd_set_temp);
  client.stop();
  delay(100);
}

void set_brig(int brig)
{
  if( brig < 1 )
  {
    brig = 1;
  }

  if( brig > 100 )
  {
    brig = 100;
  }
  
  String cmd_set_brig = "{\"id\":1,\"method\":\"set_bright\",\"params\":[$,\"sudden\",0]}\r\n";
  cmd_set_brig.replace("$", String(brig));
  WiFiClient client;
  client.connect(H0,port);
  client.print(cmd_set_brig);
  client.stop();
  delay(100);
}

void set_toggle()
{
  String cmd_toggle ="{\"id\":1,\"method\":\"toggle\",\"params\":[]}\r\n";
  WiFiClient client;
  client.connect(H0,port);
  client.print(cmd_toggle);
  client.stop();
  delay(100);
}

void OnDemandConfig(){
      wifiManager.setConfigPortalTimeout(180); //Startuva WiFi vo AP i moze da se prekonfigurira WIFI-ot i novite informacii ke se zacuvaat vo EEPROM.
      wifiManager.startConfigPortal("LightSwitch"); // Se pushta so ime SmartPost 
}

void wavelength2RGB(float wl, int* tab)
{
  float r = 0.0;
  float g = 0.0;
  float b = 0.0;
  float s = 1.0;
  
  /*interpolation linéaire entre différente plages*/
  
  if( wl < 380)
  {
    /* invisible below 380 */
    // The code is a little redundant for clarity.
    // A smart optimiser can remove any r=0, g=0, b=0.
    r = 0;
    g = 0;
    b = 0;
    s = 0;
    }

  if( (wl >= 380) && (wl < 420) )
  {
    /* 380 .. 420, intensity drop off. */
    r = (440.0-wl)/(440.0-380.0);
    g = 0.0;
    b = 1.0;
    s = 0.3 + 0.7*(wl-380.0)/(420.0-380.0);
    }

  if( (wl >= 420) && (wl < 440) )
  {
    /* 420 .. 440 */
    r = (440.0-wl)/(440.0-380.0);
    g = 0.0;
    b = 1.0;
  }

  if( (wl >= 440) && (wl < 490) )
  {
    /* 440 .. 490 */
    r = 0.0;
    g = (wl-440.0)/(490.0-440.0);
    b = 1.0;
  }

  if( (wl >= 490) && (wl < 510) )
  {
    /* 490 .. 510 */
    r = 0.0;
    g = 1.0;
    b = (510.0-wl)/(510.0-490.0);
  }

 if( (wl >= 510) && (wl < 580) )
  {
    /* 510 .. 580 */
    r = (wl-510.0)/(580.0-510.0);
    g = 1.0;
    b = 0.0;
  }

 if( (wl >= 580) && (wl < 645) )
  {
    /* 580 .. 645 */
    r = 1.0;
    g = (645.0-wl)/(645.0-580.0);
    b = 0.0;
  }

 if( (wl >= 645) && (wl < 700) )
  {
    /* 645 .. 700 */
    r = 1.0;
    g = 0.0;
    b = 0.0;
  }

 if( (wl >= 700) && (wl <= 780) )
  {
    /* 700 .. 780, intensity drop off */
    r = 1.0;
    g = 0.0;
    b = 0.0;
    s = 0.3 + 0.7*(781.0-wl)/(781.0-700.0);
  }
  

 if(wl > 780)
 {
    /* invisible above 780 */
    r = 0;
    g = 0;
    b = 0;
    s = 0;
 }

/*Correction d'intensité*/
  r *= s;
  g *= s;
  b *= s;
      
  r *= 255;
  g *= 255;
  b *= 255;  
//  int tab[3] ;
  tab[0] = FLOAT_TO_INT(r);
  tab[1] = FLOAT_TO_INT(g);
  tab[2] = FLOAT_TO_INT(b);
  Serial.print("    R_f:");
  Serial.print(tab[0]);
  Serial.print(" G_f:");
  Serial.print(tab[1]);
  Serial.print(" B_f:");
  Serial.println(tab[2]);
//  return tab;

}
int potarToWl( int vpotar)
{
  float a = 0.390625;
  int b = 380;

  return a*vpotar + b;
}
