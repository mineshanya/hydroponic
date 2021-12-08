/*#################################  ЛОГИКА РАБОТЫ КОНТРОЛЛЕРА  #########################################################################################################*/
//  Изначально, с завода, в постоянной памяти(далее EEPROM) записаны ячейки byte со своим максимальным значением (255).
//  При каждом включении идет обращение к EEPROM.
// Если при включение удается считать расписание из EEPROM,
// на дисплее отображается "Config loaded" и использоваться будет сохраненное ранее расписание.
// Если же это первое включение, на дисплее отображается "Defaults loaded" и использоваться будет стандартное расписание, которое можно настроить ниже.
//  После загрузки запускается цикл:
//      происходит актуализация состояния, в этот момент загорается индицирующий светодиод
//      по завершению обработки индицирующий светодиод гаснет и ардуино уходит в спячку на ~5 секунд
//  Если при подаче питания удерживать кнопку энкодера, произойдет сброс текущего времени на 12:00
// Если до подачи питания на модуле часов батарейка была разряжена/вынута, также произойдет сброс текущего времени на 12:00
// При сбросе времени индицирующий светодиод быстро мигает 5 раз, а на дисплее отображается "RTC RESET!" или "RTC lstpwr" соответственно
//  Чтобы попасть в меню, нужно в любой момент нажать и удерживать кнопку энкодера. Чтобы изменять значения, нужно крутить нажатый энкодер.
//  Логика подменю и меню:
//  В подменю изменения расписания полива, освещения и яркости пункт "Ok" отвечает за установку значений до следующей перезагрузки
// В подменю "Time" пункт "Save" задаст установленное время
// Пункт "Cancel" отменяет введение новых значений
// Чтобы новое расписание сохранилось и после перезагрузки, нужно в главном меню нажать "Save"
// Пункт "Load" загружает из EEPROM расписание
// Пункт "Reset" возвращает стандартное расписание, которое можно настроить ниже, а также записывает его в EEPROM

/*#################################  ПОДКЛЮЧЕНИЕ ПЕРИФЕРИИ, ПИТАНИЕ, ОБЩАЯ СХЕМА  #######################################################################################*/
// SDA - A4
// SCL, он же SCK - A5
// питать можно 3.3-5v напрямую через контакт VCC/5V, 7-15v через контакт VIN, 7-40v через пониж модуль LM2596S (его настроить на 5v и подать на контакты VCC/5V)
// потребляет 21-28мА, в зависимости от состояния
// 5539 фоторезистор соединить напрямую с землей, подтягивающий резистор не нужен, можно любой, но тогда сумерки будут не в середине диапозона и точность будет ниже
// GND общий во всей системе, в т.ч. у приборов и у LM2596S
// Gate на мосфетах подтянуть 10кОм к земле, 220 Ом к ардуино (первая ножка)
// VCC для устройств подкл к "DRAIN" на мосфетах (серединка или верхний контакт)
// Устройства могут питаться любым напряжением, в т.ч. могут работать от 1 источника, тогда "DRAIN" на мосфетах можно сразу соединить
// Устройства подключать к Source на мосфетах (3я ножка), это будет их плюсовой контакт питания
// Можно поставить светодиоды для визуального контроля как вместе с мосфетами, так и вместо них
//    Светодиоды подключаются между GND и ардуино через резистор 220ом-3кОм(больше сопротивление=тусклее)

/*#################################  ДЕБАГ  ##############################################################################################################################*/
// 5 быстрых миганий - время было сброшено, стало 1/1/2020 12:00
// постоянные быстрые мигания - нет связи с часами

/*#################################  НАСТРОЙКА ПИНОВ, К КОТОРЫМ ПОДКЛЮЧЕНА ПЕРИФЕРИЯ  ####################################################################################*/
#define ENC_CLK_S1    4      // CLK/S1 - влияет на направление вращения,                            допустимые пины 2-13, A0-A3, A6-A7
#define ENC_DT_S2     5      // DT/S2 -  влияет на направление вращения,                            допустимые пины 2-13, A0-A3, A6-A7
#define ENK_key_SW    3      // key/SW - кнопка, замыкает GND,                                      допустимые пины 2-13, A0-A3, A6-A7

