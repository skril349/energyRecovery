# Control de Motors i Servo amb Arduino

Aquest projecte permet controlar dos motors, un servomotor , una bomba d'aigua i dos ventiladors amb Arduino. Utilitza la biblioteca **ODriveArduino** per controlar motors mitjançant el controlador ODrive i la biblioteca **Servo** per manipular el servomotor. A més, es controlen dues entrades de ventilador i una bomba.

## Requisits

- **Hardware**: 
  - Un controlador **ODrive** per als motors.
  - Un servomotor connectat al pin digital 11.
  - Ventiladors connectats als pins digitals 3 i 4.
  - Bomba connectada a l'entrada 3 i 4 amb un pin d'habilitació (ENB) al pin 5.
- **Llibreries**:
  - **HardwareSerial** i **SoftwareSerial** per la comunicació sèrie.
  - **ODriveArduino** per al control del controlador ODrive.
  - **Servo** per al control del servomotor.

## Funcionalitat

El programa controla els motors, el servomotor i els ventiladors en diferents estats. Aquests estats es defineixen en la funció `loop()` i es basen en la interacció de l'usuari a través del monitor sèrie.

### Funcions principals

1. **Moviment del servomotor**:
   - `moveLeft()`: Mou el servomotor cap a l'esquerra.
   - `moveRight()`: Mou el servomotor cap a la dreta.
   - `moveCenterFromRight()` i `moveCenterFromLeft()`: Centren el servomotor després de moure’l.

2. **Moviment del motor ODrive**:
   - `moveMotorToPosition(axis, voltes)`: Mou un motor ODrive a una posició específica en funció de les voltes.
   - `moveMotorsToPosition(axis0, voltes0, axis1, voltes1)`: Mou dos motors ODrive simultàniament a posicions específiques.
   - `isMotorAtPosition(motor_number, setpoint)`: Comprova si el motor ha arribat a la posició desitjada.

3. **Lectura de paràmetres del motor**:
   - `GetIntensity(motor_number)`: Retorna la intensitat de corrent del motor.
   - `GetVelocity(motor_number)`: Retorna la velocitat actual del motor.
   - `GetPosition(motor_number)`: Retorna la posició actual del motor.

4. **Calibració de motors**:
   - `checkAndCalibrateAxis(axis)`: Comprova l'estat actual del motor i executa la seqüència de calibratge si no està calibrat.

### Funcionament en el `loop()`

La funció `loop()` defineix els diferents estats que determinen el comportament del sistema:
- **Estat 0**: Espera que l'usuari introdueixi la mostra i tanqui la tapa.
- **Estat 2-3**: Realitza el primer moviment de posicionament per a la segona mostra.
- **Estat 4**: Espera que l'usuari posi la segona mostra.
- **Estats 5-10**: Inicien cicles automàtics amb moviment del motor, activació dels ventiladors i moviment del servomotor, permetent que l'usuari aturi els cicles si cal.

## Configuració del Projecte

1. Conecta els motors ODrive a través del port sèrie en els pins 8 i 9 de l’Arduino.
2. Conecta el servomotor al pin digital 11.
3. Conecta els ventiladors als pins 3 i 4, la bomba a les entrades 3 i 4, i el pin d'habilitació a ENB.
4. Puja el codi a l'Arduino i obre el monitor sèrie a 115200 bauds.

### Comandes del Monitor Sèrie

- **`y`**: L'usuari pot escriure `y` per avançar en els estats i activar els cicles.

### Paràmetres configurables

- `move_to`: Posició de destinació per al motor 1.
- `turns`: Nombre de voltes que fa el motor 0.
- `cicles`: Nombre total de cicles automàtics.
- `pauseIn` i `pauseOut`: Temps de pausa en mil·lisegons per a cada moviment.

## Notes

- Els motors han de ser calibrats abans de ser controlats en bucle tancat.
- Assegureu-vos que l’alimentació del controlador ODrive és suficient per a l'operació.
- Es poden modificar els temps de pausa i el nombre de cicles per ajustar el comportament del sistema.

## Autor

Aquest codi ha estat desenvolupat per gestionar una seqüència automatitzada de moviment i control de motors en un sistema complex amb Arduino per en Toni Vives Cabaleiro.
