// includes
#include <HardwareSerial.h>
#include <SoftwareSerial.h>
#include <ODriveArduino.h>

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
float move_to = 2;
float turns = 2;
bool calibrated = false;
int cicles = 2;
int pauseIn = 8000;
int pauseOut = 12000;

// SoftwareSerial for ODrive communication
SoftwareSerial odrive_serial(8, 9);

// ODrive object
ODriveArduino odrive(odrive_serial);

void moveMotorToPosition(int axis, float voltes) {
  // Configura el mode d'entrada del controlador per l'eix especificat
  odrive_serial << "w axis" << axis << ".controller.config.input_mode = INPUT_MODE_TRAP_TRAJ" << '\n';
  delay(5);

  // Mou l'eix especificat al nombre de voltes indicat
  odrive_serial << "t " << axis << " " << voltes << '\n';
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

  // Serial to PC
  Serial.begin(115200);

  Serial.println("ODriveArduino");
  Serial.println("Setting parameters...");

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

  // Check and calibrate motors
  checkAndCalibrateAxis(0);
  checkAndCalibrateAxis(1);

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
      moveMotorToPosition(1, move_to);
      moveMotorToPosition(0, turns);
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
      state = 8;

      Serial.println("Si vols començar els cicles prem 'y' :");
      break;

      case 8:
      if (Serial.available()) {
        char c = Serial.read();
        if (c == 'y' || c == 'Y') {
          state = 9;
        }
      }
      break;

    case 9:

      for (int i = 0; i < cicles; i ++) {
        Serial.println(pauseIn);
        delay(pauseIn);
        moveMotorToPosition(1, move_to);
        moveMotorToPosition(0, turns);
        delay(pauseOut);
        moveMotorToPosition(1, 0);
        moveMotorToPosition(0, 0);

      }
      Serial.println("Si vols començar els cicles de nou prem 'y' :");
      state = 10;
      break;

    case 10:
    if (Serial.available()) {
        char c = Serial.read();
        if (c == 'y' || c == 'Y') {
          state = 9;
        }
      }
    
    break;


    default:
      break;
  }
}
