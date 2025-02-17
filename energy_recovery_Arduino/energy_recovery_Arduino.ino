// includes
#include <HardwareSerial.h>
#include <SoftwareSerial.h>
#include <ODriveArduino.h>
#include <Servo.h>

Servo myservo;  // crea un objecte Servo per controlar el servomotor
int fan1 = 3;
int fan2 = 4;
int IN3 = 7;    // Input3 conectada al pin 5
int IN4 = 6;    // Input4 conectada al pin 4 
int ENB = 5;
int pumpCicles = 1;
int numeroCiclesInicial = 3; //8 = 20 minuts aprox
int numeroCiclesFinal = 6; //6 = 15 minuts aprox
int multimeter = 10;
int delayWaterPump = 2000;
// Constants per als estats del motor
// Utilitza els valors de l'enum definits a ODriveEnums.h
// AxisState AXIS_STATE_IDLE = 1;
// AxisState AXIS_STATE_CLOSED_LOOP_CONTROL = 8;

// Printing with stream operator helper functions
template<class T> inline Print& operator <<(Print &obj, T arg) {
  obj.print(arg);
  return obj;
}
template<> inline Print& operator <<(Print &obj, float arg) {
  obj.print(arg, 4);
  return obj;
}

// variables
int motornum;
int state = 0;
int test;
float move_to = 18;//18
float constantDeRotacio = 20 / 33;
float turns = 25;//25
bool calibrated = false;
int cicles = 30;
int pauseIn = 15000;
int pauseOut = 15000;
//float positionTolerance = 0.05;
float positionTolerance = 0.25;  // Tolerància per comprovar la posició del motor

// SoftwareSerial for ODrive communication
SoftwareSerial odrive_serial(8, 9);

// ODrive object
ODriveArduino odrive(odrive_serial);

//Servomotor:
void moveLeft() {
  digitalWrite(multimeter,HIGH);
  myservo.write(180); // mou el servomotor a l'esquerra
  delay(700);
  stopServo();
}

void moveRight() {
  digitalWrite(multimeter,HIGH);
  myservo.write(0); // mou el servomotor a la dreta
  delay(700);
  stopServo();
}
void moveCenterFromRight() {
  digitalWrite(multimeter,LOW);
  myservo.write(0); // mou el servomotor a la dreta
  delay(350);
  stopServo();
}
void moveCenterFromLeft() {
  digitalWrite(multimeter,LOW);
  myservo.write(180); // mou el servomotor a la dreta
  delay(350);
  stopServo();
}

void stopServo() {
  myservo.write(91);  // situa el servomotor a la posició mitjana (90 graus)
}

void moveMotorToPosition(int axis, float voltes) {
  Serial.println(voltes);
  voltes = voltes;
  Serial.println(voltes);
  // Configura el mode d'entrada del controlador per l'eix especificat
  odrive_serial << "w axis" << axis << ".controller.config.input_mode = INPUT_MODE_TRAP_TRAJ" << '\n';
  delay(5);

  // Mou l'eix especificat al nombre de voltes indicat
  odrive_serial << "t " << axis << " " << voltes << '\n';

  // Espera fins que el motor hagi arribat a la posició
  while (!isMotorAtPosition(axis, voltes)) {
    delay(100);
  }
}


void moveMotorsToPosition(int axis0, float voltes0, int axis1, float voltes1) {
  // Configura el mode d'entrada del controlador per l'eix especificat
  odrive_serial << "w axis" << axis0 << ".controller.config.input_mode = INPUT_MODE_TRAP_TRAJ" << '\n';
  delay(5);
  odrive_serial << "w axis" << axis1 << ".controller.config.input_mode = INPUT_MODE_TRAP_TRAJ" << '\n';


  // Mou l'eix especificat al nombre de voltes indicat
  odrive_serial << "t " << axis0 << " " << voltes0 << '\n';
  delay(5);
  odrive_serial << "t " << axis1 << " " << voltes1 << '\n';

  // Espera fins que el motor hagi arribat a la posició
  while (!isMotorAtPosition(axis0, voltes0) || !isMotorAtPosition(axis1, voltes1)) {
    delay(100);
  }
}




