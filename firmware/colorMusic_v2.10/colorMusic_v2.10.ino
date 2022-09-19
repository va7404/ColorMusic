/*
  Скетч к проекту "Светомузыка на Arduino"
  Страница проекта (схемы, описания): https://alexgyver.ru/colormusic/
  Исходники на GitHub: https://github.com/AlexGyver/ColorMusic
  Нравится, как написан код? Поддержи автора! https://alexgyver.ru/support_alex/
  Автор: AlexGyver Technologies, 2018
  https://AlexGyver.ru/

  Как откалибровать уровень шума и как пользоваться пультом
  расписано на странице проекта! https://alexgyver.ru/colormusic/
*/

/*
   Версия 2.10
   Исправлен глюк с большим количеством светодиодов на МЕГЕ
*/


// ***************************** НАСТРОЙКИ *****************************

// ----- настройка ИК пульта
#define REMOTE_TYPE 1       // 0 - без пульта, 1 - пульт от WAVGAT, 2 - пульт от KEYES, 3 - кастомный пульт
// система может работать С ЛЮБЫМ ИК ПУЛЬТОМ (практически). Коды для своего пульта можно задать начиная со строки 160 в прошивке. Коды пультов определяются скетчем IRtest_2.0, читай инструкцию

// ----- настройки параметров
#define KEEP_SETTINGS 1     // хранить ВСЕ настройки в энергонезависимой памяти
#define KEEP_STATE 1		    // сохранять в памяти состояние вкл/выкл системы (с пульта)
#define RESET_SETTINGS 0    // сброс настроек в EEPROM памяти (поставить 1, прошиться, поставить обратно 0, прошиться. Всё)
#define SETTINGS_LOG 1      // вывод всех настроек из EEPROM в порт при запуске

// ----- настройки ленты
#define NUM_LEDS 180        // количество светодиодов (данная версия поддерживает до 410 штук)
#define CURRENT_LIMIT 3000  // лимит по току в МИЛЛИАМПЕРАХ, автоматически управляет яркостью (пожалей свой блок питания!) 0 - выключить лимит
byte BRIGHTNESS = 200;      // яркость по умолчанию (0 - 255)

// ----- пины подключения
#define SOUND_R A2         // аналоговый пин вход аудио, правый канал
#define SOUND_L A1         // аналоговый пин вход аудио, левый канал
#define SOUND_R_FREQ A2    // аналоговый пин вход аудио для режима с частотами (через кондер)
#define BTN_PIN 3          // кнопка переключения режимов (PIN --- КНОПКА --- GND)

#if defined(__AVR_ATmega32U4__) // Пины для Arduino Pro Micro (смотри схему для Pro Micro на странице проекта!!!)
#define MLED_PIN 17             // пин светодиода режимов на ProMicro, т.к. обычный не выведен.
#define MLED_ON LOW
#define LED_PIN 9               // пин DI светодиодной ленты на ProMicro, т.к. обычный не выведен.
#else                           // Пины для других плат Arduino (по умолчанию)
#define MLED_PIN 13             // пин светодиода режимов
#define MLED_ON HIGH
#define LED_PIN 12              // пин DI светодиодной ленты
#endif

#define POT_GND A0              // пин земля для потенциометра
#define IR_PIN 2                // пин ИК приёмника

// ----- настройки радуги
float RAINBOW_STEP = 5.00;         // шаг изменения цвета радуги

// ----- отрисовка
#define MODE 0                    // режим при запуске
#define MAIN_LOOP 5               // период основного цикла отрисовки (по умолчанию 5)

// ----- сигнал
#define MONO 1                    // 1 - только один канал (ПРАВЫЙ!!!!! SOUND_R!!!!!), 0 - два канала
#define EXP 1.5                   // степень усиления сигнала (для более "резкой" работы) (по умолчанию 1.4)
#define POTENT 0                  // 1 - используем потенциометр, 0 - используется внутренний источник опорного напряжения 1.1 В
byte EMPTY_BRIGHT = 30;           // яркость "не горящих" светодиодов (0 - 255)
#define EMPTY_COLOR HUE_PURPLE    // цвет "не горящих" светодиодов. Будет чёрный, если яркость 0

// ----- нижний порог шумов
uint16_t LOW_PASS = 0;          // нижний порог шумов режим VU, ручная настройка
uint16_t SPEKTR_LOW_PASS = 0;    // нижний порог шумов режим спектра, ручная настройка
#define AUTO_LOW_PASS 0           // разрешить настройку нижнего порога шумов при запуске (по умолч. 0)
#define EEPROM_LOW_PASS 1         // порог шумов хранится в энергонезависимой памяти (по умолч. 1)
#define LOW_PASS_ADD 13           // "добавочная" величина к нижнему порогу, для надёжности (режим VU)
#define LOW_PASS_FREQ_ADD 3       // "добавочная" величина к нижнему порогу, для надёжности (режим частот)

// ----- режим шкала громкости
float SMOOTH = 0.8;               // коэффициент плавности анимации VU (по умолчанию 0.5)
#define MAX_COEF 1.2              // коэффициент громкости (максимальное равно срднему * этот коэф) (по умолчанию 1.8)

