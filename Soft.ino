#include "U8glib.h"
#include <SD.h>
//#include <string.h>
//#include <stdlib.h>
#include "podstawa.h"
#include "lewy_dolny.h"
#include "lewy_gorny.h"
#include "prawy_dolny.h"
#include "prawy_gorny.h"
#include "menu_v2.h"
#include "Motor.h"

#define encoderPin1 31
#define encoderPin2 33
#define pinEncButt 35

#define extruder_grzalka_pin 8
#define extruder_termistor_pin 13

#define extruder_fan_pin 10

#define stol_termistor_pin 14
#define stol_grzalka_pin 9

#define CS_pin 53

//przyspieszenie
#define op_max 800
#define op_min 100
#define op_krok 10

Motor motor[4] = {Motor(A0, A1, 38, 3, 160, 0, 1), Motor(A6, A7, A2, 14, 160, 0, 1), Motor(46, 48, A8, 18, 800, 0, 0), Motor(26, 28, 24, 0, 196, 0, 0)};

float poz_X = 84.348;
float poz_Y = 96.533;
float poz_Z = 160;
float poz_E = 0;

float N_poz_X = 160;
float N_poz_Y = 160;
float N_poz_Z = 160;
float N_poz_E = 160;

int last;

long feedrate;

int abs_rel = 0; //0=absolute positioning; 1=relative positioning;
/*//////////////////////////////////////////////////////////////////////////////
  silnik[0]=>oś X
  silnik[1]=>oś Y
  silnik[2]=>oś Z
  silnik[3]=>oś E
  ////////////////////////////////////////////////////////////////////////////*/

float encoderValue = 100;
volatile int lastEncoded;
float menu = 100;
float menu_manual = 25;
float inkrement_manual = 0;
int opcje_manual = 0;
int flaga_manual = 1;


char *nazwy_plikow[] = {"----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------", "----------"};
char *menu_final[] = {"temp.sto.", "temp.ext.", "manual", "plik", "drukuj"};
char *menu_manual_opcje[] = {"X", "Y", "Z", "E", "Home", "Powrot", "blad"};
char *wybrany_plik = "-----";

char Buffer[64];
int nr_linijki;

long lastencoderValue;
int lastMSB = 0;
int lastLSB = 0;

float T_ext_zad = 25;
float T_ext_real;
float T_stol_zad = 25;
float T_stol_real = 80;

int opcje = 0;
int flaga = 1;
int flaga2 = 0;
int opoznienie = op_max;
int opoznienie_wl = op_max;

float x = 90.5;
float y = 50.88;
float z = 40.69;

float I_ext = 0;
float I_stol = 0;
int j = 0;

long recNum = 0;
long kolejna_linijka = 1;

File root;

U8GLIB_ST7920_128X64_1X u8g(23, 17, 16);

void loop(void)
{
  u8g.firstPage();
  do
  {
    if (digitalRead(pinEncButt) == 1 and flaga == 0) {
      flaga = 1;
    }
    draw();
    reg_T_ext();
    reg_T_stol();

    switch (opcje) {
      case 0:
        draw_menu_glowne();

        menu = updateEncoder(menu, 0.5);

        menu_manual = 0;
        N_poz_X = poz_X;
        N_poz_Y = poz_Y;
        N_poz_Z = poz_Z;
        N_poz_E = poz_E;

        if (digitalRead(pinEncButt) == 0 and flaga == 1) {
          opcje = (((int(menu) / 2) + 1) % 5 + 1);
          flaga = 0;
        }
        break;
      case 1:
        draw_T_stol_zad();
        T_stol_zad = updateEncoder(T_stol_zad, 0.5);
        powrot();
        break;
      case 2:
        draw_T_ext_zad();
        T_ext_zad = updateEncoder(T_ext_zad, 0.5);
        powrot();
        break;
      case 3:
        manual();
        break;
      case 4:
        draw_pliki();
        menu = updateEncoder(menu, 0.5);
        if (digitalRead(pinEncButt) == 0) {
          wybrany_plik = nazwy_plikow[(((int(menu) / 2) + 1) % 4) + 5];
        }
        powrot();
        break;
      case 5:
        flaga2 = 0;
        Drukowanie();
        break;
    }
  } while (u8g.nextPage());
}