#define WTR_PIN       11     // pin реле увлажнения, обычно A0,                                     допустимые пины 2-13, A0-A3, A6-A7
#define LGHT_PIN      9      // pin реле освещения, обычно A1,                                      допустимые пины 2-13, A0-A3, A6-A7
#define BR_SENS_PIN   A3     // pin датчик освещенности, обычно A3,                                 допустимые пины A0-A3, A6-A7
#define IND_LED_PIN   13     // pin светодиода-индикатора, обычно 13(распаян на плате)              допустимые пины 2-13, A0-A3, A6-A7

#define OLED_EN_PIN   2      // pin включения дисплея, если нужно задействовать, замыкаем на землю, допустимые пины 2-13, A0-A3, A6-A7

/*#################################  РАСПИСАНИЕ ПО-УМОЛЧАНИЮ  ############################################################################################################*/
#define OLED_TIMEOUT  10     // время в секундах до автовыключения дисплея, 1-240 сек, 0 горит всегда
#define WTR_ON        0      // время вкл полива (минуты), 0-59
#define WTR_OFF       5      // время выкл полива (минуты), 0-59
#define LGHT_ON       8      // время вкл света (часы), 0-23
#define LGHT_OFF      21     // время выкл света (часы), 0-23
#define BRTNSS_ON     40     // нижний порог % освещенности для вкл света, обычно 52, 0-100
#define BRTNSS_OFF    60     // верхний порог % освещенности для выкл света, обычно 52, 0-100

/*########################################################################################################################################################################*/
#define sketchversion F("v3.4.2")
#define ABOUTTEXT     "\n\rHydroponic\n\rv3.4.2\n\rby mineshanya"
#define SerialSpeed   57600
#define separator     F("----------------------------------")
#include <EncButton.h>
#include <microDS3231.h>
#include <GyverOLED.h>
#include <EEManager.h>

struct paramptr {
  paramptr(){};
  //paramptr(const String& _name, byte& _val, byte& _min, byte& _max){name = _name; val = &_val; min = &_min; max = &_max;};
  //paramptr(const String& _name, byte& _val, const byte& _min, byte& _max){name = _name; val = &_val; min = &_min; max = &_max;};
  //paramptr(const String& _name, byte& _val, byte& _min, const byte& _max){name = _name; val = &_val; min = &_min; max = &_max;};
  paramptr(const char* _name, byte& _val, const byte& _min, const byte& _max){name = _name; val = &_val; min = &_min; max = &_max;};
  const char* name;
  byte *val;
  byte *min;
  byte *max;
};

struct hydroponic {
    byte w_pin;
    byte w_on;
    byte w_off;
    byte l_pin;
    byte l_on;
    byte l_off;
    byte br_lvl_on;
    byte br_lvl_off;
    byte brightness;
    bool wtr_pin_state;
    bool lght_pin_state;
    hydroponic(){
      w_pin = WTR_PIN;
      w_on  = WTR_ON;
      w_off = WTR_OFF;
      l_pin = LGHT_PIN;
      l_on  = LGHT_ON;
      l_off = LGHT_OFF;
      br_lvl_on  = BRTNSS_ON;
      br_lvl_off = BRTNSS_OFF;
    };
};
struct hydromodule {
  bool useOLED;
  byte OLED_en_pin = OLED_EN_PIN;
  byte oled_timeout = OLED_TIMEOUT;
  byte ind_led_pin = IND_LED_PIN;
  byte br_sens_pin = BR_SENS_PIN;
  hydroponic pot1;
};
hydromodule grower1;

EEManager memory(grower1);
MicroDS3231 RTC;
DateTime now;
GyverOLED<SSD1306_128x32, OLED_BUFFER> OLED(0x3C);
EncButton<EB_TICK, ENC_CLK_S1, ENC_DT_S2, ENK_key_SW> enc;

uint32_t myTimer1 = 0;
uint32_t myTimer2 = 5*1000;