// ----- режим цветомузыки
float SMOOTH_FREQ = 0.8;          // коэффициент плавности анимации частот (по умолчанию 0.8)
float MAX_COEF_FREQ = 1.1;        // коэффициент порога для "вспышки" цветомузыки (по умолчанию 1.5)
float MAX_COEF_FREQ0 = 1.1;        // коэффициент порога для "вспышки" цветомузыки низких частот

int SMOOTH_STEP=20;            // шаг уменьшения яркости в режиме цветомузыки (чем больше, тем быстрее гаснет)
#define LOW_COLOR HUE_RED         // цвет низких частот
#define MID_COLOR HUE_GREEN       // цвет средних
#define HIGH_COLOR HUE_YELLOW     // цвет высоких

// ----- режим стробоскопа
uint16_t STROBE_PERIOD = 140;     // период вспышек, миллисекунды
#define STROBE_DUTY 20            // скважность вспышек (1 - 99) - отношение времени вспышки ко времени темноты
#define STROBE_COLOR HUE_YELLOW   // цвет стробоскопа
#define STROBE_SAT 0              // насыщенность. Если 0 - цвет будет БЕЛЫЙ при любом цвете (0 - 255)
byte STROBE_SMOOTH = 200;         // скорость нарастания/угасания вспышки (0 - 255)

// ----- режим подсветки
byte LIGHT_COLOR = 0;             // начальный цвет подсветки
byte LIGHT_SAT = 255;             // начальная насыщенность подсветки
byte COLOR_SPEED = 100;
int RAINBOW_PERIOD = 1;
float RAINBOW_STEP_2 = 0.5;

// ----- режим бегущих частот
byte RUNNING_SPEED = 11;

// ----- режим анализатора спектра
byte HUE_START = 0;
byte HUE_STEP = 5;
#define LIGHT_SMOOTH 2

//-- для определения границ анализа спектра
float avgFht;
float sigma;
int toneCount;






/*
  Цвета для HSV
  HUE_RED
  HUE_ORANGE
  HUE_YELLOW
  HUE_GREEN
  HUE_AQUA
  HUE_BLUE
  HUE_PURPLE
  HUE_PINK
*/

// ----- КНОПКИ ПУЛЬТА WAVGAT -----
#if REMOTE_TYPE == 1
#define BUTT_UP     0xF39EEBAD
#define BUTT_DOWN   0xC089F6AD
#define BUTT_LEFT   0xE25410AD
#define BUTT_RIGHT  0x14CE54AD
#define BUTT_OK     0x297C76AD
#define BUTT_1      0x4E5BA3AD
#define BUTT_2      0xE51CA6AD
#define BUTT_3      0xE207E1AD
#define BUTT_4      0x517068AD
#define BUTT_5      0x1B92DDAD
#define BUTT_6      0xAC2A56AD
#define BUTT_7      0x5484B6AD
#define BUTT_8      0xD22353AD
#define BUTT_9      0xDF3F4BAD
#define BUTT_0      0xF08A26AD
#define BUTT_STAR   0x68E456AD
#define BUTT_HASH   0x151CD6AD
#endif

// ----- КНОПКИ ПУЛЬТА KEYES -----
#if REMOTE_TYPE == 2
#define BUTT_UP     0xE51CA6AD
#define BUTT_DOWN   0xD22353AD
#define BUTT_LEFT   0x517068AD
#define BUTT_RIGHT  0xAC2A56AD
#define BUTT_OK     0x1B92DDAD
#define BUTT_1      0x68E456AD
#define BUTT_2      0xF08A26AD
#define BUTT_3      0x151CD6AD
#define BUTT_4      0x18319BAD
#define BUTT_5      0xF39EEBAD
#define BUTT_6      0x4AABDFAD
#define BUTT_7      0xE25410AD
#define BUTT_8      0x297C76AD
#define BUTT_9      0x14CE54AD
#define BUTT_0      0xC089F6AD
#define BUTT_STAR   0xAF3F1BAD
#define BUTT_HASH   0x38379AD
#endif

// ----- КНОПКИ СВОЕГО ПУЛЬТА -----
#if REMOTE_TYPE == 3
#define BUTT_UP     0xE51CA6AD
#define BUTT_DOWN   0xD22353AD
#define BUTT_LEFT   0x517068AD
#define BUTT_RIGHT  0xAC2A56AD
#define BUTT_OK     0x1B92DDAD
#define BUTT_1      0x68E456AD
#define BUTT_2      0xF08A26AD
#define BUTT_3      0x151CD6AD
#define BUTT_4      0x18319BAD
#define BUTT_5      0xF39EEBAD
#define BUTT_6      0x4AABDFAD
#define BUTT_7      0xE25410AD
#define BUTT_8      0x297C76AD
#define BUTT_9      0x14CE54AD
#define BUTT_0      0xC089F6AD
#define BUTT_STAR   0xAF3F1BAD  // *
#define BUTT_HASH   0x38379AD   // #
#endif