void dokladnosc() {

  draw_dokladnosc();
  inkrement_manual = updateEncoder(inkrement_manual, 0.1);

  if (digitalRead(pinEncButt) == 0 and flaga == 1) {
    flaga_manual = 0;
    flaga = 0;
  }

}

void Interpreter(char linijka[]) {
  int nr_trybu;
  if (linijka[0] == 'G') {

    nr_trybu = tryb('G', -1, linijka);

    switch (nr_trybu) {

      case 0:
        //feedrate = tryb('F', feedrate, linijka);
        if (abs_rel == 0) {
          linia(tryb('X', poz_X, linijka), tryb('Y', poz_Y, linijka), tryb('Z', poz_Z, linijka), tryb('E', poz_E, linijka));
        } else {
          linia(tryb('X', 0, linijka), tryb('Y', 0, linijka), tryb('Z', 0, linijka), tryb('E', 0, linijka));
        }
        break;

      case 1:
        //feedrate = tryb('F', feedrate, linijka);
        if (abs_rel == 0) {
          linia(tryb('X', poz_X, linijka), tryb('Y', poz_Y, linijka), tryb('Z', poz_Z, linijka), tryb('E', poz_E, linijka));
        } else {
          linia(tryb('X', 0, linijka), tryb('Y', 0, linijka), tryb('Z', 0, linijka), tryb('E', 0, linijka));
        }
        break;

      case 28:
        poz_X = motor[0].home();
        poz_Y = motor[1].home();
        poz_Z = motor[2].home();
        break;

      case 90:
        abs_rel = 0;
        break;

      case 91:
        abs_rel = 1;
        break;

      case 92:
        poz_X = tryb('X', poz_X, linijka);
        poz_Y = tryb('Y', poz_Y, linijka);
        poz_Z = tryb('Z', poz_Z, linijka);
        poz_E = tryb('E', poz_E, linijka);

        break;

      default:
        break;
    }

  } else if (linijka[0] == 'M') {
    nr_trybu = tryb('M', -1, linijka);

    switch (nr_trybu) {

      case 104:
        T_ext_zad = tryb('S', -1, linijka);
        break;

      case 106:
        analogWrite(extruder_fan_pin, 255);
        delay(1500);
        analogWrite(extruder_fan_pin, int(tryb('S', 0, linijka)));
        break;

      case 109:
        while (abs(T_ext_real - T_ext_zad) > 5) {
          reg_T_ext();
          reg_T_stol();
          u8g.firstPage();
          do
          {
            draw_menu_glowne();
          } while (u8g.nextPage());
        }
        break;

      case 140:
        T_stol_zad = tryb('S', -1, linijka);
        break;

      case 190:
        while (abs(T_stol_real - T_stol_zad) > 5) {
          reg_T_stol();
          reg_T_ext();
          u8g.firstPage();
          do
          {
            draw_menu_glowne();
          } while (u8g.nextPage());
        }
        break;


      default:
        break;

    }

  } else if (linijka[1] == 'E' and linijka[2] == 'n' and linijka[3] == 'd') {
    opcje = 0;
    flaga2 = 1;
  }
}