#define defaultHours    12
#define defaultMinutes  0
const byte justzero        = 0;
const byte secondmax       = 59;
const byte minutemax       = 59;
const byte hourmax         = 23;
const byte percentmax      = 100;
const byte digpinmax       = 19;
const byte analogpinmin    = 14;
const byte analogpinmax    = 21;
const byte oledtimeoutmax  = 240;

const char sl_0[] PROGMEM = "";
const char sl_1[] PROGMEM = "Watering";
const char sl_2[] PROGMEM = "Lighting";
const char sl_3[] PROGMEM = "Brightness";
const char sl_4[] PROGMEM = "Time";
const char sl_5[] PROGMEM = "All pins";
const char sl_6[] PROGMEM = "Display timeout";
const char sl_7[] PROGMEM = "About";
const char *const settingsList[] PROGMEM ={sl_0, sl_1, sl_2, sl_3, sl_4, sl_5, sl_6, sl_7};

const char shl_0[] PROGMEM = "0";
const char shl_1[] PROGMEM = " (mins, hrly)";
const char shl_2[] PROGMEM = " (hrs, daily)";
const char shl_3[] PROGMEM = " (%,hyst)";
const char shl_4[] PROGMEM = "";
const char shl_5[] PROGMEM = " (A0->14 etc)";
const char shl_6[] PROGMEM = "\n\r\n\r\n\r0 = always";
const char shl_7[] PROGMEM = ABOUTTEXT;
const char *const settingsHintList[] PROGMEM ={shl_0, shl_1, shl_2, shl_3, shl_4, shl_5, shl_6, shl_7};
const byte settingsSize = sizeof(settingsList)/sizeof(settingsList[0]);

const char sal_0[] PROGMEM = "Save";
const char sal_1[] PROGMEM = "Load";
const char sal_2[] PROGMEM = "Reset";
const char sal_3[] PROGMEM = "Close";
const char *const settingsActionList[] PROGMEM = {sal_0, sal_1, sal_2, sal_3};
const byte settingsActionListSize=sizeof(settingsActionList)/sizeof(settingsActionList[0]);

const char mval_0[] PROGMEM = "OK";
const char mval_1[] PROGMEM = "Cancel";
const char* const menuValActionList[] PROGMEM ={mval_0,mval_1};
const byte menuValActionListSize=sizeof(menuValActionList)/sizeof(menuValActionList[0]);


// мигание индицирующим светодиодом i раз
void blink (byte i = 1){
  for (;i>0;i--) {
    digitalWrite(grower1.ind_led_pin, HIGH);
    delay(70);
    digitalWrite(grower1.ind_led_pin, LOW);
    delay(250);
  }
}

// отображение message на дисплее текстом размера 1 или 2 и продолжительностью ms/1000 секунд
void showMessage(const byte& text_size, const __FlashStringHelper *message,int ms=3500){
  Serial.println(message);
  Serial.println(separator);
  if(grower1.useOLED){
    OLED.clear();
    OLED.setCursor(0, 0);
    OLED.setScale(text_size);
    OLED.print(message);
    OLED.update();
    delay(ms);
  }
}

// отображение message на дисплее текстом размера 1 или 2 и продолжительностью ms/1000 секунд
void showMessage(const byte& text_size, const String& message,int ms=3500){
  Serial.println(message);
  Serial.println(separator);
  if(grower1.useOLED){
    OLED.clear();
    OLED.setCursor(0, 0);
    OLED.setScale(text_size);
    OLED.print(message);
    OLED.update();
    delay(ms);
  }
}

// сброс времени на 1 января 2020 12:00
void rstTime(const String& reason){
  RTC.setTime(0,defaultMinutes,defaultHours,1,12,2021);
  showMessage(2,(reason+F("\n\rnow ")+RTC.getTimeString().substring(0,5)),2000);
  blink(5);
}

// сохранение параметров расписания в постоянную память
void saveToEEPROM (hydromodule* grower) {
  memory.updateNow();
  showMessage(2,F("Saved to\n\rEEPROM"),2000);
}