// ------------------------------ ДЛЯ РАЗРАБОТЧИКОВ --------------------------------
#define MODE_AMOUNT 9      // количество режимов

#define STRIPE NUM_LEDS / 5
float freq_to_stripe = NUM_LEDS / 40; // /2 так как симметрия, и /20 так как 20 частот

#define FHT_N 64         // ширина спектра х2
#define LOG_OUT 1
#include <FHT.h>         // преобразование Хартли

#include <EEPROMex.h>

#define FASTLED_ALLOW_INTERRUPTS 1
#include "FastLED.h"
CRGB leds[NUM_LEDS];

#include "GyverButton.h"
GButton butt1(BTN_PIN);

#include "IRLremote.h"
CHashIR IRLremote;
uint32_t IRdata;

// градиент-палитра от зелёного к красному
DEFINE_GRADIENT_PALETTE(soundlevel_gp) {
  0,    0,    255,  0,  // green
  100,  255,  255,  0,  // yellow
  150,  255,  100,  0,  // orange
  200,  255,  50,   0,  // red
  255,  255,  0,    0   // red
};
CRGBPalette32 myPal = soundlevel_gp;

int Rlenght, Llenght;
float RsoundLevel, RsoundLevel_f, minLevel_f;
float LsoundLevel, LsoundLevel_f;

float averageLevel = -1;
float averageLevelMin = 0;
float maxLevel;
float minLevel = 100;

int rMinLevel = 1023;
int rMaxLevel = 0;
float avgMinRealLevel = 0;
float avgMaxRealLevel = 50;
int avrageMinLevel=0;

int MAX_CH = NUM_LEDS / 2;
int hue;
unsigned long main_timer, hue_timer, strobe_timer, running_timer, color_timer, rainbow_timer, eeprom_timer;
float averK = 0.05;
float averKFreg = 0.3;  
float averKFreg0 = 0.3;
byte count;
float index = (float)255 / MAX_CH;   // коэффициент перевода для палитры
boolean lowFlag;
byte low_pass;
int RcurrentLevel, LcurrentLevel;
int colorMusic[3];
int toneColorCount[3];
float colorMusic_f[3], colorMusic_aver[3];
boolean colorMusicFlash[3], strobeUp_flag, strobeDwn_flag;
byte this_mode = MODE;
int thisBright[3], strobe_bright = 0;
unsigned int light_time = STROBE_PERIOD * STROBE_DUTY / 100;
volatile boolean ir_flag;
boolean settings_mode, ONstate = true;
//int8_t freq_strobe_mode, light_mode;
int freq_max;
float freq_max_f, rainbow_steps;
int freq_f[32];
int this_color;
boolean running_flag[3], eeprom_flag;

#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
// ------------------------------ ДЛЯ РАЗРАБОТЧИКОВ --------------------------------

void setup() {
  Serial.begin(9600);
  FastLED.addLeds<WS2811, LED_PIN, GRB>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  if (CURRENT_LIMIT > 0) FastLED.setMaxPowerInVoltsAndMilliamps(5, CURRENT_LIMIT);
  FastLED.setBrightness(BRIGHTNESS);

#if defined(__AVR_ATmega32U4__)   //Выключение светодиодов на Pro Micro
  TXLED1;                           //на ProMicro выключим и TXLED
  delay (1000);                     //При питании по usb от компьютера нужна задержка перед выключением RXLED. Если питать от БП, то можно убрать эту строку.
#endif
  pinMode(MLED_PIN, OUTPUT);        //Режим пина для светодиода режима на выход
  digitalWrite(MLED_PIN, !MLED_ON); //Выключение светодиода режима

  pinMode(POT_GND, OUTPUT);
  digitalWrite(POT_GND, LOW);
  butt1.setTimeout(900);

  IRLremote.begin(IR_PIN);
  analogReference(INTERNAL);
/*
  // для увеличения точности уменьшаем опорное напряжение,
  // выставив EXTERNAL и подключив Aref к выходу 3.3V на плате через делитель
  // GND ---[10-20 кОм] --- REF --- [10 кОм] --- 3V3
  // в данной схеме GND берётся из А0 для удобства подключения
  if (POTENT) analogReference(EXTERNAL);
  else
#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
    analogReference(INTERNAL1V1);
#else
    analogReference(INTERNAL);
#endif
*/
  // автор этих комментариев долбоеб:
  // жуткая магия, меняем частоту оцифровки до 18 кГц
  // команды на ебучем ассемблере, даже не спрашивайте, как это работает
  // поднимаем частоту опроса аналогового порта до 38.4 кГц, по теореме
  // Котельникова (Найквиста) частота дискретизации будет 19.2 кГц
  // http://yaab-arduino.blogspot.ru/2015/02/fast-sampling-from-analog-input.html
  sbi(ADCSRA, ADPS2);
  cbi(ADCSRA, ADPS1);
  sbi(ADCSRA, ADPS0);

  if (RESET_SETTINGS) EEPROM.write(100, 0);        // сброс флага настроек

  if (AUTO_LOW_PASS && !EEPROM_LOW_PASS) {         // если разрешена автонастройка нижнего порога шумов
    autoLowPass();
  }
  if (EEPROM_LOW_PASS) {                // восстановить значения шумов из памяти
    //LOW_PASS = EEPROM.readInt(70);
    //SPEKTR_LOW_PASS = EEPROM.readInt(72);
  }

  // в 100 ячейке хранится число 100. Если нет - значит это первый запуск системы
  if (KEEP_SETTINGS) {
    if (EEPROM.read(100) != 100) {
      //Serial.println(F("First start"));
      EEPROM.write(100, 100);
      updateEEPROM();
    } else {
      readEEPROM();
    }
  }

#if (SETTINGS_LOG == 1)
  Serial.print(F("this_mode = ")); Serial.println(this_mode);
  Serial.print(F("RAINBOW_STEP = ")); Serial.println(RAINBOW_STEP);
  Serial.print(F("MAX_COEF_FREQ = ")); Serial.println(MAX_COEF_FREQ);
  Serial.print(F("STROBE_PERIOD = ")); Serial.println(STROBE_PERIOD);
  Serial.print(F("LIGHT_SAT = ")); Serial.println(LIGHT_SAT);
  Serial.print(F("RAINBOW_STEP_2 = ")); Serial.println(RAINBOW_STEP_2);
  Serial.print(F("HUE_START = ")); Serial.println(HUE_START);
  Serial.print(F("SMOOTH = ")); Serial.println(SMOOTH);
  Serial.print(F("SMOOTH_FREQ = ")); Serial.println(SMOOTH_FREQ);
  Serial.print(F("STROBE_SMOOTH = ")); Serial.println(STROBE_SMOOTH);
  Serial.print(F("LIGHT_COLOR = ")); Serial.println(LIGHT_COLOR);
  Serial.print(F("COLOR_SPEED = ")); Serial.println(COLOR_SPEED);
  Serial.print(F("RAINBOW_PERIOD = ")); Serial.println(RAINBOW_PERIOD);
  Serial.print(F("RUNNING_SPEED = ")); Serial.println(RUNNING_SPEED);
  Serial.print(F("HUE_STEP = ")); Serial.println(HUE_STEP);
  Serial.print(F("EMPTY_BRIGHT = ")); Serial.println(EMPTY_BRIGHT);
  Serial.print(F("ONstate = ")); Serial.println(ONstate);
#endif
}