void linia(float NN_poz_X, float NN_poz_Y, float NN_poz_Z, float NN_poz_E) {

  if (abs_rel == 0) {
    motor[0].delta = abs(round((poz_X - NN_poz_X) * motor[0].step_resolution));
    motor[0].dir = (poz_X > NN_poz_X) ? HIGH : LOW;
    digitalWrite(motor[0].dir_pin, motor[0].dir);
    motor[1].delta = abs(round((poz_Y - NN_poz_Y) * motor[1].step_resolution));
    motor[1].dir = (poz_Y > NN_poz_Y) ? HIGH : LOW;
    digitalWrite(motor[1].dir_pin, motor[1].dir);
    motor[2].delta = abs(round((poz_Z - NN_poz_Z) * motor[2].step_resolution));
    motor[2].dir = (poz_Z > NN_poz_Z) ? LOW : HIGH;
    digitalWrite(motor[2].dir_pin, motor[2].dir);
    motor[3].delta = abs(round((poz_E - NN_poz_E) * motor[3].step_resolution));
    motor[3].dir = (poz_E > NN_poz_E) ? LOW : HIGH;
    digitalWrite(motor[3].dir_pin, motor[3].dir);
  } else {
    motor[0].delta = abs(round(NN_poz_X * motor[0].step_resolution));
    motor[0].dir = (0 > NN_poz_X) ? HIGH : LOW;
    digitalWrite(motor[0].dir_pin, motor[0].dir);
    motor[1].delta = abs(round(NN_poz_Y * motor[1].step_resolution));
    motor[1].dir = (0 > NN_poz_Y) ? HIGH : LOW;
    digitalWrite(motor[1].dir_pin, motor[1].dir);
    motor[2].delta = abs(round(NN_poz_Z * motor[2].step_resolution));
    motor[2].dir = (0 > NN_poz_Z) ? LOW : HIGH;
    digitalWrite(motor[2].dir_pin, motor[2].dir);
    motor[3].delta = abs(round(NN_poz_E * motor[3].step_resolution));
    motor[3].dir = (0 > NN_poz_E) ? LOW : HIGH;
    digitalWrite(motor[3].dir_pin, motor[3].dir);
  }

  int Max = max(max(motor[0].delta, motor[1].delta), max(motor[2].delta, motor[3].delta));
  int i = Max;
  int Xpom = Max / 2;
  int Ypom = Max / 2;
  int Zpom = Max / 2;
  int Epom = Max / 2;
  int op_pom = Max / 2;

  for (;;) {
    if (i-- == 0) break;

    /*
        if (i > op_pom) {
          opoznienie -= op_krok;
        } else {
          opoznienie += op_krok;
        }
        opoznienie_wl = (opoznienie < op_min) ? op_min : opoznienie;
    */
    if (i > op_pom) {
      opoznienie = opoznienie - op_krok;
    } else {
      opoznienie = opoznienie + op_krok;
    }

    if (opoznienie < op_min) {
      opoznienie_wl = op_min;
    } else {
      opoznienie_wl = opoznienie;
    }

    //Serial.println(opoznienie_wl, DEC);

    Xpom -= motor[0].delta;
    if (Xpom < 0) {
      Xpom += Max;
      //motor[0].make_step(opoznienie_wl);
      motor[0].krok = 1;
    }

    Ypom -= motor[1].delta;
    if (Ypom < 0 ) {
      Ypom += Max;
      //motor[1].make_step(opoznienie_wl);
      motor[1].krok = 1;
    }

    Zpom -= motor[2].delta;
    if (Zpom < 0 ) {
      Zpom += Max;
      //motor[2].make_step(opoznienie_wl);
      motor[2].krok = 1;
    }

    Epom -= motor[3].delta;
    if (Epom < 0) {
      Epom += Max;
      //motor[3].make_step(opoznienie_wl  );
      motor[3].krok = 1;
    }

    (motor[0].krok == 1) ? digitalWrite(motor[0].step_pin, HIGH) : digitalWrite(motor[0].step_pin, LOW);
    (motor[1].krok == 1) ? digitalWrite(motor[1].step_pin, HIGH) : digitalWrite(motor[1].step_pin, LOW);
    (motor[2].krok == 1) ? digitalWrite(motor[2].step_pin, HIGH) : digitalWrite(motor[2].step_pin, LOW);
    (motor[3].krok == 1) ? digitalWrite(motor[3].step_pin, HIGH) : digitalWrite(motor[3].step_pin, LOW);

    delayMicroseconds(opoznienie_wl);

    digitalWrite(motor[0].step_pin, LOW);
    digitalWrite(motor[1].step_pin, LOW);
    digitalWrite(motor[2].step_pin, LOW);
    digitalWrite(motor[3].step_pin, LOW);

    delayMicroseconds(opoznienie_wl);

    motor[0].krok = 0;
    motor[1].krok = 0;
    motor[2].krok = 0;
    motor[3].krok = 0;

  }

  opoznienie = op_max;

  if (abs_rel == 0) {
    poz_X = NN_poz_X;
    poz_Y = NN_poz_Y;
    poz_Z = NN_poz_Z;
    poz_E = NN_poz_E;
  } else {
    poz_X = poz_X + NN_poz_X;
    poz_Y = poz_Y + NN_poz_Y;
    poz_Z = poz_Z + NN_poz_Z;
    poz_E = poz_E + NN_poz_E;
  }


  /*
    if (abs_rel == 0) {

      if (poz_X < NN_poz_X) {
        poz_X = (poz_X + (motor[0].delta / motor[0].step_resolution));
      } else {
        poz_X = (poz_X - (motor[0].delta / motor[0].step_resolution));
      }

      if (poz_Y < NN_poz_Y) {
        poz_Y = (poz_Y + (motor[1].delta / motor[1].step_resolution));
      } else {
        poz_Y = (poz_Y - (motor[1].delta / motor[1].step_resolution));
      }

      if (poz_Z < NN_poz_Z) {
        poz_Z = (poz_Z + (motor[2].delta / motor[2].step_resolution));
      } else {
        poz_Z = (poz_Z - (motor[2].delta / motor[2].step_resolution));
      }

      if (poz_E < NN_poz_E) {
        poz_E = (poz_E + (motor[3].delta / motor[3].step_resolution));
      } else {
        poz_E = (poz_E - (motor[3].delta / motor[3].step_resolution));
      }

    } else {

      if (0 < NN_poz_X) {
        poz_X = (poz_X + (motor[0].delta / motor[0].step_resolution));
      } else {
        poz_X = (poz_X - (motor[0].delta / motor[0].step_resolution));
      }

      if (0 < NN_poz_Y) {
        poz_Y = (poz_Y + (motor[1].delta / motor[1].step_resolution));
      } else {
        poz_Y = (poz_Y - (motor[1].delta / motor[1].step_resolution));
      }

      if (0 < NN_poz_Z) {
        poz_Z = (poz_Z + (motor[2].delta / motor[2].step_resolution));
      } else {
        poz_Z = (poz_Z - (motor[2].delta / motor[2].step_resolution));
      }

      if (0 < NN_poz_E) {
        poz_E = (poz_E + (motor[3].delta / motor[3].step_resolution));
      } else {
        poz_E = (poz_E - (motor[3].delta / motor[3].step_resolution));
      }
  */

}


