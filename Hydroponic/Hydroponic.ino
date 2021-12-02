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
#define ENC_CLK_S1 11           // CLK/S1 - влияет на направление вращения,                            допустимые пины 2-13, A0-A3, A6-A7
#define ENC_DT_S2 12            // DT/S2 -  влияет на направление вращения,                            допустимые пины 2-13, A0-A3, A6-A7
#define ENK_key_SW 10           // key/SW - кнопка, замыкает GND,                                      допустимые пины 2-13, A0-A3, A6-A7

#define WTR_PIN A0              // pin реле увлажнения, обычно A0,                                     допустимые пины 2-13, A0-A3, A6-A7
#define LGHT_PIN A1             // pin реле освещения, обычно A1,                                      допустимые пины 2-13, A0-A3, A6-A7
#define BR_SENS_PIN A3          // pin датчик освещенности, обычно A3,                                 допустимые пины A0-A3, A6-A7
#define IND_LED_PIN 13          // pin светодиода-индикатора, обычно 13(распаян на плате)              допустимые пины 2-13, A0-A3, A6-A7

#define OLED_en_pin 2           // pin включения дисплея, если нужно задействовать, замыкаем на землю, допустимые пины 2-13, A0-A3, A6-A7

/*#################################  РАСПИСАНИЕ ПО-УМОЛЧАНИЮ  ############################################################################################################*/
#define oled_timeout 10         // время в секундах до автовыключения дисплея, 1-240 сек, 0 горит всегда
#define WTR_ON 0                // время вкл полива (минуты), 0-59
#define WTR_OFF 10              // время выкл полива (минуты), 0-59
#define LGHT_ON 8               // время вкл света (часы), 0-23
#define LGHT_OFF 21             // время выкл света (часы), 0-23
#define BRTNSS_ON 45            // уровень освещенности вкл света, обычно 52, 0-100
#define BRTNSS_OFF 55           // уровень освещенности выкл света, обычно 52, 0-100

/*########################################################################################################################################################################*/
#define projectversion F("version\n\r4.0.0")
#define SerialSpeed 57600
#define separator F("----------------------------------")
#include <EncButton.h>
#include <microDS3231.h>
#include <GyverOLED.h>
#include <EEManager.h>

struct paramptr {
  paramptr(){};
  paramptr(String _name, byte& _val, byte& _min, byte& _max){name = _name; val = &_val; min = &_min; max = &_max;};
  paramptr(String _name, byte& _val, const byte& _min, byte& _max){name = _name; val = &_val; min = &_min; max = &_max;};
  paramptr(String _name, byte& _val, byte& _min, const byte& _max){name = _name; val = &_val; min = &_min; max = &_max;};
  paramptr(String _name, byte& _val, const byte& _min, const byte& _max){name = _name; val = &_val; min = &_min; max = &_max;};
  String name;
  byte *val;
  byte *min;
  byte *max;
};

// struct param {
//   param(){};
//   param(String _name, byte& _val, byte _min, byte _max){name = _name; val = _val; min = _min; max = _max;};
//   param(byte& _val, byte _min, byte _max){val = _val; min = _min; max = _max;};
//   param(paramptr _paramptr){name = _paramptr.name; val = *_paramptr.val; min = *_paramptr.min; max = *_paramptr.max;};
//   String name;
//   byte val;
//   byte min;
//   byte max;
//   paramptr toParamPtr(){
//     return {name, &val, &min, &max};
//   };
// };

struct hydroponic {
    byte w_pin = WTR_PIN;
    byte w_on  = WTR_ON;
    byte w_off = WTR_OFF;
    byte l_pin = LGHT_PIN;
    byte l_on  = LGHT_ON;
    byte l_off = LGHT_OFF;
    byte br_lvl_on  = BRTNSS_ON;
    byte br_lvl_off = BRTNSS_OFF;
    byte brightness;
    bool wtr_pin_state;
    bool lght_pin_state;
};
struct hydromodule {
  byte ind_led_pin = IND_LED_PIN;
  byte br_sens_pin = BR_SENS_PIN;
  hydroponic pot1;
};
hydromodule hydromodule1;
bool useOLED;
EEManager memory(hydromodule1);
MicroDS3231 RTC;
DateTime now;
#define defaultHours 12
#define defaultMinutes 0
GyverOLED<SSD1306_128x32, OLED_BUFFER> OLED(0x3C);
EncButton<EB_TICK, ENC_CLK_S1, ENC_DT_S2, ENK_key_SW> enc;
uint32_t myTimer1 = 0;
uint32_t myTimer2 = 5*1000;
const byte justzero = 0;
const byte secondmax = 59;
const byte minutemax = 59;
const byte hourmax = 59;
const byte percentmax = 100;
const byte pinmax = 19;
const byte analogpinmin = 14;
const byte analogpinmax = 21;