void loop() {
  buttonTick();     // опрос и обработка кнопки
#if REMOTE_TYPE != 0
  remoteTick();     // опрос ИК пульта
#endif
  mainLoop();       // главный цикл обработки и отрисовки
  eepromTick();     // проверка не пора ли сохранить настройки
}

void mainLoop() {
  // главный цикл отрисовки
  if (ONstate) {
    if (millis() - main_timer > MAIN_LOOP) {
      // сбрасываем значения
      RsoundLevel = 0;
      LsoundLevel = 0;
      rMinLevel=1023;
      rMaxLevel=0;


      analyzeAudio();
      // перваые два режима - громкость (VU meter)
      if (this_mode == 0 || this_mode == 1) {
        // заливаем "подложку", если яркость достаточная
        if (EMPTY_BRIGHT > 5) {
          for (int i = 0; i < NUM_LEDS; i++)
            leds[i] = CHSV(EMPTY_COLOR, 255, EMPTY_BRIGHT);
        }

        // преобразуем сигнал в длину ленты (где MAX_CH это половина количества светодиодов)
        Rlenght = map(RsoundLevel_f, minLevel_f, maxLevel, 0, MAX_CH);
        Llenght = map(LsoundLevel_f, minLevel_f, maxLevel, 0, MAX_CH);


        // ограничиваем до макс. числа светодиодов
        Rlenght = constrain(Rlenght, 0, MAX_CH);
        Llenght = constrain(Llenght, 0, MAX_CH);

        animation();       // отрисовать

      }

      // 3-5 режим - цветомузыка
      if (this_mode == 2 || this_mode == 3 || this_mode == 4 || this_mode == 7 || this_mode == 8) {
        colorMusic[0] = 0;
        colorMusic[1] = 0;
        colorMusic[2] = 0;
        toneColorCount[0]=0;
        toneColorCount[1]=0;
        toneColorCount[2]=0;

        for (int i = 0 ; i < 32 ; i++) {
          if (fht_log_out[i] < SPEKTR_LOW_PASS) fht_log_out[i] = 0;
        }
        // низкие частоты, выборка со 2 по 5 тон (0 и 1 зашумленные!)
        for (byte i = 2; i < 6; i++) {
         // if (fht_log_out[i] > colorMusic[0]) colorMusic[0] = fht_log_out[i];
          colorMusic[0]+=fht_log_out[i];
        }
        // средние частоты, выборка с 6 по 10 тон
        for (byte i = 6; i < 11; i++) {
          //if (fht_log_out[i] > colorMusic[1]) colorMusic[1] = fht_log_out[i];
          colorMusic[1]+=fht_log_out[i];
        }
        // высокие частоты, выборка с 11 по 31 тон
        for (byte i = 11; i < 32; i++) {
          //if (fht_log_out[i] > colorMusic[2]) colorMusic[2] = fht_log_out[i];
          colorMusic[2]+=fht_log_out[i];
        }
        
        freq_max = 0;
        for (byte i = 0; i < 30; i++) {
          if (fht_log_out[i + 2] > freq_max) freq_max = fht_log_out[i + 2];
          if (freq_max < 5) freq_max = 5;

          if (freq_f[i] < fht_log_out[i + 2]) freq_f[i] = fht_log_out[i + 2];
          if (freq_f[i] > 0) freq_f[i] -= LIGHT_SMOOTH;
          else freq_f[i] = 0;
        }
        freq_max_f = freq_max * averKFreg + freq_max_f * (1 - averKFreg);
        for (byte i = 0; i < 3; i++) {

          if(i==0){
            colorMusic_aver[i] = colorMusic[i] * averKFreg0 + colorMusic_aver[i] * (1 - averKFreg0);  // общая фильтрация
          }
          else{
            colorMusic_aver[i] = colorMusic[i] * averKFreg + colorMusic_aver[i] * (1 - averKFreg);  // общая фильтрация
          }
          colorMusic_f[i] = colorMusic[i] * SMOOTH_FREQ + colorMusic_f[i] * (1 - SMOOTH_FREQ);      // локальная
          if  ( (colorMusic_f[i] > ((float)colorMusic_aver[i] * MAX_COEF_FREQ)) || (i==0 && (colorMusic_f[i] > ((float)colorMusic_aver[i] * MAX_COEF_FREQ0))  )  ) {
            thisBright[i] = 255;
            colorMusicFlash[i] = true;
            running_flag[i] = true;
          } else colorMusicFlash[i] = false;

          
          if (thisBright[i] >= 0) thisBright[i] -= SMOOTH_STEP;
          if (thisBright[i] < EMPTY_BRIGHT) {
            thisBright[i] = EMPTY_BRIGHT;
            running_flag[i] = false;
          }
          /*
          Serial.println(F("  ")); 
          Serial.print(i);Serial.print(F(": colorMusic_f: "));   Serial.print(colorMusic_f[i]);
          Serial.print(F(": colorMusic_aver: "));   Serial.print(colorMusic_aver[i]);
          Serial.print(F(": thisBright: "));   Serial.print(thisBright[i]);
          */
        }

        //Serial.println(F("  ")); 
        animation();
      }
      if (this_mode == 5) {
        if ((long)millis() - strobe_timer > STROBE_PERIOD) {
          strobe_timer = millis();
          strobeUp_flag = true;
          strobeDwn_flag = false;
        }
        
        
        if ((long)millis() - strobe_timer > light_time) {
          strobeDwn_flag = true;
        }
        if (strobeUp_flag) {                    // если настало время пыхнуть
          if (strobe_bright < 255)              // если яркость не максимальная
            strobe_bright += STROBE_SMOOTH;     // увелчить
          if (strobe_bright > 255) {            // если пробили макс. яркость
            strobe_bright = 255;                // оставить максимум
            strobeUp_flag = false;              // флаг опустить
          }
        }

        if (strobeDwn_flag) {                   // гаснем
          if (strobe_bright > 0)                // если яркость не минимальная
            strobe_bright -= STROBE_SMOOTH;     // уменьшить
          if (strobe_bright < 0) {              // если пробили мин. яркость
            strobeDwn_flag = false;
            strobe_bright = 0;                  // оставить 0
          }
        }

/*
        Serial.println(F(" "));
        Serial.print(F("strobeUp_flag: "));Serial.print(strobeUp_flag);
        Serial.print(F("    trobeDwn_flag: "));Serial.print(strobeDwn_flag);
        Serial.print(F("    strobe_bright: "));Serial.print(strobe_bright);

*/        
        animation();
      }
      if (this_mode == 6) animation();

      if (!IRLremote.receiving())    // если на ИК приёмник не приходит сигнал (без этого НЕ РАБОТАЕТ!)
        FastLED.show();         // отправить значения на ленту

      if (this_mode != 7)       // 7 режиму не нужна очистка!!!
        FastLED.clear();          // очистить массив пикселей
      main_timer = millis();    // сбросить таймер
    }
  }
}