bool isMotorAtPosition(int motor_number, float setpoint) {
  // Llegeix la posició actual del motor
  odrive_serial << "r axis" << motor_number << ".encoder.pos_estimate\n";
  float position = readString().toFloat();

  // Llegeix l'estat actual del motor
  odrive_serial << "r axis" << motor_number << ".current_state\n";
  int current_state = readString().toInt();

  if (current_state == AXIS_STATE_CLOSED_LOOP_CONTROL) {
    if (abs(position - setpoint) < positionTolerance) {
      Serial.println("El motor ha arribat a la posició desitjada.");
      return true;
    }
  } else {
    Serial.println("El motor no està en control en bucle tancat.");
  }
  return false;
}

// functions
float GetIntensity(int motor_number) {
  odrive_serial << "r axis" << motor_number << ".motor.current_control.Iq_measured\n";
  return readString().toFloat();
}

float GetVelocity(int motor_number) {
  odrive_serial << "r axis" << motor_number << ".encoder.vel_estimate\n";
  return readString().toFloat();
}

float GetPosition(int motor_number) {
  odrive_serial << "r axis" << motor_number << ".encoder.pos_estimate\n";
  return readString().toFloat();
}

String readString() {
  String str = "";
  static const unsigned long timeout = 1000;
  unsigned long timeout_start = millis();
  for (;;) {
    while (!odrive_serial.available()) {
      if (millis() - timeout_start >= timeout) {
        return str;
      }
    }
    char c = odrive_serial.read();
    if (c == '\n')
      break;
    str += c;
  }
  return str;
}

void checkAndCalibrateAxis(int axis) {
  int requested_state;

  // Check current state
  odrive_serial << "r axis" << axis << ".current_state\n";
  int current_state = readString().toInt();

  if (current_state != AXIS_STATE_CLOSED_LOOP_CONTROL) {
    Serial.print("Starting calibration for axis ");
    Serial.println(axis);

    requested_state = AXIS_STATE_FULL_CALIBRATION_SEQUENCE;
    odrive_serial << "w axis" << axis << ".requested_state " << requested_state << '\n';
    Serial.print("Requested state: ");
    Serial.println(requested_state);

    do {
      odrive_serial << "r axis" << axis << ".current_state\n";
      current_state = readString().toInt();
      delay(100);
    } while (current_state != AXIS_STATE_IDLE);

    requested_state = AXIS_STATE_CLOSED_LOOP_CONTROL;
    odrive_serial << "w axis" << axis << ".requested_state " << requested_state << '\n';
  } else {
    Serial.print("Axis ");
    Serial.print(axis);
    Serial.println(" already calibrated.");
  }

  Serial.print("Current state: ");
  Serial.println(current_state);
}

