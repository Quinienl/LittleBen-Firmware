# LittleBen-Firmware
LittleBen a eurorack masterclock (firmware)

* Internal Clock (play/pause/stop)
  * bpm clock setting
  * amount of steps per bpm
  * reset on beat X
  * 4 clock out
  * 4 reset out
* External Clock  (play as clock in)
  * Still uses internal counter for step and reset
  * 4 clock out (pass thru)
  * 4 reset out
* Random  (play/pause/stop)
  * 8 random output
* Counter  (play/pause/stop)
  * Count from 0 to 255 and outputs the number as byte pins
  * 8 outputs
* Calculate (play as clock in) tested with other LittleBen rounds of to full numbers
  * Calculate a incomming clock signal and set/show bpm
  * Functions like internal
* Divider Internal  (play/pause/stop)
  * Divides the clock in 1,2,3,4,6,8,12,16 
* Divider External  (play as clock in)
  * Divides the external clock in 1,2,3,4,6,8,12,16 