void animation() {
  // согласно режиму
  switch (this_mode) {
    case 0:
      count = 0;
      for (int i = (MAX_CH - 1); i > ((MAX_CH - 1) - Rlenght); i--) {
        leds[i] = ColorFromPalette(myPal, (count * index));   // заливка по палитре " от зелёного к красному"
        count++;
      }
      count = 0;
      for (int i = (MAX_CH); i < (MAX_CH + Llenght); i++ ) {
        leds[i] = ColorFromPalette(myPal, (count * index));   // заливка по палитре " от зелёного к красному"
        count++;
      }
      if (EMPTY_BRIGHT > 0) {
        CHSV this_dark = CHSV(EMPTY_COLOR, 255, EMPTY_BRIGHT);
        for (int i = ((MAX_CH - 1) - Rlenght); i > 0; i--)
          leds[i] = this_dark;
        for (int i = MAX_CH + Llenght; i < NUM_LEDS; i++)
          leds[i] = this_dark;
      }
      break;
    case 1:
      if (millis() - rainbow_timer > 30) {
        rainbow_timer = millis();
        hue = floor((float)hue + RAINBOW_STEP);
      }
      count = 0;
      for (int i = (MAX_CH - 1); i > ((MAX_CH - 1) - Rlenght); i--) {
        leds[i] = ColorFromPalette(RainbowColors_p, (count * index) / 2 - hue);  // заливка по палитре радуга
        count++;
      }
      count = 0;
      for (int i = (MAX_CH); i < (MAX_CH + Llenght); i++ ) {
        leds[i] = ColorFromPalette(RainbowColors_p, (count * index) / 2 - hue); // заливка по палитре радуга
        count++;
      }
      if (EMPTY_BRIGHT > 0) {
        CHSV this_dark = CHSV(EMPTY_COLOR, 255, EMPTY_BRIGHT);
        for (int i = ((MAX_CH - 1) - Rlenght); i > 0; i--)
          leds[i] = this_dark;
        for (int i = MAX_CH + Llenght; i < NUM_LEDS; i++)
          leds[i] = this_dark;
      }
      break;
    case 2:
      for (int i = 0; i < NUM_LEDS; i++) {
        if (i < STRIPE)          leds[i] = CHSV(HIGH_COLOR, 255, thisBright[2]);
        else if (i < STRIPE * 2) leds[i] = CHSV(MID_COLOR, 255, thisBright[1]);
        else if (i < STRIPE * 3) leds[i] = CHSV(LOW_COLOR, 255, thisBright[0]);
        else if (i < STRIPE * 4) leds[i] = CHSV(MID_COLOR, 255, thisBright[1]);
        else if (i < STRIPE * 5) leds[i] = CHSV(HIGH_COLOR, 255, thisBright[2]);
      }
      break;
    case 3:
      for (int i = 0; i < NUM_LEDS; i++) {
        if (i < NUM_LEDS / 3)          leds[i] = CHSV(HIGH_COLOR, 255, thisBright[2]);
        else if (i < NUM_LEDS * 2 / 3) leds[i] = CHSV(MID_COLOR, 255, thisBright[1]);
        else if (i < NUM_LEDS)         leds[i] = CHSV(LOW_COLOR, 255, thisBright[0]);
      }
      break;
    case 4:
    
      if (colorMusicFlash[2]) HIGHS();
      else if (colorMusicFlash[1]) MIDS();
      else if (colorMusicFlash[0]) LOWS();
      else SILENCE();
      break;
      
    case 5:
      if (strobe_bright > 0)
        for (int i = 0; i < NUM_LEDS; i++) leds[i] = CHSV(STROBE_COLOR, STROBE_SAT, strobe_bright);
      else
        for (int i = 0; i < NUM_LEDS; i++) leds[i] = CHSV(EMPTY_COLOR, 255, EMPTY_BRIGHT);
      break;
    case 6:
       for (int i = 0; i < NUM_LEDS; i++) leds[i] = CHSV(HUE_YELLOW, 0,BRIGHTNESS);
      break;
    case 7:

        if (running_flag[2]) leds[NUM_LEDS / 2] = CHSV(HIGH_COLOR, 255, thisBright[2]);
        else if (running_flag[1]) leds[NUM_LEDS / 2] = CHSV(MID_COLOR, 255, thisBright[1]);
        else if (running_flag[0]) leds[NUM_LEDS / 2] = CHSV(LOW_COLOR, 255, thisBright[0]);
        else leds[NUM_LEDS / 2] = CHSV(EMPTY_COLOR, 255, EMPTY_BRIGHT);
     
      leds[(NUM_LEDS / 2) - 1] = leds[NUM_LEDS / 2];
      if (millis() - running_timer > RUNNING_SPEED) {
        running_timer = millis();
        for (int i = 0; i < NUM_LEDS / 2 - 1; i++) {
          leds[i] = leds[i + 1];
          leds[NUM_LEDS - i - 1] = leds[i];
        }
      }
      break;
    case 8:
      byte HUEindex = HUE_START;
      for (int i = 0; i < NUM_LEDS / 2; i++) {
        byte this_bright = map(freq_f[(int)floor((NUM_LEDS / 2 - i) / freq_to_stripe)], 0, freq_max_f, 0, BRIGHTNESS);
        this_bright = constrain(this_bright, 0, 255);
        leds[i] = CHSV(HUEindex, 255, this_bright);
        leds[NUM_LEDS - i - 1] = leds[i];
        HUEindex += HUE_STEP;
        if (HUEindex > 255) HUEindex = 0;
      }
      break;
  }
}