void Drukowanie() {
  File myFile = SD.open(wybrany_plik);
  while (myFile.available()) {

    reg_T_ext();
    reg_T_stol();
    //////////////////////////////////////////////////////////////////////////////////////////////
    //    u8g.firstPage();
    //    do
    //    {
    //      draw_menu_glowne();
    //    } while (u8g.nextPage());

    //    if (digitalRead(pinEncButt) == 0 and flaga == 1) {
    //      opcje = 0;
    //      flaga = 0;
    //      break;
    //    } else if (flaga2 == 1) {
    //      break;
    //    }
    //
    //    if (digitalRead(pinEncButt) == 1)flaga = 1;

    //draw_menu_glowne();
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    if (flaga2 == 1) {
      //myFile =SD.close(wybrany_plik);
      break;
    }

    String linijka = myFile.readStringUntil('\n');
    char linijka_ch[linijka.length()];
    linijka.toCharArray(linijka_ch, linijka.length());
    Interpreter(linijka_ch);
    //}
    //} while (u8g.nextPage() );
  }
}

float tryb(char litera, float zwrot, char linijka[]) {
  char *ptr = linijka;
  for (int i = 0; i < 5; i++) {
    if (*ptr == litera) {
      return atof(ptr + 1);
    } else {
      ptr = strchr(ptr, ' ') + 1;
    }
  }
  return zwrot;
}

void powrot(void) {
  if (digitalRead(pinEncButt) == 0 and flaga == 1) {
    opcje = 0;
    flaga = 0;
  }
}

void printDirectory(File dir, int numTabs)
{
  while (true)
  {
    j++;
    File entry = dir.openNextFile();
    if (! entry)
    {
      return;
    }
    Serial.print('\t');
    Serial.print(j, DEC);
    sprintf (nazwy_plikow[j], "%s", entry.name());
    Serial.print(entry.name());
    if (entry.isDirectory())
    {
      Serial.println("/");
      printDirectory(entry, numTabs + 1);
    }
    else
    {
      Serial.print("\t\t");
      Serial.println(entry.size(), DEC);
    }
    entry.close();
  }
}