// мигание индицирующим светодиодом i раз
void blink (byte i){
  for (;i>0;i--) {
    digitalWrite(hydromodule1.ind_led_pin, HIGH);
    delay(70);
    digitalWrite(hydromodule1.ind_led_pin, LOW);
    delay(250);
  }
}

// отображение message на дисплее текстом размера 1 или 2 и продолжительностью ms/1000 секунд
void showMessage(byte text_size,String message,int ms=3500){
  Serial.println(message);
  if(useOLED){
    OLED.clear();
    OLED.setCursor(0, 0);
    OLED.setScale(text_size);
    OLED.print(message);
    OLED.update();
    delay(ms);
  }
}

// сброс времени на 1 января 2020 12:00
void rstTime(String reason){
  RTC.setTime(0,defaultMinutes,defaultHours,1,12,2021);
  showMessage(2,reason+F("\n\rnow ")+RTC.getTimeString().substring(0,5),2000);
  blink(5);
}

// сохранение параметров расписания в постоянную память
void saveToEEPROM (hydromodule& hydromodule1) {
  memory.updateNow();
  showMessage(2,F("Saved to\n\rEEPROM"),2000);
}

// загрузка параметров расписания из постоянной памяти
void loadFromEEPROM (hydromodule& hydromodule1) {
  byte stat = memory.begin(1,'x');
  if (stat==1) {showMessage(2,F("Loaded\n\rEEPROM"),2000); }
  else if (stat==2) {showMessage(2,F("EEPROM\n\ris empty"),2000); restoreDefaults(hydromodule1); }
  else showMessage(2,F("EEPROM\n\r!ERROR!"), 3500);
}

// взять стандартные значения из скетча
void restoreDefaults (hydromodule& hydromodule1) {
  hydromodule newhydromodule;
  hydromodule1=newhydromodule;
  showMessage(2,F("Loaded\n\rdefaults"),2000);
  saveToEEPROM(hydromodule1);
}

// обновление текущего времени
void updTime (DateTime& now) {
  now = RTC.getTime();
}

// проверка, какие ножки нужно включить, какие выключить
void updState (hydromodule& hydromodule1, DateTime& now){
  hydromodule1.pot1.wtr_pin_state = now.minute>=hydromodule1.pot1.w_on && now.minute<hydromodule1.pot1.w_off; // полив вкл, если минут больше w_on и меньше w_off
  hydromodule1.pot1.brightness=map(analogRead(hydromodule1.br_sens_pin),1019,10,0,100);
  if (now.hour>=hydromodule1.pot1.l_on && now.hour<hydromodule1.pot1.l_off) {                                 // свет вкл, если попадает на заданный промежуток и внешний свет ниже порогового
    if (hydromodule1.pot1.brightness < hydromodule1.pot1.br_lvl_on) hydromodule1.pot1.lght_pin_state = true;
    else if (hydromodule1.pot1.brightness > hydromodule1.pot1.br_lvl_off) hydromodule1.pot1.lght_pin_state = false;
  } else hydromodule1.pot1.lght_pin_state = false;
  digitalWrite(hydromodule1.pot1.l_pin, hydromodule1.pot1.lght_pin_state);                                    // задаем нужное состояние освещения
  digitalWrite(hydromodule1.pot1.w_pin, hydromodule1.pot1.wtr_pin_state);                                     // задаем нужное состояние полива
}

