/* VERSION FINAL 16/12/2019 */


/******  HUERTO DOMOTICO "HUERTUINO" **********
  Susana Loreto Jiménez Casas
  José Manuel Ruiz García
  Enrique Brito Álvaro
  Diciembre 2019
***********************************************/

/***********************************************

  CODIGO CON LICENCIA CREATIVE COMMONS BY-CC-SA
  ATTRIBUTION, NON COMERCIAL, SHARE ALIKE

************************************************/

/*========================================*/

/******  BIBLIOTECAS     ******/

/*** BME280
  Sensor de presión, temperatura y humedad
  se comumica por I2C                ****/
#include "DFRobot_BME280.h"
#include "Wire.h"

/*** LiquidCristal 16x2 I2C ***/
/* #include <Wire.h>  Ya declarada */
#include <LiquidCrystal_I2C.h>

/*** Zumbador pasivo  **/
#include "pitches.h"

/*========================================*/


/***** CONSTANTES *****/
const boolean DEBUG = true;

/***** PINES DIGITALES *****/
const short PIN_BOMBA_RIEGO = 2;
const short PIN_BOMBA_DESAGUE = 3;
const short PIN_ZUMBADOR_PASIVO = 12;

//***** PINES ANALOGICOS ******/
const short PIN_LDR = 0;
const short PIN_SENSOR_HUMEDAD_TIERRA = 1;
const short PIN_SENSOR_LLUVIA = 2;
const short PIN_NIVEL_AGUA = 3;
const short PIN_I2C_SDA = 4;
const short PIN_I2C_SCL = 5;

/***** TEMPORIZADORES EN SEGUNDOS *****/
const int TIEMPO_RIEGO = 20; /* 60 */
const int TIEMPO_DESAGUE = 5; /* 15 */

/****** VALORES CALIBRADOS DE SENSORES *****/
/* Sensor nivel de líquido */
const int TANQUE_VACIO = 350; /* 420 450 POR DEBAJO de este valor el tanque está vacío */
const int TANQUE_LLENO = 500; /* 575 600 POR ENCIMA de este valor el tanque está lleno */
const int TANQUE_REBOSA = 575; /* 625 660 POR ENCIMA de este valor el tanque rebosa */

/* Sensor humedad del suelo */
const int SUELO_FUERA = 1000; /*POR ENCIMA de este valor el sensor está fuera */
const int SUELO_SECO = 400; /*  400 600 POR ENCIMA de este valor el suelo está seco */

/* Sensor de lluvia */
const int HAY_LLUVIA = 400; /* POR DEBAJO de este valor hay lluvia */

/* Sensor de luz LDR **/
const int UMBRAL_LUZ = 25; /* POR DEBAJO de este valor es de noche */

/* Zumbador */
const int melody[] = {NOTE_C5, NOTE_D5, NOTE_E5, NOTE_F5, NOTE_G5, NOTE_A5, NOTE_B5, NOTE_C6};
const int duracionZumbido = 200;
long finZumbidos;

/*========================================*/


/****  VARIABLES  *****/

/* VARIABLES DE ESTADO */
volatile boolean hayAgua = true; /* ¿Hay agua en el depósito? */
volatile boolean estoyRegando = false;
volatile boolean estaLloviendo = false;
volatile boolean hayExcesoAgua = false;
volatile boolean estoyDesaguando = false;
volatile boolean estaSueloHumedo = false;

/*** CREAMOS OBJETOS ***/
/* SENSOR BME280 */
typedef DFRobot_BME280_IIC    BME;
BME   bme(&Wire, 0x76);   /* Interfaz I2C, dirección 0x76 */
/* LCD  direccion  0x27, 16 columnas, 2 filas */
LiquidCrystal_I2C lcd(0x27, 16, 2);

/* VALORES LEIDOS POR LOS SENSORES */

int nivelLuz; /* Lectura del LDR en A0 */
int humedadTierra; /* Lectura sensor humedad tierra en A1 */
int cuantaLluvia; /* Lectura sensor lluvia en A2 */
int nivelAgua; /* Lectura nivel agua en A3 */

float    temperaturaAmbiente; /* BME280 temperatura ºC */
uint32_t    presionAmbiente; /* BME280 temperatura mb */
float     humedadAmbiente; /* BME280 humedad % */

String estadoLCD = "      ";  /* Los seis caracteres vecios */

/*========================================*/



/****  FUNCIONES *****/

/* FUNCION ESCRIBE DATOS EN LCD */
/* Referencia: https://www.prometec.net/displays-lcd/ */
void escribeLCD(String linea1, String linea2) {
  /*    lcd.begin(); */
  lcd.init(); /* hay que limpiar primero la pantalla */
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print(linea1);
  lcd.setCursor(0, 1);
  lcd.print(linea2);
}