void HIGHS() {
  for (int i = 0; i < NUM_LEDS; i++) leds[i] = CHSV(HIGH_COLOR, 255, thisBright[2]);
}
void MIDS() {
  for (int i = 0; i < NUM_LEDS; i++) leds[i] = CHSV(MID_COLOR, 255, thisBright[1]);
}
void LOWS() {
  for (int i = 0; i < NUM_LEDS; i++) leds[i] = CHSV(LOW_COLOR, 255, thisBright[0]);
}
void SILENCE() {
  for (int i = 0; i < NUM_LEDS; i++) leds[i] = CHSV(EMPTY_COLOR, 255, EMPTY_BRIGHT);
}

// вспомогательная функция, изменяет величину value на шаг incr в пределах minimum.. maximum
int smartIncr(int value, int incr_step, int mininmum, int maximum) {
  int val_buf = value + incr_step;
  val_buf = constrain(val_buf, mininmum, maximum);
  return val_buf;
}

float smartIncrFloat(float value, float incr_step, float mininmum, float maximum) {
  float val_buf = value + incr_step;
  val_buf = constrain(val_buf, mininmum, maximum);
  return val_buf;
}

#if REMOTE_TYPE != 0
void remoteTick() {
  if (IRLremote.available())  {
    auto data = IRLremote.read();
    IRdata = data.command;
    ir_flag = true;
  }
  if (ir_flag) { // если данные пришли
    eeprom_timer = millis();
    eeprom_flag = true;
    switch (IRdata) {
      // режимы
      case BUTT_1: this_mode = 0;
        break;
      case BUTT_2: this_mode = 1;
        break;
      case BUTT_3: this_mode = 2;
        break;
      case BUTT_4: this_mode = 3;
        break;
      case BUTT_5: this_mode = 4;
        break;
      case BUTT_6: this_mode = 5;
        break;
      case BUTT_7: this_mode = 6;
        break;
      case BUTT_8: this_mode = 7;
        break;
      case BUTT_9: this_mode = 8;
        break;
      case BUTT_0: fullLowPass();
        break;
      case BUTT_STAR: ONstate = !ONstate; FastLED.clear(); FastLED.show(); updateEEPROM();
        break;
      case BUTT_HASH:

      
        break;
      case BUTT_OK: digitalWrite(MLED_PIN, settings_mode ^ MLED_ON); settings_mode = !settings_mode;
        break;
      case BUTT_UP:
        BRIGHTNESS = smartIncr(BRIGHTNESS, 10, 0, 255);
        setBrightness();
        break;
      case BUTT_DOWN:
        BRIGHTNESS = smartIncr(BRIGHTNESS, -10, 0, 255);
        setBrightness();
        break;
      case BUTT_LEFT:
       
        break;
      case BUTT_RIGHT:
        
        break;
      default: eeprom_flag = false;   // если не распознали кнопку, не обновляем настройки!
        break;
    }
    ir_flag = false;
  }
}
#endif

