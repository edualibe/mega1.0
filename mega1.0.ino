//***Variables para manejo de mensajes y llamadas por sim 900
int respuesta; //respuesta de la conexion grps
char aux_str[50]; //variable para la conexion gprs
char incoming_char; //variable donde recibe datos del SIM900
char* sms; //variable que contiene sms que envia el SIM900
char* celu_responder; //variable para almacenar celular de respuesta (responde al celular que le envia algun codigo valido)
char* celu_llamar; //variable que contiene el celular a quien llamar (celular que le envia codigo de llamar es a quien llama)
int posicion = 0; //utilizada para leer caracteres recibidos por el SIM900
char num; //para utilizar con SIM900
String cel_recibido = ""; //para armar cadena del numero recibido por SIM900
String cel_recibido_at = ""; //para armar comando at del celu recibido
String cel_llamar_at = ""; //para armar comando at del celu a llamar
char clave; //variable para clave recibida por sms
String clave_recibida = ""; //para armar clave recibida por sms
String celu_aviso = "AT+CMGS=\"+543794888545\""; //celular al que se envia msj por alerta
unsigned long chequeosim_actual = 0; //almacena el tiempo en que realizara el chequeo del SIM900
#define intervalo_chequeoSIM900 600000 // tiempo de intervalo en que sera chequeado el estado del SIM9000 - 600000=10 min
#define SIM900 Serial3
#define NODEMCU Serial2
#define RESTART asm("jmp 0x0000") //comando para reinciar el arduino
//Fin variables manejo SIM900*************************************************************

//variables para lectura de entradas analogicas conectadas a las salidas del nodemcu esp8266
int valor_A0, valor_A1, valor_A2, valor_A3, valor_A4, valor_A5, valor_A6, valor_A7, valor_A8;
#define entrada_analoga_0 A0
#define entrada_analoga_1 A1
#define entrada_analoga_2 A2
#define entrada_analoga_3 A3
#define entrada_analoga_4 A4
#define entrada_analoga_5 A5
#define entrada_analoga_6 A6
#define entrada_analoga_7 A7
#define entrada_analoga_8 A8
//fin variables lectura analogicas esp8266

//variables para salidas digitales activadas atraves de las analogicas por el esp8266
#define salida_1 2
#define salida_2 3
#define salida_3 4
#define salida_4 5
#define salida_5 6
#define salida_6 7
#define salida_7 8
#define salida_8 10
#define salida_9 11
int valor_salida_1, valor_salida_2, valor_salida_3, valor_salida_4, valor_salida_5, valor_salida_6, valor_salida_7, valor_salida_8, valor_salida_9 = 0;
//fin variables salidas digitales

char entradaserial, entradaserial2;
String entradaserial_s, entradaserial2_s;

void setup() {
  pinMode(entrada_analoga_0, INPUT);
  pinMode(entrada_analoga_1, INPUT);
  pinMode(entrada_analoga_2, INPUT);
  pinMode(entrada_analoga_3, INPUT);
  pinMode(entrada_analoga_4, INPUT);
  pinMode(entrada_analoga_5, INPUT);
  pinMode(entrada_analoga_6, INPUT);
  pinMode(entrada_analoga_7, INPUT);
  pinMode(entrada_analoga_8, INPUT);
  pinMode(salida_1, OUTPUT);
  pinMode(salida_2, OUTPUT);
  pinMode(salida_3, OUTPUT);
  pinMode(salida_4, OUTPUT);
  pinMode(salida_5, OUTPUT);
  pinMode(salida_6, OUTPUT);
  pinMode(salida_7, OUTPUT);
  pinMode(salida_8, OUTPUT);
  pinMode(salida_9, OUTPUT);
  digitalWrite(salida_1, 0);
  digitalWrite(salida_2, 0);
  digitalWrite(salida_3, 0);
  digitalWrite(salida_4, 0);
  digitalWrite(salida_5, 0);
  digitalWrite(salida_6, 0);
  digitalWrite(salida_7, 0);
  digitalWrite(salida_8, 0);
  digitalWrite(salida_9, 0);
  pinMode(9, OUTPUT); //para encender la tarjeta sim900
  SIM900.begin(19200); //Configura velocidad del puerto serie para el SIM9000
  Serial.begin(9600);
  NODEMCU.begin(9600);
  encenderSIM900(); //enciende el SIM900 en su primer inicio
}