// загрузка параметров расписания из постоянной памяти
void loadFromEEPROM (hydromodule* grower) {
  byte stat = memory.begin(1,'x');
  if (stat==1) {showMessage(2,F("Loaded\n\rEEPROM"),2000); }
  else if (stat==2) {showMessage(2,F("EEPROM\n\ris empty"),2000); restoreDefaults(grower); }
  else showMessage(2,F("EEPROM\n\r!ERROR!"), 3500);
}

// взять стандартные значения из скетча
void restoreDefaults (hydromodule* grower) {
  hydromodule newhydromodule;
  *grower=newhydromodule;
  showMessage(2,F("Loaded\n\rdefaults"),2000);
  saveToEEPROM(grower);
}

// обновление текущего времени
void updTime (DateTime* now) {
  *now = RTC.getTime();
}

// проверка, какие ножки нужно включить, какие выключить
void updState (hydromodule& grower, DateTime& now){
  grower.pot1.wtr_pin_state = now.minute>=grower.pot1.w_on && now.minute<grower.pot1.w_off;       // полив вкл, если минут больше w_on и меньше w_off
  grower.pot1.brightness=map(analogRead(grower.br_sens_pin),1019,10,0,100);
  if (now.hour>=grower.pot1.l_on && now.hour<grower.pot1.l_off) {                                 // свет вкл, если попадает на заданный промежуток и внешний свет ниже порогового
    if (grower.pot1.brightness < grower.pot1.br_lvl_on) grower.pot1.lght_pin_state = true;
    else if (grower.pot1.brightness > grower.pot1.br_lvl_off) grower.pot1.lght_pin_state = false;
  } else grower.pot1.lght_pin_state = false;
  digitalWrite(grower.pot1.l_pin, grower.pot1.lght_pin_state);                                    // задаем нужное состояние освещения
  digitalWrite(grower.pot1.w_pin, grower.pot1.wtr_pin_state);                                     // задаем нужное состояние полива
}

// отправка времени, состояния и расписания в COM-порт
void sendToSerial (hydroponic& pot, DateTime& now) {
  const __FlashStringHelper* F_on = F("on");
  const __FlashStringHelper* F_off = F("off");
  Serial.println(String(now.hour)+F("h:")+now.minute+F("m:")+now.second+'s');
  Serial.println(String(F("wtr="))+(pot.wtr_pin_state?F_on:F_off)+F(" (")+pot.w_on+F_on+F(",")+pot.w_off+F_off+')');
  Serial.println(String(F("lght="))+(pot.lght_pin_state?F_on:F_off)+F(" (")+pot.l_on+F_on+F(",")+pot.l_off+F_off+')');
  Serial.println(String(F("brightness="))+pot.brightness+F("% <")+pot.br_lvl_on+F_on+F(",>")+pot.br_lvl_off+F_off);
  Serial.println(separator);
}

// отправка отладочной инфы на дисплей
void sendToDisp (hydroponic &pot, DateTime &now) {
  OLED.clear();
  OLED.setScale(1);
  OLED.setCursor(0, 0);
  const __FlashStringHelper *F_on = F("on");
  const __FlashStringHelper *F_off = F("off");
  OLED.println(String(now.hour)+F("h:")+now.minute+F("m:")+now.second+'s');
  //OLED.print(now.hour); OLED.print(F("h:")); OLED.print(now.minute); OLED.print(F("m:")); OLED.print(now.second); OLED.println('s');
  OLED.println(String(F("wtr="))+(pot.wtr_pin_state?F_on:F_off)+F(" (")+pot.w_on+F_on+F(",")+pot.w_off+F_off+')');
  OLED.println(String(F("lght="))+(pot.lght_pin_state?F_on:F_off)+F(" (")+pot.l_on+F_on+F(",")+pot.l_off+F_off+')');
  OLED.println(String(F("br="))+pot.brightness+F("% <")+pot.br_lvl_on+F_on+F(",>")+pot.br_lvl_off+F_off);
  OLED.update();
}