void autoLowPass() {

  return ;
  // для режима VU
  delay(10);                                // ждём инициализации АЦП
  int thisMax = 0;                          // максимум
  int thisLevel;
  for (byte i = 0; i < 200; i++) {
    thisLevel = analogRead(SOUND_R);        // делаем 200 измерений
    if (thisLevel > thisMax)                // ищем максимумы
      thisMax = thisLevel;                  // запоминаем
    delay(4);                               // ждём 4мс
  }
  LOW_PASS = thisMax + LOW_PASS_ADD;        // нижний порог как максимум тишины + некая величина

  // для режима спектра
  thisMax = 0;
  for (byte i = 0; i < 100; i++) {          // делаем 100 измерений
    analyzeAudio();                         // разбить в спектр
    for (byte j = 2; j < 32; j++) {         // первые 2 канала - хлам
      thisLevel = fht_log_out[j];
      if (thisLevel > thisMax)              // ищем максимумы
        thisMax = thisLevel;                // запоминаем
    }
    delay(4);                               // ждём 4мс
  }
  SPEKTR_LOW_PASS = thisMax + LOW_PASS_FREQ_ADD;  // нижний порог как максимум тишины
  if (EEPROM_LOW_PASS && !AUTO_LOW_PASS) {
    EEPROM.updateInt(70, LOW_PASS);
    EEPROM.updateInt(72, SPEKTR_LOW_PASS);
  }
}