void setup() {
  // ODrive uses 115200 baud
  odrive_serial.begin(115200);
  pinMode (ENB, OUTPUT); 
  pinMode (IN3, OUTPUT);
  pinMode (IN4, OUTPUT);
  pinMode(multimeter, OUTPUT);
  digitalWrite (IN3, LOW);
  digitalWrite (IN4, LOW);
  analogWrite(ENB,0);
  // Serial to PC
  Serial.begin(115200);
  myservo.attach(11);   // adjunta el servomotor al pin digital 11 de l'Arduino
  myservo.write(90);   // situa el servomotor a la posició mitjana (quiet)
  Serial.println("ODriveArduino");
  Serial.println("Setting parameters...");
  pinMode(fan1, OUTPUT);
  pinMode(fan2,OUTPUT);

  // Set parameters for both motors
  odrive_serial << "w axis0.controller.config.vel_limit " << 30.0f << '\n';
  odrive_serial << "w axis0.motor.config.current_lim " << 25.0f << '\n';
  odrive_serial << "w axis0.trap_traj.config.accel_lim " << 50.0f << '\n';
  odrive_serial << "w axis0.trap_traj.config.decel_lim " << 50.0f << '\n';
  odrive_serial << "w axis0.trap_traj.config.vel_lim " << 30.0f << '\n';

  odrive_serial << "w axis1.controller.config.vel_limit " << 30.0f << '\n';
  odrive_serial << "w axis1.motor.config.current_lim " << 25.0f << '\n';
  odrive_serial << "w axis1.trap_traj.config.accel_lim " << 50.0f << '\n';
  odrive_serial << "w axis1.trap_traj.config.decel_lim " << 50.0f << '\n';
  odrive_serial << "w axis1.trap_traj.config.vel_lim " << 30.0f << '\n';

  Serial.println("Ready!");
  digitalWrite(fan1, LOW);
  digitalWrite(fan2, LOW);
  moveCenterFromRight();
  // Check and calibrate motors
  checkAndCalibrateAxis(1);
  checkAndCalibrateAxis(0);
  
  

  Serial.println("posiciona el pistó 1 en la posició mínima de forma manual i introdueix la mostra. escriu 'y' quan ho tinguis fet i tanca la tapa amb el motor posat: ");
}