// прокручиваем значение val по кругу между min и max
void spinVal (byte& val, const byte& min, const byte& max, const int8_t& direction) {
  if (direction>0)
    if (val<max) val++; else val = min;
  else if (val>min) val--; else val = max;
}
void spinVal (paramptr& elem, const int8_t& direction) {
  if (direction>0)
    if (*elem.val < *elem.max) (*elem.val)++; else *elem.val = *elem.min;
  else if (*elem.val > *elem.min) (*elem.val)--; else *elem.val = *elem.max;
}

// проверяем, на сколько мы ушли за пределы отображаемого списка
byte calcShift(const byte& listsize, const byte& nSel, const byte& maxlines){
  if (listsize>maxlines && nSel>maxlines) {
      if (nSel<=listsize)
        return nSel-maxlines;
      else 
        return 0;
  }
  else return 0;
}

// считывание строки типа char из PROGMEM в буфер
char* readLinePROGMEM(char* &arr, uint16_t charMap){
  delete[] arr;
  arr = new char[strlen_P((char*)pgm_read_word(charMap))+1];
  strcpy_P(arr, (char*)pgm_read_word(charMap));
  return arr;
}

// компоновка меню для отображения на дисплее
void menuCompose(const byte &nSel,char** paramlist1, char** paramlist2, const byte &paramlistsize, const char *const actionList[], const byte &actionlistsize, byte NextColX = 95){
  OLED.clear();
  OLED.setScale(1);
  OLED.setCursor(0, 0);
  byte printheader = paramlist1[0][0]!='\0';
  byte maxlines = 4 - printheader;
  byte shift = calcShift(paramlistsize-1,nSel,maxlines);
  byte printmode = byte(paramlist2[0][0])-48;         // 48 Это код символа '0', получаем цифру
  if(printheader) {OLED.print(paramlist1[0]);}        // Заголовок окна
  for (byte i=1; i<paramlistsize && i<=maxlines; i++){
    OLED.setCursor(0, i-(!printheader));
    OLED.invertText(i+shift==nSel);
    OLED.print(paramlist1[i+shift]);
    if (printmode) {
      OLED.invertText(false);
      OLED.print(paramlist2[i+shift]);
    }
  }
  if (actionlistsize) {
    char* buffer = NULL;
    shift=calcShift(actionlistsize,nSel-paramlistsize+1,maxlines);
    OLED.fastLineV(NextColX, printheader*8, 31, OLED_FILL);
    NextColX+=3;
    for (byte i=0; i<actionlistsize && i<=maxlines; i++){
      OLED.setCursor(NextColX, i+printheader);
      OLED.invertText(i+shift==nSel-paramlistsize);
      readLinePROGMEM(buffer, &(actionList[i+shift]));
      OLED.println(buffer);
    }
    delete[] buffer;
  }
  OLED.invertText(false);
}

// компоновка меню из PROGMEM для отображения на дисплее
void menuComposePGM(const byte &nSel,const char *const paramlist1[], const char *const paramlist2[], const byte &paramlistsize, const char *const actionList[], const byte &actionlistsize, byte NextColX = 95){
  OLED.clear();
  OLED.setScale(1);
  OLED.setCursor(0, 0);
  char* buffer = NULL;

  readLinePROGMEM(buffer,&(paramlist2[0]));
  byte printmode = byte(buffer[0])-'0';     // 48 Это код символа '0', получаем цифру режима печати

  readLinePROGMEM(buffer,&(paramlist1[0])); // Теперь читаем в буфер заголовок
  byte printheader = buffer[0]!='\0'; 

  byte maxlines = 4 - printheader;
  byte shift = calcShift(paramlistsize-1,nSel,maxlines);
  if(printheader) {OLED.print(buffer);}     // Заголовок окна
  for (byte i=1; i<paramlistsize && i<=maxlines; i++){
    OLED.setCursor(0, i-(!printheader));
    OLED.invertText(i+shift==nSel);
    OLED.print(readLinePROGMEM(buffer,&(paramlist1[i+shift])));
    if (printmode) {
      OLED.invertText(false);
      readLinePROGMEM(buffer, &(paramlist2[i+shift]));
      OLED.print(buffer);
    }
  }
  if (actionlistsize) {
    shift=calcShift(actionlistsize,nSel-paramlistsize+1,maxlines);
    OLED.fastLineV(NextColX, printheader*8, 31, OLED_FILL);
    NextColX+=3;
    for (byte i=0; i<actionlistsize && i<=maxlines; i++){
      OLED.setCursor(NextColX, i+printheader);
      OLED.invertText(i+shift==nSel-paramlistsize);
      readLinePROGMEM(buffer, &(actionList[i+shift]));
      OLED.println(buffer);
    }
  }
  delete[] buffer;
  OLED.invertText(false);
}