void analyzeAudio() {

  RsoundLevel = 0;
  LsoundLevel = 0;
  rMinLevel=1023;
  rMaxLevel=0;
  
  for (int i = 0 ; i < FHT_N ; i++) {
    RcurrentLevel = analogRead(SOUND_R);                            // с правого
    RsoundLevel += RcurrentLevel;
    if (rMinLevel > RcurrentLevel) rMinLevel = RcurrentLevel; // ищем максимальное
    if (rMaxLevel < RcurrentLevel) rMaxLevel = RcurrentLevel; // ищем максимальное
    fht_input[i] = RcurrentLevel; // put real data into bins
  }

   //Serial.println(F("  "));
  RsoundLevel =rMaxLevel;

  //Serial.print(F("RsoundLevel1: ")); Serial.print(RsoundLevel);
  //Serial.print(F("  rMinLevel: ")); Serial.print(rMinLevel);


  avgMinRealLevel = (float)rMinLevel  * averK + avgMinRealLevel * (1 - averK);
  avgMaxRealLevel = (float)rMaxLevel  * averK + avgMaxRealLevel * (1 - averK);
  //Serial.print(F("  avgMinRealLevel: ")); Serial.print(avgMinRealLevel);
  //Serial.print(F("  avgMaxRealLevel: ")); Serial.print(avgMaxRealLevel);


  // фильтруем по нижнему порогу шумов
  // ограничиваем диапазон
  RsoundLevel = pow(RsoundLevel, EXP);
  rMinLevel= pow(rMinLevel, EXP);
  //Serial.print(F("  RsoundLevelPow: ")); Serial.print(RsoundLevel);
  //Serial.print(F("  rMinLevelPow: ")); Serial.print(rMinLevel);
  
  // фильтр
  RsoundLevel_f = RsoundLevel * SMOOTH + RsoundLevel_f * (1 - SMOOTH);
  //Serial.print(F("  RsoundLevel_f: ")); Serial.print(RsoundLevel_f);

  minLevel_f = rMinLevel * averK + minLevel_f * (1 - averK);
  //Serial.print(F("  minLevel_f: ")); Serial.print(minLevel_f);

  
  LsoundLevel_f = RsoundLevel_f;  // если моно, то левый = правому

  // расчёт общей средней громкости с обоих каналов, фильтрация.
  // Фильтр очень медленный, сделано специально для автогромкости
  if (averageLevel<0){
    averageLevel=RsoundLevel_f;
  }
  
  averageLevel = (float)RsoundLevel_f  * averK + averageLevel * (1 - averK);

  // принимаем максимальную громкость шкалы как среднюю, умноженную на некоторый коэффициент MAX_COEF
  maxLevel = (float)averageLevel * MAX_COEF;

  if (minLevel_f>maxLevel) minLevel_f=maxLevel-1;
  //Serial.print(F("  maxLevel: ")); Serial.print(maxLevel);


  if (this_mode == 2 || this_mode == 3 || this_mode == 4 || this_mode == 7 || this_mode == 8) {

        
        float mult=1/(avgMaxRealLevel-avgMinRealLevel)*1000;
        
        //Serial.println(F("  "));
        for (int i = 0 ; i < FHT_N ; i++) {
          fht_input[i] = (fht_input[i]-avgMinRealLevel)*mult;
          //Serial.print(F("  ")); Serial.print(fht_input[i]);
        }
        
        
        
        fht_window();  // window the data for better frequency response
        fht_reorder(); // reorder the data before doing the fht
        fht_run();     // process the data in the fht
        fht_mag_log(); // take the output of the fht
        //Serial.println(F("  "));
        avgFht=0;
        toneCount=0;

        for (int i = 2 ; i < 32 ; i++) {
          if (fht_log_out[i]>8){
              toneCount++;
              avgFht+=fht_log_out[i];
            }
          
          //Serial.print(fht_log_out[i]);Serial.print(F("  "));
        }
/*
        Serial.println(F("  "));
        sigma=0;

        if (toneCount>0){
          avgFht/=toneCount;
          
          for (int i = 2 ; i < 32 ; i++) {
              sigma+=(fht_log_out[i]-avgFht)*(fht_log_out[i]-avgFht);
          }
          sigma=sqrt(sigma/toneCount);

          //Serial.print(F("  avgFht: ")); Serial.print(avgFht);
          //Serial.print(F("  sigma: ")); Serial.print(sigma);
       }
       else {
          sigma=100000;  
      }
  */      
  }

/*
  Serial.println(F(" "));
  Serial.println(F(" "));
  */
}

void buttonTick() {
  butt1.tick();  // обязательная функция отработки. Должна постоянно опрашиваться
  if (butt1.isSingle())                              // если единичное нажатие
    if (++this_mode >= MODE_AMOUNT) this_mode = 0;   // изменить режим

  if (butt1.isHolded()) {     // кнопка удержана
    fullLowPass();
  }
}
void fullLowPass() {
  digitalWrite(MLED_PIN, MLED_ON);   // включить светодиод
  FastLED.setBrightness(0); // погасить ленту
  FastLED.clear();          // очистить массив пикселей
  FastLED.show();           // отправить значения на ленту
  delay(500);               // подождать чутка
  autoLowPass();            // измерить шумы
  delay(500);               // подождать
  FastLED.setBrightness(BRIGHTNESS);  // вернуть яркость
  digitalWrite(MLED_PIN, !MLED_ON);    // выключить светодиод
}
void updateEEPROM() {
  EEPROM.updateByte(1, this_mode);
  EEPROM.updateByte(1, BRIGHTNESS);

}
void readEEPROM() {
  this_mode = EEPROM.readByte(1);
  BRIGHTNESS= EEPROM.readByte(2);

  setBrightness();
  

}
void eepromTick() {
  if (eeprom_flag)
    if (millis() - eeprom_timer > 30000) {  // 30 секунд после последнего нажатия с пульта
      eeprom_flag = false;
      eeprom_timer = millis();
      updateEEPROM();
    }
}


void setBrightness(){
    FastLED.setBrightness(BRIGHTNESS);
    SMOOTH_STEP=BRIGHTNESS/10;
    EMPTY_BRIGHT=SMOOTH_STEP;
    
  }