void loop() {
  switch (state) {
    case 0:
      if (Serial.available()) {
        char c = Serial.read();
        if (c == 'y' || c == 'Y') {
          state = 2;
        }
      }
      break;

    case 2:
      Serial.print("Ens mourem a: ");
      Serial.println(move_to);
      Serial.print("Rotarem a :");
      Serial.println(turns);
      Serial.println("Esperem 5 segons per fer el primer moviment i així posar la segona mostra:");
      for (int i = 5; i > 0; i--) {
        delay(1000);
        Serial.print(i);
        Serial.print(" ");
      }
      Serial.println();
      state = 3;
      break;

    case 3:
      moveMotorToPosition(1, move_to * -1);
      moveMotorToPosition(0, turns * 20 / 33);
      state = 4;
      break;
    case 4:
      // Afegir qualsevol altra lògica que vulguis per a l'estat 4
      Serial.println("Moviment completat. Pots posar la segona mostra. prem 'y' quan tinguis la segona mostra posada");
      state = 5; // Reiniciar l'estat per permetre un nou cicle de moviment
      break;

    case 5:
      if (Serial.available()) {
        char c = Serial.read();
        if (c == 'y' || c == 'Y') {
          Serial.println("Retornarem a la posició inicial en 5 segons");
          for (int i = 5; i > 0; i--) {
            delay(1000);
            Serial.print(i);
            Serial.print(" ");
          }
          Serial.println();
          state = 6;
        }
      }
      break;

    case 6:
      moveMotorToPosition(1, 0);
      moveMotorToPosition(0, 0);
      state = 7;
      break;

    case 7:
      Serial.print("pauseIn en milisegons = ");
      Serial.println(pauseIn);
      Serial.print("pauseOut en milisegons = ");
      Serial.println(pauseOut);
      Serial.println("Esperem 5 segons per començar els cicles");
      for (int i = 5; i > 0; i--) {
        delay(1000);
        Serial.print(i);
        Serial.print(" ");
      }
      Serial.println();
      moveLeft();
      Serial.println("Si vols començar els cicles prem 'y' :");
      state = 8;
      break;

    case 8:
      if (Serial.available()) {
        
        
        
        char c = Serial.read();
        if (c == 'y' || c == 'Y') {
       /*   for (int l = 0; l < numeroCiclesInicial; l++) {
          digitalWrite (IN3, LOW);
          digitalWrite (IN4, HIGH);
          analogWrite(ENB,255);
          delay(delayWaterPump);
          analogWrite(ENB,0);
          digitalWrite (IN3, LOW);
          digitalWrite (IN4, LOW);
        Serial.print(l);
        Serial.print(" ");
        //delay(pumpCicles*2*pauseIn);
        delay(30000);
        
      }
      digitalWrite (IN3, LOW);
          digitalWrite (IN4, HIGH);
          analogWrite(ENB,255);
          delay(delayWaterPump);
          analogWrite(ENB,0);

         
          digitalWrite (IN3, LOW);
          digitalWrite (IN4, LOW);
        delay(30000);

        */
          digitalWrite(fan1, HIGH);
          digitalWrite(fan2, LOW);
          state = 9;
          
        }
      }
      break;

    case 9:
      for (int i = 0; i < cicles; i++) {
        Serial.print("Cicle = ");
        Serial.println(i);
        delay(pauseIn-6000);
        digitalWrite(fan1, LOW);
        digitalWrite(fan2, LOW);
        delay(6000);
        moveCenterFromRight();

        if(i % pumpCicles == 0){
          digitalWrite (IN3, LOW);
          digitalWrite (IN4, HIGH);
          analogWrite(ENB,255);
          delay(delayWaterPump);
          analogWrite(ENB,0);
        }
        else{
          digitalWrite (IN3, LOW);
          digitalWrite (IN4, LOW);
          analogWrite(ENB,0);
          }
        
        //moveMotorToPosition(1, move_to * -1);
        //moveMotorToPosition(0, turns * 20 / 33);
        
        moveMotorsToPosition(0,turns * 20/33, 1, move_to * -1);
        digitalWrite(fan1, LOW);
        digitalWrite(fan2, HIGH);
        //delay(1000);
        moveRight();
        delay(pauseOut-6000);
        digitalWrite(fan1, LOW);
        digitalWrite(fan2, LOW);
        delay(6000);
        moveCenterFromLeft();
        //moveMotorToPosition(1, 0);
        //moveMotorToPosition(0, 0);
        moveMotorsToPosition(0,0, 1,0);
        digitalWrite(fan1, HIGH);
        digitalWrite(fan2, LOW);
        //delay(1000);
        moveLeft();
        //delay(1000);

        // Comprova si hi ha dades disponibles al port sèrie
        if (Serial.available() > 0) {
          int input = Serial.parseInt(); // Llegeix les dades del port sèrie
          Serial.println(input);
          // Si la dada rebuda és "stop", surt del bucle
          if (input == -1) {
            Serial.println("Cicles aturats per l'usuari.");
            i = cicles;
            state = 10;
            break;
          }
        }
      }

      digitalWrite(fan1, LOW);
      digitalWrite(fan2, LOW);

/*for (int l = 0; l < numeroCiclesFinal; l++) {
          digitalWrite (IN3, LOW);
          digitalWrite (IN4, HIGH);
          analogWrite(ENB,255);
          delay(delayWaterPump);
          analogWrite(ENB,0);
          digitalWrite (IN3, LOW);
          digitalWrite (IN4, LOW);
        Serial.print(l);
        Serial.print(" ");
        delay(30000);
      }

      */
      
      Serial.println("Si vols començar els cicles de nou prem 'y' :");
      state = 10;
      
      break;

    case 10:
      if (Serial.available()) {
        char c = Serial.read();
        if (c == 'y' || c == 'Y') {
          for (int l = 0; l < numeroCiclesInicial; l++) {
          digitalWrite (IN3, LOW);
          digitalWrite (IN4, HIGH);
          analogWrite(ENB,255);
          delay(delayWaterPump);
          analogWrite(ENB,0);
          digitalWrite (IN3, LOW);
          digitalWrite (IN4, LOW);
        Serial.print(l);
        Serial.print(" ");
        delay(30000);
        
      }
      digitalWrite (IN3, LOW);
          digitalWrite (IN4, HIGH);
          analogWrite(ENB,255);
          delay(delayWaterPump);
          analogWrite(ENB,0);
          digitalWrite (IN3, LOW);
          digitalWrite (IN4, LOW);
        delay(30000);
          digitalWrite(fan1, HIGH);
          digitalWrite(fan2, LOW);
          state = 9;
        }
      }
      break;

    default:
      break;
  }
}