// вынесено в целях абстрации, выводит на экран меню из подготовленного буфера
void menuShow(){
  OLED.update();
}


// типовое подменю дисплея для изменения текущего времени и параметров расписания
bool menuValChange(char* &header, paramptr paramArray[] = {}, const byte& paramArraySize = 0){
  byte nSel = 1;
  Serial.print(F("Editing ")); Serial.println(header);
  menuValChangeDraw(header, paramArray, paramArraySize, nSel);
  bool cancel = false;
  byte valueBkp[paramArraySize];
  for (byte i=0;i<paramArraySize;i++) valueBkp[i] = *paramArray[i].val;
  do {                                                                                  // переходим в режим ввода с энкодера
    enc.tick();                                                                         // опрашиваем энкодер
    if (enc.turn()) {                                                                   // выделяем другой пункт меню при вращении
      spinVal(nSel, 1, paramArraySize+menuValActionListSize, enc.getDir());
      menuValChangeDraw(header, paramArray, paramArraySize, nSel);
    }
    else if (enc.turnH() && nSel<=paramArraySize) {                                     // редактируем выбранный пункт при нажатом вращении
      spinVal(paramArray[nSel-1], enc.getDir());
      menuValChangeDraw(header, paramArray, paramArraySize, nSel);
    }
    else if (enc.click()) switch(nSel-paramArraySize){                                  // если был нажат выбранный пункт
      case 1:{                                                                          // пункт "Ok", сохранение установленных значений
        return true;
        break;
        }
      case 2:{                                                                          // пункт "Cancel", выход из подменю без сохранения
        for (byte i=0;i<paramArraySize;i++) *paramArray[i].val=valueBkp[i];
        return false;
        break;
        }
    }
  } while (!cancel);
  return false;
}
void menuValChangeDraw(char* &header,paramptr* &paramArray, const byte& paramArraySize, byte& nSel){
  char** menulist1 = new char*[paramArraySize+1];
  char** menulist2 = new char*[paramArraySize+1];
  for (byte i=0;i<paramArraySize;i++){
          menulist1[i+1] = new char[5];
          //if (*paramArray[i].val<10) menulist[i+1][0]=strcat('0',(*paramArray[i].val));
          //else menulist[i+1][0]=(char*)(*paramArray[i].val);
          sprintf(menulist1[i+1],"%02u", *paramArray[i].val);
          //itoa(*paramArray[i].val, menulist1[i+1], 10);
          menulist2[i+1]=paramArray[i].name;
  }
  menulist1[0] = new char[strlen(header)+1];
  strcpy(menulist1[0],header);
  menulist2[0] = new char[2];
  menulist2[0] = '1';
  menuCompose(nSel,menulist1, menulist2,paramArraySize+1,menuValActionList,menuValActionListSize,86);
  for (byte i=0;i<=paramArraySize;i++) {
    delete[] menulist1[i];
  }
  delete[] menulist1;
  delete[] menulist2;
  if (nSel<=paramArraySize) {
    OLED.fastLineH(3*8-2, 87, 128, OLED_FILL);
    OLED.setCursor(90, 3);
    OLED.print(String(*paramArray[nSel-1].min)+'-'+String(*paramArray[nSel-1].max));
  }
  menuShow();
}