// отправка времени, состояния и расписания в COM-порт
void sendToSerial (hydroponic& pot, DateTime& now) {
  Serial.println(String(now.hour)+F("h:")+now.minute+F("m:")+now.second+F("s"));
  Serial.println(String(F("wtr="))+(pot.wtr_pin_state?F("on"):F("off"))+F(" (")+pot.w_on+F("on,")+pot.w_off+F("off)"));
  Serial.println(String(F("lght="))+(pot.lght_pin_state?F("on"):F("off"))+F(" (")+pot.l_on+F("on,")+pot.l_off+F("off)"));
  Serial.println(String(F("brightness="))+String(pot.brightness)+F("% <")+pot.br_lvl_on+F("on,>")+pot.br_lvl_off+F("off"));
  Serial.println(separator);
}

// отправка отладочной инфы на дисплей
void sendToDisp (hydroponic& pot, DateTime& now) {
  OLED.clear();
  OLED.setScale(1);
  OLED.setCursor(0, 0);
  OLED.println(String(now.hour)+F("h:")+now.minute+F("m:")+now.second+F("s"));
  OLED.println(String(F("wtr="))+(pot.wtr_pin_state?F("on"):F("off"))+F(" (")+pot.w_on+F("on,")+pot.w_off+F("off)"));
  OLED.println(String(F("lght="))+(pot.lght_pin_state?F("on"):F("off"))+F(" (")+pot.l_on+F("on,")+pot.l_off+F("off)"));
  OLED.println(String(F("br="))+String(pot.brightness)+F("% <")+pot.br_lvl_on+F("on,>")+pot.br_lvl_off+F("off"));
  OLED.update();
}

// прокручиваем значение val по кругу между min и max
void spinVal (byte& val, byte min, byte max, int8_t direction) {
  if (direction>0)
    if (val<max) val++; else val = min;
  else if (val>min) val--; else val = max;
}
void spinVal (paramptr& elem, int8_t direction) {
  if (direction>0)
    if (*elem.val < *elem.max) (*elem.val)++; else *elem.val = *elem.min;
  else if (*elem.val > *elem.min) (*elem.val)--; else *elem.val = *elem.max;
}

// компоновка меню для отображения на дисплее
void menuCompose(const String menulist[][2], const byte &menusize, const byte &n, const String &header = (const String)(""), byte NextColAfterItem = 3, byte NextColX = 98){
  OLED.clear();
  OLED.setScale(1);
  OLED.setCursor(0, 0);
  if(header!="") {OLED.println(header); NextColAfterItem++;}
  for (byte i=0; i<menusize; i++){
    OLED.invertText(i+1==n);
    if (i>NextColAfterItem) {OLED.setCursor(NextColX, i-NextColAfterItem-1);}
    OLED.print(menulist[i][0]);
    OLED.invertText(false);
    OLED.print(menulist[i][1]);
  }
}
void menuShow(){
  OLED.update();
}


// типовое подменю дисплея для изменения текущего времени и параметров расписания
bool menuValChange(const String& header, paramptr paramArray[], const byte& size){
  byte nSel = 0;
  Serial.println((String)F("Editing ") + header);
    bool cancel = false;
    menuValChange(header, paramArray, size, ++nSel);
    byte valueBkp[size];
    for (byte i=0;i<size;i++) valueBkp[i] = *paramArray[i].val;
    do {                                                                                  // переходим в режим ввода с энкодера
      enc.tick();                                                                         // опрашиваем энкодер
      if (enc.turn()) {                                                                   // выделяем другой пункт меню при вращении
        spinVal(nSel, 1, size+2, enc.getDir());
        menuValChange(header, paramArray, size, nSel);
      }
      else if (enc.turnH() && nSel<=size) {                                               // редактируем выбранный пункт при нажатом вращении
        spinVal(paramArray[nSel-1], enc.getDir());
        menuValChange(header, paramArray, size, nSel);
      }
      else if (enc.click()) switch(nSel-size){                                            // если был нажат выбранный пункт
        case 1:{                                                                          // пункт "Ok", сохранение установленных значений
          return true;
          break;
          }
        case 2:{                                                                          // пункт "Cancel", выход из подменю без сохранения
          for (byte i=0;i<size;i++) *paramArray[i].val=valueBkp[i];
          return false;
          break;
          }
      }
    } while (!cancel);
    return false;
}
void menuValChange(const String& header, paramptr paramArray[], const byte& size, byte& nSel){
  String menulist[size+2][2];
  for (byte i=0;i<size;i++){
          if (*paramArray[i].val<10) menulist[i][0]='0'+String(*paramArray[i].val);
          else menulist[i][0]=String(*paramArray[i].val);
          menulist[i][1]=paramArray[i].name;
  }
  menulist[size][0] = F("\n\rOk");
  menulist[size][1] = F(" ");
  menulist[size+1][0] = F("Cancel");
  menuCompose(menulist,size+2, nSel, header);                                           // компоновка меню
  if (nSel<=size) {
    OLED.setCursor(86, 1);
    OLED.print((String)F("(")+String(*paramArray[nSel-1].min)+F("-")+String(*paramArray[nSel-1].max)+F(")"));
  }
  menuShow();
}

