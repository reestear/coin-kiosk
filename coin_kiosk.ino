#include <LiquidCrystal_I2C.h>

// Coins
// 20 -> 10 -> 50 -> 100
const int coins[4] = {20, 10, 50, 100};
unsigned long last_mode_debounce_time = 0, last_confirm_debounce_time = 0, debounce_delay = 100;
int mode_button_state = HIGH, last_mode_button_state = HIGH;
int confirm_button_state, last_confirm_button_state = 0;

// Defining Pins
const int ir_pins[4] = {8, 9, 10, 11};
const int motor_pins[4] = {4, 5, 6, 7};
const int mod_pin = 3, confirm_pin = 2, buzzer_pin = 12;

// Configuring LCD Screen
LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0x27, 26, 2);

// Global States
// status: 0 - Idle, 1 - Filling Coins, 2 - Selecting Mode, 3 - Giving Coins
int filling_sum = 0, status = 2;
bool mode = false;
int bank_amount[4] = {0, 0, 0, 0}, total_sum = 0;

int first_time = 0;

/*
  mode: 0 - Initial Bank Amount, 1 - Filled Amount, 2 - Selecting Mode, 3 - Giving Coins

  Coins:  20 | 10 | 50 | 100
  Amount: 2  | 13 | 0  | 1

  Amount: xxx
  Press OK to continue

  Selected Mode: Smaller(Bigger)
  Press OK to continue

  Giving your coins
  Please wait......

*/
void print_screen(){
  switch (status) {
    case 0:
      lcd.setCursor(0, 0);
      lcd.print("C: ");
      for(int i = 0; i < 4; i++) {
        lcd.print(coins[i]);
        if(i != 3) lcd.print("|");
      }
      
      lcd.setCursor(0, 1);
      lcd.print("A: ");
      for(int i = 0; i < 4; i++) {
        lcd.print(bank_amount[i]);
        if(bank_amount[i] < 10) lcd.print(" ");
        if(i != 3) lcd.print("|");
      }

      return;
    
    case 1:
      lcd.setCursor(0, 0);
      lcd.print("Amount: ");
      lcd.print(filling_sum);

      lcd.setCursor(0, 1);
      lcd.print("OK to continue");

      return;
    
    case 2:
      lcd.setCursor(0, 0);
      lcd.print("Mode: ");
      lcd.print(mode == 1 ? "Smaller" : "Bigger ");

      lcd.setCursor(0, 1);
      lcd.print("OK to continue");

      return;
    
    case 3:
      lcd.setCursor(0, 0);
      lcd.print("Giving coins");

      while(1) {
        lcd.setCursor(0, 1);
        lcd.print("Please wait   ");

        delay(500);

        lcd.setCursor(0, 1);
        lcd.print("Please wait.");

        delay(500);

        lcd.setCursor(0, 1);
        lcd.print("Please wait..");

        delay(500);

        lcd.setCursor(0, 1);
        lcd.print("Please wait...");

        delay(500);
      }

      return;
  }
}

int find_coin(int target_ind){
  if(target_ind == 4) return -1;

  int start_time = millis();
  while(millis() < start_time + 300) {
    if(digitalRead(ir_pins[target_ind])) {
      int ret = find_coin(target_ind + 1);
      if(ret == -1) return target_ind;
      else return ret;
    }
  }

  return -1;
}

void give_coins(){

}

void setup()
{
  for(int i = 0; i < 4; i++) pinMode(ir_pins[i], INPUT), pinMode(motor_pins[i], OUTPUT);
  pinMode(mod_pin, INPUT);
  pinMode(confirm_pin, INPUT);
  pinMode(buzzer_pin, OUTPUT);

  lcd.init();
  lcd.setBacklight(1);

  print_screen();

  Serial.begin(9600);
}

void loop()
{
  // delay(1000);
  // if(first_time == 0) print_screen(), first_time = 1;
  // int found_coin = find_coin(0);
  int mode_reading = digitalRead(mod_pin);
  // int confirm_reading = digitalRead(confirm_pin);

  // Serial.println(confirm_reading);
  
  // int is_confirmed = 0;

  if (mode_reading != last_mode_button_state) {
    last_mode_debounce_time = millis();
  }

  if ((millis() - last_mode_debounce_time) > debounce_delay) {
    if (mode_reading != mode_button_state) {
      mode_button_state = mode_reading;

      if (mode_button_state == HIGH) {
        mode = !mode;
        print_screen();
      }
    }
  }

  // if (confirm_reading != last_confirm_button_state) {
  //   last_confirm_debounce_time = millis();
  // }

  // if ((millis() - last_confirm_debounce_time) > debounce_delay) {
  //   if (confirm_reading != confirm_button_state) {
  //     confirm_button_state = confirm_reading;

  //     if (confirm_button_state == HIGH) {
  //       is_confirmed = 1;
  //     }
  //   }
  // }

  // // if(found_coin != -1) {
  // //   status = 1;
  // //   filling_sum += coins[found_coin];
  // //   print_screen();
  // // }

  // Serial.print("Current mode: ");
  // Serial.println(mode);

  // if(is_confirmed) {
  //   Serial.println("Pressed confirm");
  //   // if(status == 1) status = 2;
  //   // if(status == 2) {
  //   //   status = 3;
  //   //   print_screen();

  //   //   // Waiting Function
  //   //   give_coins();

  //   //   status = 0;
  //   //   filling_sum = 0;
  //   //   print_screen();
  //   // }
  // }

  last_mode_button_state = mode_reading;
  // last_confirm_button_state = confirm_reading;
}