// отрисовка главного меню, nSel-й элемент будет выделен, нумерация с 0
void menuMain (hydroponic& pot, DateTime& now){
  byte nSel=1;
  bool cancel = false;
  showMessage(2,F(" Settings"),2000);
  menuMainDraw(pot, now, nSel);                       // отрисовка главного меню с выделенным первым элементом
  do{                                                 // запуск бесконечного цикла опроса энкодера, пока не будет нажат пункт "Cancel"
    enc.tick();                                       // опрос состояния энкодера
    if (enc.turn()) {
      spinVal(nSel, 1, settingsSize-1+settingsActionListSize, enc.getDir());
      menuMainDraw(pot, now, nSel);
    }
    if (enc.click()){                                 // если был нажат выбранный пункт
      if (nSel<settingsSize) {
        
        char* header = NULL;
        char* header_hint = NULL;
        readLinePROGMEM(header,&(settingsList[nSel]));
        readLinePROGMEM(header_hint,&(settingsHintList[nSel]));

        char* newHeader = new char[strlen(header)+strlen(header_hint)+1];
        strcpy(newHeader, header);
        strcat(newHeader, header_hint);
        delete[] header;
        delete[] header_hint;

        switch(nSel){
          case 1: {                                     // пункт настройки полива
            paramptr paramArrayptr[] = {  {" (m) on",     pot.w_on,       justzero,       pot.w_off},
                                          {" (m) off",    pot.w_off,      pot.w_on,       minutemax}
                                        };
            menuValChange(newHeader,paramArrayptr,sizeof(paramArrayptr)/sizeof(paramArrayptr[0]));
          break;}
          case 2: {                                     // пункт настройки освещения
            paramptr paramArrayptr[] = {  {" (h) on",     pot.l_on,       justzero,       pot.l_off},
                                          {" (h) off",    pot.l_off,      pot.l_on,       hourmax}
                                        };
            menuValChange(newHeader,paramArrayptr,sizeof(paramArrayptr)/sizeof(paramArrayptr[0]));
          break;}
          case 3: {                                     // пункт настройки порога вкл/выкл освещения
            paramptr paramArrayptr[] = {  {" (%) on",     pot.br_lvl_on,  justzero,       pot.br_lvl_off},
                                          {" (%) off",    pot.br_lvl_off, pot.br_lvl_on,  percentmax}
                                        };
            menuValChange(newHeader,paramArrayptr,sizeof(paramArrayptr)/sizeof(paramArrayptr[0]));
          break;}
          case 4: {                                     // пункт настройки времени
            updTime(&now);
            paramptr paramArrayptr[] = {  {" (h)",         now.hour,       justzero,       hourmax},
                                          {" (m)",         now.minute,     justzero,       minutemax},
                                          {" (s)",         now.second,     justzero,       secondmax}
                                        };
            if (menuValChange(newHeader,paramArrayptr,sizeof(paramArrayptr)/sizeof(paramArrayptr[0])))
              RTC.setTime(now.second, now.minute, now.hour, 1, 12, 2021);
          break;}
          case 5: {                                     // пункт настройки всех пинов
            paramptr paramArrayptr[] = {  {" pin wtr",     pot.w_pin,      justzero,       digpinmax},
                                          {" pin lght",    pot.l_pin,      justzero,       digpinmax},
                                          {" pin br sens", grower1.br_sens_pin,analogpinmin,analogpinmax},
                                          {" pin led ind", grower1.ind_led_pin,justzero,   digpinmax},
                                          {" pin OLED en",grower1.OLED_en_pin,justzero,    digpinmax}
                                        };
            menuValChange(newHeader,paramArrayptr,sizeof(paramArrayptr)/sizeof(paramArrayptr[0]));
          break;}
          case 6: {                                     // пункт настройки таймаута дисплея
            paramptr paramArrayptr[] = {  {" (s)",    grower1.oled_timeout,justzero,       oledtimeoutmax}
                                        };
            menuValChange(newHeader,paramArrayptr,sizeof(paramArrayptr)/sizeof(paramArrayptr[0]));
          break;}
          case 7: {                                     // пункт "About"
            menuValChange(newHeader);
          break;}
        }
        delete[] newHeader;
      }
      else switch(nSel-settingsSize){
        case 0: {saveToEEPROM(&grower1);    cancel=true; break;}  // пункт "Save"
        case 1: {loadFromEEPROM(&grower1);  cancel=true; break;}  // пункт "Load"
        case 2: {restoreDefaults(&grower1); cancel=true; break;}  // пункт "Reset"
        case 3: {cancel=true; break;}                             // пункт "Cancel"
      }
      menuMainDraw(pot, now, nSel);
    }
  } while (!cancel);
  Serial.println(separator);
}