void reg_T_ext(void) {
  float a = 0.7944142696e-03, b = 2.010564365e-04, c = 1.450471517e-07;
  float R1 = 4700;
  float Vo = analogRead(extruder_termistor_pin);
  float R2 = R1 / ((1023.0 / (float)Vo) - 1.0);
  T_ext_real = (1 / (a + (b * log(R2)) + (c * log(R2) * log(R2) * log(R2))));
  T_ext_real = T_ext_real - 273.15;
  int sygnal_sterujacy;
  float P_ext = (T_ext_zad - T_ext_real) * 30;
  I_ext = I_ext + (T_ext_zad - T_ext_real) * 0.005;
  if (I_ext < 0)I_ext = 0;
  if (I_ext > 100)I_ext = 100;
  if (P_ext < 0)P_ext = 0;

  if ((abs(T_ext_real - T_ext_zad)) < 5) {
    sygnal_sterujacy = I_ext + P_ext;
    if (sygnal_sterujacy > 255)sygnal_sterujacy = 255;
    analogWrite(extruder_grzalka_pin, sygnal_sterujacy);
  } else {
    if (T_ext_zad > T_ext_real)digitalWrite(extruder_grzalka_pin, HIGH);
    else digitalWrite(extruder_grzalka_pin, LOW);
  }
}

void reg_T_stol(void) {
  float a = 0.7944142696e-03, b = 2.010564365e-04, c = 1.450471517e-07;
  float R1 = 4700;
  float Vo = analogRead(stol_termistor_pin);
  float R2 = R1 / ((1023.0 / (float)Vo) - 1.0);
  T_stol_real = (1 / (a + (b * log(R2)) + (c * log(R2) * log(R2) * log(R2))));
  T_stol_real = T_stol_real - 273.15;
  int sygnal_sterujacy;
  float P_stol = (T_stol_zad - T_stol_real) * 30;
  I_stol = I_stol + (T_stol_zad - T_stol_real) * 0.005;
  if (I_stol < 0)I_stol = 0;
  if (I_ext > 100)I_ext = 100;
  if (P_stol < 0)P_stol = 0;

  if ((abs(T_stol_real - T_stol_zad)) < 5) {
    sygnal_sterujacy = I_stol + P_stol;
    if (sygnal_sterujacy > 255)sygnal_sterujacy = 255;
    analogWrite(stol_grzalka_pin, sygnal_sterujacy);
  } else {
    if (T_stol_zad > T_stol_real)digitalWrite(stol_grzalka_pin, HIGH);
    else digitalWrite(stol_grzalka_pin, LOW);
  }
}

float updateEncoder(float encoderValue, float inkrement) {
  int MSB = digitalRead(encoderPin1);
  int LSB = digitalRead(encoderPin2);

  int encoded = (MSB << 1) | LSB;
  int sum = (lastEncoded << 2) | encoded;

  if (sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011)
    encoderValue = encoderValue + inkrement;
  if (sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000)
    encoderValue = encoderValue - inkrement;

  lastEncoded = encoded;
  return encoderValue;
}

void odczyt_nazw(void) {
  SD.begin(CS_pin);
  root = SD.open("/");
  printDirectory(root, 0);
}

void manual(void) {

  switch (opcje_manual) {
    case 0:
      draw_menu_manual();
      flaga_manual = 1;
      menu_manual = updateEncoder(menu_manual, 0.5);
      if (menu_manual < 6) {
        menu_manual = 60;
      } else {
        if (menu_manual > 60) {
          menu_manual = 6;
        }
      }
      if (digitalRead(pinEncButt) == 0 and flaga == 1) {
        opcje_manual = (((int(menu_manual) + 1) % 6) + 1);
        flaga = 0;
      }
      break;

    case 1:
      if (flaga_manual == 1) {
        dokladnosc();
      }

      if (flaga_manual == 0) {
        N_poz_X = updateEncoder(N_poz_X, inkrement_manual);
        draw_sterowanie('X', N_poz_X);
        linia(N_poz_X, poz_Y, poz_Z, poz_E);

        if (digitalRead(pinEncButt) == 0 and flaga == 1) {
          opcje_manual = 0;
          flaga = 0;
        }
      }
      break;

    case 2:
      if (flaga_manual == 1) {
        dokladnosc();
      }

      if (flaga_manual == 0) {
        N_poz_Y = updateEncoder(N_poz_Y, inkrement_manual);
        draw_sterowanie('Y', N_poz_Y);
        linia(poz_X, N_poz_Y, poz_Z, poz_E);

        if (digitalRead(pinEncButt) == 0 and flaga == 1) {
          opcje_manual = 0;
          flaga = 0;
        }
      }
      break;

    case 3:
      if (flaga_manual == 1) {
        dokladnosc();
      }

      if (flaga_manual == 0) {
        N_poz_Z = updateEncoder(N_poz_Z, inkrement_manual);
        draw_sterowanie('Z', N_poz_Z);
        linia(poz_X, poz_Y, N_poz_Z, poz_E);

        if (digitalRead(pinEncButt) == 0 and flaga == 1) {
          opcje_manual = 0;
          flaga = 0;
        }
      }
      break;

    case 4:
      if (flaga_manual == 1) {
        dokladnosc();
      }

      if (flaga_manual == 0) {
        N_poz_E = updateEncoder(N_poz_E, inkrement_manual);
        draw_sterowanie('E', N_poz_E);
        linia(poz_X, poz_Y, poz_Z, N_poz_E);

        if (digitalRead(pinEncButt) == 0 and flaga == 1) {
          opcje_manual = 0;
          flaga = 0;
        }
      }
      break;

    case 5:
      poz_X = motor[0].home();
      poz_Y = motor[1].home();
      poz_Z = motor[2].home();
      N_poz_X = poz_X;
      N_poz_Y = poz_Y;
      N_poz_Z = poz_Z;
      N_poz_E = poz_E;
      opcje_manual = 0;
      break;

    case 6:
      opcje = 0;
      opcje_manual = 0;
      break;
  }
}