/* FUNCION LEE LUZ */
int leeLuz() {
  int lecturaLuz = analogRead(PIN_LDR) / 10; /* no alcanza 1024, no hace falta map() */
  return lecturaLuz;
}

/* FUNCION LEE HUMEDAD SUELO */
int leeHumedadSuelo() {
  int lecturaHumedadSuelo = analogRead(PIN_SENSOR_HUMEDAD_TIERRA);
  return lecturaHumedadSuelo;
}

/* FUNCION LEE LLUVIA */
int leeLluvia() {
  int lecturaLluvia = analogRead(PIN_SENSOR_LLUVIA);
  return lecturaLluvia;
}

/* FUNCION LEE NIVEL AGUA */
int leeNivelAgua() {
  int lecturaNivelAgua = analogRead(PIN_NIVEL_AGUA);
  return lecturaNivelAgua;
}

/* FUNCION LEE TEMPERATURA AMBIENTE */
float leeTemperaturaAmbiente() {
  float lecturaTemperaturaAmbiente = bme.getTemperature();
  return lecturaTemperaturaAmbiente;
}

/* FUNCION LEE PRESION AMBIENTE */
uint32_t leePresionAmbiente() {
  float lecturaPresionAmbiente = bme.getPressure();
  return lecturaPresionAmbiente;
}

/* FUNCION LEE HUMEDAD AMBIENTE */
float leeHumedadAmbiente() {
  float lecturaHumedadAmbiente = bme.getHumidity();
  return lecturaHumedadAmbiente;
}

/* FUNCION RIEGO */
void activaRiego() {
  /* Regamos el tiempo establecido en la constante TIEMPO_RIEGO */
  /* Bomba de riego en PIN_BOMBA_RIEGO (2) */
  long comienzoRiego = millis();
  digitalWrite (PIN_BOMBA_RIEGO, HIGH); /* Enciendo bomba */
  estoyRegando = true; /* Actualizo variable de estado */
  estadoLCD = "RIEGO";
  if (DEBUG) Serial.println("activaRiego");
  if (millis() - comienzoRiego > (TIEMPO_RIEGO * 1000)) {
    detieneRiego();
  }
}

/* FUNCION DETIENE RIEGO */
void detieneRiego() {
  digitalWrite (PIN_BOMBA_RIEGO, LOW); /* Apago bomba */
  estoyRegando = false; /* Actualizo variable de estado */
  estadoLCD = "      "; /* Borro el mensaje sobreescribiendo espacios */
  if (DEBUG) Serial.println("detieneRiego");
}

/* FUNCION DESAGUE */
void activaDesague() {
  /* Desaguamos el tiempo establecido en la constante TIEMPO_DESAGUE */
  /* bomba de desague en PIN_BOMBA_DESAGUE (3) */
  long comienzoDesague = millis();
  digitalWrite (PIN_BOMBA_DESAGUE, HIGH); /* Enciendo bomba */
  estoyDesaguando = true; /* Actualizo variable de estado */
  estadoLCD = "EVACUA";
  if (DEBUG) Serial.println("activaDesague");
  if (millis() - comienzoDesague > TIEMPO_DESAGUE * 1000) {
    digitalWrite (PIN_BOMBA_DESAGUE, LOW); /* Apago bomba */
    estoyDesaguando = false; /* Actualizo variable de estado */
    estadoLCD = "      "; /* Borro el mensaje sobreescribiendo espacios */
    if (DEBUG) Serial.println("cierra bomba desague");
  }
}

void printLastOperateStatus(BME::eStatus_t eStatus) /* Muestra el ultimo estado de operacion del BME280 */


{
  switch (eStatus) {
    case BME::eStatusOK:    Serial.println("Todo ok"); break;
    case BME::eStatusErr:   Serial.println("Error desconocido"); break;
    case BME::eStatusErrDeviceNotDetected:    Serial.println("Sensor no detectado"); break;
    case BME::eStatusErrParameter:    Serial.println("Error de parametro"); break;
    default: Serial.println("Estado desconocido"); break;

  }
}

/*========================================*/

void setup() {
  bme.reset();
  Serial.println("Test de lectora de datos de BME");
  while (bme.begin() != BME::eStatusOK) {
    Serial.println("Inicio de BME fallido");
    printLastOperateStatus(bme.lastOperateStatus);
    delay(2000);
  }
  Serial.println("Inicio de BME correcto");
  delay(100);

  lcd.init(); /*Inicializo LCD */
  lcd.backlight(); /* Luz de fondo de LCD */
  if (DEBUG) Serial.begin(9600);
}