// отрисовка главного меню, nSel-й элемент будет выделен, нумерация с 0
void menuMain (hydroponic& pot, DateTime& now, byte nSel = 0){
  const String menulist[][2]={
                        {{F("Watering\n\r")},   {F("")}},
                        {{F("Lighting\n\r")},   {F("")}},
                        {{F("Brightness\n\r")}, {F("")}},
                        {{F("Time\n\r")},       {F("")}},
                        {{F("Save")},           {F("")}},
                        {{F("Load")},           {F("")}},
                        {{F("Reset")},          {F("")}},
                        {{F("Close")},          {F("")}}
                        };
  if (nSel){
    menuCompose(menulist,8, nSel);                      // 8 кол-во эл меню
    OLED.fastLineV(94, 0, 31, OLED_FILL);               // компоновка линии по координатам (94,0) -> (94,31)
    menuShow();                                         // вывод меню и линии на экран
  }
  else {
    bool cancel = false;
    Serial.println(F("Menu Entered"));
    showMessage(2,F(" Settings"),2000);
    menuMain(pot, now, ++nSel);                         // отрисовка главного меню с выделенным первым элементом
    do{                                                 // запуск бесконечного цикла опроса энкодера, пока не будет нажат пункт "Cancel"
      enc.tick();                                       // опрос состояния энкодера
      if (enc.turn()) {
        spinVal(nSel, 1, 8, enc.getDir());
        menuMain(pot, now, nSel);
      }
      if (enc.click()){                                 // если был нажат выбранный пункт
        switch(nSel){
          case 1: {                                                                                       // пункт настройки полива
            paramptr paramArrayptr[3] = { {F(" pin\n\r"),   pot.w_pin,      justzero,       pinmax},
                                          {F("(m) on - "),  pot.w_on,       justzero,       pot.w_off},
                                          {F("(m) off"),    pot.w_off,      pot.w_on,       minutemax}
                                          };
            menuValChange(F("Water (mins, hrly)"),paramArrayptr,sizeof(paramArrayptr)/sizeof(paramArrayptr[0]));
            break;}
          case 2: {                                                                                       // пункт настройки освещения
            paramptr paramArrayptr[3] = { {F(" pin\n\r"),   pot.l_pin,      justzero,       pinmax},
                                          {F("(h) on - "),  pot.l_on,       justzero,       pot.l_off},
                                          {F("(h) off"),    pot.l_off,      pot.l_on,       hourmax}
                                          };
            menuValChange(F("Light (hrs, daily)"),paramArrayptr,sizeof(paramArrayptr)/sizeof(paramArrayptr[0]));
            break;}
          case 3: {                                                                                       // пункт настройки порога вкл/выкл освещения
            paramptr paramArrayptr[3] = { {F(" pin\n\r"),hydromodule1.br_sens_pin,analogpinmin,analogpinmax},
                                          {F("(%) on - "),  pot.br_lvl_on,  justzero,       pot.br_lvl_off},
                                          {F("(%) off"),    pot.br_lvl_off, pot.br_lvl_on,  percentmax}
                                          };
            menuValChange(F("Brightness (%,hyst)"),paramArrayptr,sizeof(paramArrayptr)/sizeof(paramArrayptr[0]));
             break;}
          case 4: {                                                                                       // пункт настройки времени
            paramptr paramArrayptr[3] = { {F("h:"),         now.hour,       justzero,       hourmax},
                                          {F("m:"),         now.minute,     justzero,       minutemax},
                                          {F("s"),          now.second,     justzero,       secondmax}
                                          };
            if (menuValChange(F("Time"),paramArrayptr,sizeof(paramArrayptr)/sizeof(paramArrayptr[0]))) RTC.setTime(now.second, now.minute, now.hour, 1, 12, 2021);
            break;}
          case 5: {saveToEEPROM(hydromodule1); cancel=true; break;}   // пункт "Save"
          case 6: {loadFromEEPROM(hydromodule1); cancel=true; break;}   // пункт "Load"
          case 7: {restoreDefaults(hydromodule1); cancel=true; break;}                                             // пункт "Reset"
          case 8: {cancel=true; break;}                                                                   // пункт "Cancel"
        }
        menuMain(pot, now, nSel);
      }
    } while (!cancel);
    Serial.println(separator);
  }
}