void draw(void)
{
  u8g.drawPixel(menu, 1);
  u8g.drawBitmapP(0, 56, 1, 8, lewy_dolny_h);
  u8g.drawBitmapP(120, 56, 1, 8, prawy_dolny_h);
  u8g.drawBitmapP(0, 0, 1, 8, lewy_gorny_h);
  u8g.drawBitmapP(120, 0, 1, 8, prawy_gorny_h);
}

void draw_manual(void) {
  u8g.drawBitmapP(8, 27, 14, 112, menu_v2_h);
  u8g.setFont(u8g_font_5x7);
  u8g.setPrintPos(12, 20);
  u8g.drawStr(2, 20, "X:");
  u8g.print(poz_X, 2);
  u8g.setPrintPos(56, 20);
  u8g.drawStr(46, 20, "Y:");
  u8g.print(poz_Y, 2);
  u8g.setPrintPos(100, 20);
  u8g.drawStr(90, 20, "Z:");
  u8g.print(poz_Z, 2);
}

void draw_menu_manual(void) {
  u8g.drawBitmapP(8, 27, 14, 112, menu_v2_h);
  u8g.setFont(u8g_font_5x7);
  u8g.drawStr(39, 36, menu_manual_opcje[((int(menu_manual) + 1) % 6)]);
  u8g.setFont(u8g_font_u8glib_4);
  u8g.drawStr(2, 44, menu_manual_opcje[((int(menu_manual) + 2) % 6)]);
  u8g.drawStr(92, 44, menu_manual_opcje[((int(menu_manual) + 0) % 6)]);

}

void draw_menu_glowne(void) {

  //char buf[32];

  u8g.drawBitmapP(8, 27, 14, 112, menu_v2_h);
  u8g.setFont(u8g_font_5x7);
  u8g.drawStr(39, 36, menu_final[((int(menu) / 2) + 1) % 5]);
  u8g.setFont(u8g_font_u8glib_4);
  u8g.drawStr(2, 44, menu_final[((int(menu) / 2) + 2) % 5]);
  u8g.drawStr(92, 44, menu_final[((int(menu) / 2) + 0) % 5]);

  u8g.setFont(u8g_font_u8glib_4);
  u8g.drawStr(3, 10, "temp.stol.:");
  u8g.setPrintPos(3, 20);
  u8g.print(T_stol_zad, 0);
  u8g.setPrintPos(23, 20);
  u8g.print(T_stol_real, 0);

  u8g.drawStr(90, 10, "temp.ext.:");
  u8g.setPrintPos(90, 20);
  u8g.print(T_ext_zad, 0);
  u8g.setPrintPos(110, 20);
  u8g.print(T_ext_real, 0);

  u8g.setFont(u8g_font_u8glib_4);
  u8g.drawStr(50, 10, "plik:");
  u8g.setPrintPos(40, 20);
  u8g.print(wybrany_plik);
}