void loop() {

  if (DEBUG) {
    Serial.println("---------------------------------------------");
    Serial.print("| hayAgua: ");
    Serial.println(hayAgua);
    Serial.print("| estoyRegando: ");
    Serial.println(estoyRegando);
    Serial.print("| estaLloviendo: ");
    Serial.println(estaLloviendo);
    Serial.print("| hayExcesoAgua: ");
    Serial.println(hayExcesoAgua);
    Serial.print("| estoyDesaguando: ");
    Serial.println(estoyDesaguando);
    Serial.print("| estaSueloHumedo: ");
    Serial.println(estaSueloHumedo);
    Serial.print("| nivelLuz: ");
    Serial.println(nivelLuz);
    Serial.print("| humedadTierra: ");
    Serial.println(humedadTierra);
    Serial.print("| cuantaLluvia: ");
    Serial.println(cuantaLluvia);
    Serial.print("| nivelAgua: ");
    Serial.println(nivelAgua);
    Serial.print("| temperaturaAmbiente: ");
    Serial.println(String(temperaturaAmbiente));
    Serial.print("| presionAmbiente: ");
    Serial.println(presionAmbiente);
    Serial.print("| humedadAmbiente: ");
    Serial.println(String(humedadAmbiente));
    Serial.print("| estadoLCD: ");
    Serial.println(estadoLCD);
    Serial.println("---------------------------------------------");
    delay(500);
  }



  /* Comprobamos si llueve y almacenamos el valor en cuantaLluvia */
  cuantaLluvia = leeLluvia();

  /* Tomo valores */
  temperaturaAmbiente = leeTemperaturaAmbiente(); /* BME280 temperatura ºC */
  presionAmbiente = leePresionAmbiente(); /* BME280 temperatura mb */
  humedadAmbiente = leeHumedadAmbiente(); /* BME280 humedad % */
  humedadTierra = leeHumedadSuelo(); /* Lectura sensor humedad tierra en A1 */
  nivelLuz = leeLuz();
  nivelAgua = leeNivelAgua();

  /* Se actualizan las variables de estado */
  if (nivelAgua < TANQUE_VACIO) {
    hayAgua = false;
  } else if (nivelAgua > TANQUE_LLENO) {
    hayAgua = true;
  }
  if (nivelAgua > TANQUE_REBOSA) {
    hayExcesoAgua = true;
  } else {
    hayExcesoAgua = false;
  }
  if (humedadTierra > SUELO_SECO) {
    estaSueloHumedo = false;
  } else {
    estaSueloHumedo = true;
  }
  if (cuantaLluvia < HAY_LLUVIA) {
    estaLloviendo = true;
  } else {
    estaLloviendo = false;
  }

  /* Nivel luz: si es bajo, si tierra seca y hay agua y no esta lloviendo: riego */
  if ( nivelLuz < UMBRAL_LUZ & humedadTierra < 300 & hayAgua & !estaLloviendo) {
    //if (nivelLuz < 20 & !estaSueloHumedo & hayAgua & !estaLloviendo) {
    activaRiego();
  }

  /* Si tierra seca, hay agua y no llueve, activo el riego */
  if ( leeHumedadSuelo() < 300 & hayAgua & !estaLloviendo) {
    //  if ( !estaSueloHumedo & hayAgua & !estaLloviendo ) {
    activaRiego();
  }

  /* Si llueve y estoy regando, paro el riego */
  if ( estaLloviendo & estoyRegando ) {
    detieneRiego();
  }

  /* Si depósito lleno ,desaguo o riego */
  if ( hayExcesoAgua ) {
    //int miHumedadSuelo = leeHumedadSuelo(); /* con el IF de abajo, me da que no hace falta */
    if (!estaSueloHumedo) {
      activaRiego();
    } else {
      activaDesague();
    }

  }

  /*  Zumbador si el tanque está vacío o si va a rebosar */
  if (!hayAgua) {
    tone(12, melody[0], duracionZumbido);
    tone(12, melody[2], duracionZumbido);
    tone(12, melody[6], duracionZumbido);
    finZumbidos = millis();
    estadoLCD = "LLENAR";
    while (millis() < finZumbidos + (duracionZumbido * 1.1) ) {
      // espere [(duracionZumbido * 1.1)] milisegundos
    }
    /* delay(duracionZumbido * 1.1);  */

  } else if (hayExcesoAgua) {
    tone(12, melody[7], duracionZumbido);
    tone(12, melody[5], duracionZumbido);
    tone(12, melody[1], duracionZumbido);
    finZumbidos = millis();
    // MAAAAL quita lo que no debe estadoLCD = "      "; /* Borro el mensaje sobreescribiendo espacios */
    while (millis() < finZumbidos + (duracionZumbido * 1.1) ) {
      // espere [(duracionZumbido * 1.1)] milisegundos
    }
    /* delay(duracionZumbido * 1.1);  */
  }

  /* Compongo lineas a mostrar en LCD */
  escribeLCD(String(presionAmbiente) + "pa " + String(temperaturaAmbiente) + "\337C",
             String(int(humedadAmbiente)) + "%A " + String(int(100 - (humedadTierra / 10))) + "%T " + estadoLCD);
}