// инициализация пинов полива и освещения
void potPinMode (hydroponic& pot) {
  pinMode(pot.w_pin, OUTPUT);                             // PIN управления поливом, настраивается вверху
  pinMode(pot.l_pin, OUTPUT);                             // PIN дуправления освещением, настраивается вверху
}

// вызывается при подаче питания или после нажатия кнопки RESET на плате
void setup() {                                            // настройка устройства при подаче питания
  Serial.begin(SerialSpeed);                              // инициализация Serial на скорости SerialSpeed, заданной в начале
  Serial.println(separator);
  Serial.println(F("Starting..."));

  pinMode(OLED_en_pin, INPUT_PULLUP);                     // PIN определения подключения дисплея, вкл, когда замкнут на землю
  useOLED=!digitalRead(OLED_en_pin);                      // попытка инициализации дисплея. Если дисплей не подключен, useOLED=false и функционал меню настроек не будет использоваться
  if (useOLED) OLED.init();
  if (useOLED) Serial.println(F("Display detected")); else Serial.println(F("Display didn't detected, resuming without it"));
  showMessage(2,projectversion,2000);

  loadFromEEPROM (hydromodule1);
  pinMode(hydromodule1.br_sens_pin, INPUT_PULLUP);        // PIN датчика освещенности, настраивается вверху
  pinMode(hydromodule1.ind_led_pin, OUTPUT);              // PIN светодиода-индикатора, настраивается вверху
  potPinMode(hydromodule1.pot1);                          // инициализация пинов полива и освещения для горшка

  if (!RTC.begin()) {                                     // попытка инициализации часов
    showMessage(2,F("Couldn't\n\rfind RTC"),0);           // а так же вывод ошибки в Serial и на дисплей, если часы не запустились
    for(;;) blink(1);                                     // после вывода ошибки бесконечное моргание
  }

  enc.tick();                                             // опрос состояния энкодера
  if (RTC.lostPower()) rstTime(F("RTC lstpwr"));          // сброс времени, если во время отсутствия питания была вынута батарейка часов
  else if (enc.state()) rstTime(F("RTC RESET!"));         // сброс времени, если при подаче питания была нажата кнопка энкодера

  Serial.println(separator);
}

// основная функция, выполняется циклически
void loop () {                                            // циклическая функция, стартует после функции setup()
  updTime(now);                                           // узнаем текущее время
  enc.tick();
  
  if (millis()-myTimer2 >= 5*1000) {
    digitalWrite(hydromodule1.ind_led_pin, HIGH);
    updState(hydromodule1, now);                                        // обновляем состояние ножек
    sendToSerial(hydromodule1.pot1, now);                               // отправляем отладочную инфу в COM-порт на скорости 57600
    myTimer2 = millis();
    digitalWrite(hydromodule1.ind_led_pin, LOW);
  }

  if (useOLED) {
    if (!oled_timeout || (millis()-myTimer1 <= oled_timeout*1000)) {
      sendToDisp(hydromodule1.pot1, now);                               // отправка отладочной инфы на экран до таймаута
    }
    else if (millis()-myTimer1>oled_timeout*1000) {OLED.clear(); OLED.update();}
    if (enc.hold()){menuMain(hydromodule1.pot1, now); myTimer1=millis();}            // если был нажат и удержан энкодер/кнопка, запуск меню
    else if (enc.click()) myTimer1=millis();
  }

}