void loop() {
  //verifica cada cierto tiempo establecido en intervalo_chequeoSIM900 el estado del modulo SIM9000
  //si lo encuentra apagado envia comando para encenderlo a traves del pin 9
  if (millis() > (chequeosim_actual + intervalo_chequeoSIM900)) {
    chequeosim_actual = millis();
    encenderSIM900();
  }
  //fin verificacion estado del SIM900


  //RECEPCION DE DATOS POR SMS
  if (SIM900.available()) {
    delay(50); //parada de control de flujo
    incoming_char = SIM900.read();
    if ((posicion == 0) && (incoming_char == '+')) {
      posicion = 1;
      cel_recibido = "";
      clave_recibida = "";
    }
    if ((posicion == 1) && (incoming_char == 'C')) {
      posicion = 2;
    }
    if ((posicion == 2) && (incoming_char == 'M')) {
      posicion = 3;
    }
    if ((posicion == 3) && (incoming_char == 'T')) {
      posicion = 4;
    }
    if ((posicion == 4) && (incoming_char == ':')) {
      posicion = 5;
    }
    if ((posicion == 5) && (incoming_char == ' ')) {
      posicion = 6;
    }
    if ((posicion == 6) && (incoming_char == '"')) {
      cel_recibido = "";
      posicion = 0;
      for (int i = 0; i < 13; i++) {
        delay(50);
        num = SIM900.read(); //Captura del número remitente.
        if (num == '"') break;
        else
          cel_recibido += num;
      }
      cel_recibido_at = "AT+CMGS=\"" + cel_recibido + "\"";
      cel_llamar_at = "ATD" + cel_recibido + ";";
      celu_llamar = cel_llamar_at.c_str();
      celu_responder = cel_recibido_at.c_str();
    }
    if (incoming_char == '#' || incoming_char == '*') {
      delay(50);
      clave_recibida = "";
      clave_recibida += incoming_char;
      for (int x = 0; x < 2; x++) {
        delay(50);
        clave = SIM900.read();
        clave_recibida += clave;
      }
      while ( SIM900.available() > 0) SIM900.read(); // Limpia el buffer de entrada
      ingreso_codigo(clave_recibida);
    }
  }
  // FIN RECEPCION DE DATOS POR SMS

  //chequeo de las entradas analogas que dependiendo de su valor activa las salidas digitales
  //analogica A0
  valor_A0 = analogRead(entrada_analoga_0);
  if (valor_A0 > 200) {
    if (valor_salida_1 == 0) {
      valor_salida_1 = 1;
      digitalWrite(salida_1, valor_salida_1);
    }
  } else {
    if (valor_salida_1 == 1) {
      valor_salida_1 = 0;
      digitalWrite(salida_1, valor_salida_1);
    }
  }
  //fin analogica A0

  //analogica A1
  valor_A1 = analogRead(entrada_analoga_1);
  if (valor_A1 > 200) {
    if (valor_salida_2 == 0) {
      valor_salida_2 = 1;
      digitalWrite(salida_2, valor_salida_2);
    }
  } else {
    if (valor_salida_2 == 1) {
      valor_salida_2 = 0;
      digitalWrite(salida_2, valor_salida_2);
    }
  }
  //fin analogica A1

  
  //fin chequeo entradas analogas para controlar salidas digitales

  //************************************
  //PORCION DE CODIGO PARA SIMULAR COMANDOS DESDE SIM900
  if (Serial.available()>0) {
    entradaserial_s = "";
    while (Serial.available() > 0) {
      entradaserial = Serial.read();
      delay(100);
      entradaserial_s += entradaserial;
    }
    entradaserial_s.trim();
    ingreso_codigo(entradaserial_s);    
  }
  //FIN PRUEBA COMANDOS SIM900

  //lectura del puerto serial2 donde esta conectada la placa nodemcu esp8266
  /*
  if (NODEMCU.available()>0) {
    entradaserial2_s = "";
    while (NODEMCU.available() > 0) {
      delay(50);
      entradaserial2 = NODEMCU.read();
      entradaserial2_s += entradaserial2;
    }
    Serial.println(entradaserial2_s);
  }
  */
  
  delay(100); //parada para estabilizar flujo

}
//FIN LOOP PRINCIPAL

void encenderSIM900() {
  if (enviarAT("AT", "OK") == 0) {
    digitalWrite(9, HIGH);
    delay(1000);
    digitalWrite(9, LOW);
  }
  iniciarSIM900();
}

void iniciarSIM900() {
  enviarAT("AT+CLIP=1\r", "OK"); // Activamos la identificacion de llamadas
  enviarAT("AT+CMGF=1\r", "OK"); //Configura el modo texto para enviar o recibir mensajes
  enviarAT("AT+CNMI=2,2,0,0,0\r", "OK"); //Configuramos el modulo para que nos muestre los SMS recibidos por comunicacion serie
}

int enviarAT(String ATcommand, char* resp_correcta) {
  int x = 0;
  int correcto = 0;
  char respuesta[100];
  while ( SIM900.available() > 0) SIM900.read(); // Limpia el buffer de entrada
  memset(respuesta, '\0', 100); // Inicializa el string
  delay(50);
  SIM900.println(ATcommand); // Envia el comando AT
  x = 0;
  while (x < 25 || correcto == 1) {
    delay(70);
    respuesta[x] = SIM900.read();
    Serial.println(respuesta);
    if (strstr(respuesta, resp_correcta) != NULL) {
      correcto = 1;
      break;
    }
    x++;
  }
  return correcto;
}

void ingreso_codigo(String data) {
  Serial.println(data);
  if(data == "#*0") {
    RESTART;
  }

  if(data == "#01"){
    if(valor_salida_1==1){
      NODEMCU.println("V00");        
    }else{
      NODEMCU.println("V01");          
    }
  }

  if(data == "#02"){
    if(valor_salida_2==1){
      NODEMCU.println("V10");        
    }else{
      NODEMCU.println("V11");          
    }
  }
}