void menuMainDraw (const hydroponic& pot,const DateTime& now,const byte& nSel){
  menuComposePGM(nSel, settingsList, settingsHintList, settingsSize, settingsActionList, settingsActionListSize);
  menuShow();                                              // вывод меню на экран
}

// инициализация пинов выращивателя
void potPinMode (hydromodule &grower) {
  pinMode(grower.pot1.w_pin, OUTPUT);         // PIN управления поливом, настраивается вверху
  pinMode(grower.pot1.l_pin, OUTPUT);         // PIN дуправления освещением, настраивается вверху
  pinMode(grower.br_sens_pin, INPUT_PULLUP);  // PIN датчика освещенности, настраивается вверху
  pinMode(grower.ind_led_pin, OUTPUT);        // PIN светодиода-индикатора, настраивается вверху
}

// вызывается при подаче питания или после нажатия кнопки RESET на плате
void setup() {                                            // настройка устройства при подаче питания
  Serial.begin(SerialSpeed);                              // инициализация Serial на скорости SerialSpeed, заданной в начале
  Serial.println(separator);
  Serial.println(F("Starting..."));

  pinMode(grower1.OLED_en_pin, INPUT_PULLUP);             // PIN определения подключения дисплея, вкл, когда замкнут на землю
  grower1.useOLED=!digitalRead(grower1.OLED_en_pin);      // попытка инициализации дисплея. Если дисплей не подключен, grower1.useOLED=false и функционал меню настроек не будет использоваться
  if (grower1.useOLED) Serial.println(F("Display detected")); else Serial.println(F("Display didn't detected, resuming without it"));
  if (grower1.useOLED) OLED.init();

  loadFromEEPROM (&grower1);                              // загрузка параметров из энергонезависимой памяти
  potPinMode(grower1);                                    // инициализация пинов выращивателя

  if (!RTC.begin()) {                                     // попытка инициализации часов
    showMessage(2,F("Couldn't\n\rfind RTC"),0);           // а так же вывод ошибки в Serial и на дисплей, если часы не запустились
    for(;;) blink(1);                                     // после вывода ошибки бесконечное моргание
  }

  enc.tick();                                             // опрос состояния энкодера
  if (RTC.lostPower()) rstTime(F("RTC lstpwr"));          // сброс времени, если во время отсутствия питания была вынута батарейка часов
  else if (enc.state()) rstTime(F("RTC RESET!"));         // сброс времени, если при подаче питания была нажата кнопка энкодера

  Serial.println(separator);
}

// основная функция, выполняется циклически после функции setup()
void loop () {
  updTime(&now);                                                       // узнаем текущее время
  enc.tick();                                                          // опрашиваем энкодер
  
  if (millis()-myTimer2 >= 5*1000) {
    digitalWrite(grower1.ind_led_pin, HIGH);
    updState(grower1, now);                                            // обновляем состояние ножек
    sendToSerial(grower1.pot1, now);                                   // отправляем отладочную инфу в COM-порт на скорости 57600
    myTimer2 = millis();
    digitalWrite(grower1.ind_led_pin, LOW);
  }

  if (grower1.useOLED) {
    if (enc.hold()){menuMain(grower1.pot1, now); myTimer1=millis();}   // если был нажат и удержан энкодер/кнопка, запуск меню
    else if (enc.turn() || enc.click()) myTimer1=millis();
    else if (!grower1.oled_timeout || (millis()-myTimer1 <= grower1.oled_timeout*1000)) {
      sendToDisp(grower1.pot1, now);                                   // отправка отладочной инфы на экран до таймаута
    }
    else if (millis()-myTimer1>grower1.oled_timeout*1000) {OLED.clear(); OLED.update();}
  }

}