void draw_T_ext_zad(void) {
  char buf[8];
  u8g.drawBitmapP(8, 27, 14, 112, menu_v2_h);

  u8g.setFont(u8g_font_5x7);
  u8g.drawStr(8, 16, "Temperatura extrudera");
  sprintf(buf, "%d", int(T_ext_zad));
  u8g.drawStr(39, 36, buf);

  u8g.setFont(u8g_font_u8glib_4);
  sprintf(buf, "%d", int(T_ext_zad + 1));
  u8g.drawStr(6, 44, buf);

  sprintf(buf, "%d", int(T_ext_zad - 1));
  u8g.drawStr(110, 44, buf);
}



void draw_T_stol_zad(void) {
  char buf[8];
  u8g.drawBitmapP(8, 27, 14, 112, menu_v2_h);

  u8g.setFont(u8g_font_5x7);
  u8g.drawStr(22, 16, "Temperatura stolu");
  sprintf(buf, "%d", int(T_stol_zad));
  u8g.drawStr(39, 36, buf);

  u8g.setFont(u8g_font_u8glib_4);
  sprintf(buf, "%d", int(T_stol_zad) + 1);
  u8g.drawStr(6, 44, buf);

  sprintf(buf, "%d", int(T_stol_zad) - 1);
  u8g.drawStr(110, 44, buf);
}

void draw_dokladnosc(void) {
  //char buf[8];
  u8g.drawBitmapP(8, 27, 14, 112, menu_v2_h);

  u8g.setFont(u8g_font_5x7);
  u8g.drawStr(8, 16, "Dokladnosc");
  u8g.setPrintPos(39, 36);
  u8g.print(inkrement_manual, 2);

  u8g.setFont(u8g_font_u8glib_4);
  u8g.setPrintPos(6, 44);
  u8g.print((inkrement_manual + 0.1), 2);

  u8g.setPrintPos(110, 44);
  u8g.print(inkrement_manual - 0.1, 2);
}

void draw_sterowanie(char os, float N_poz) {
  //char buf[8];
  u8g.drawBitmapP(8, 27, 14, 112, menu_v2_h);

  u8g.setFont(u8g_font_5x7);
  u8g.setPrintPos(38, 16);
  u8g.print(("%c:", os), 2);

  u8g.setPrintPos(39, 36);
  u8g.print(N_poz, 2);

  u8g.setFont(u8g_font_u8glib_4);
  u8g.setPrintPos(6, 44);
  u8g.print((N_poz + inkrement_manual), 2);

  u8g.setPrintPos(110, 44);
  u8g.print((N_poz - inkrement_manual), 2);
}

void draw_pliki(void) {
  u8g.drawBitmapP(8, 27, 14, 112, menu_v2_h);

  u8g.setFont(u8g_font_5x7);
  u8g.drawStr(37, 36, nazwy_plikow[(((int(menu) / 2) + 1) % 4) + 5]);
  u8g.setFont(u8g_font_u8glib_4);
  u8g.drawStr(2, 44, nazwy_plikow[(((int(menu) / 2) + 2) % 4) + 5]);
  u8g.drawStr(90, 44, nazwy_plikow[(((int(menu) / 2) + 0) % 4) + 5]);
}


void setup(void)
{
  Serial.begin(9600);
  while (!Serial) {
  }
  if (u8g.getMode() == U8G_MODE_R3G3B2)
  {
    u8g.setColorIndex(255);
  }
  else if (u8g.getMode() == U8G_MODE_GRAY2BIT)
  {
    u8g.setColorIndex(3);
  }
  else if (u8g.getMode() == U8G_MODE_BW)
  {
    u8g.setColorIndex(1);
  }
  else if (u8g.getMode() == U8G_MODE_HICOLOR)
  {
    u8g.setHiColorByRGB(255, 255, 255);
  }
  opcje = 0;
  pinMode(encoderPin1, INPUT);
  pinMode(encoderPin2, INPUT);
  pinMode(pinEncButt, INPUT);

  digitalWrite(encoderPin1, HIGH);
  digitalWrite(encoderPin2, HIGH);
  digitalWrite(pinEncButt, HIGH);

  attachInterrupt(0, updateEncoder, CHANGE);
  attachInterrupt(1, updateEncoder, CHANGE);

  odczyt_nazw();

}
