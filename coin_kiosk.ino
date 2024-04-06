#include <LiquidCrystal_I2C.h>
#include <Servo.h> 

// Coins
// 20 -> 10 -> 50 -> 100
const int coins[4] = {20, 10, 50, 100};
const int ind[4] = {1, 0, 2, 3};
unsigned long last_mode_debounce_time = 0, last_confirm_debounce_time = 0, debounce_delay = 50;
int mode_button_state = HIGH, last_mode_button_state = HIGH;
int confirm_button_state, last_confirm_button_state = 0;

// Defining Pins
const int ir_pins[4] = {8, 9, 10, 11};
const int motor_pins[4] = {4, 5, 6, 7};
const int mod_pin = 3, confirm_pin = 2, buzzer_pin = 12;
// const int x_pin = A0, y_pin = A1;

// Configuring LCD Screen
LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0x27, 26, 2);
Servo servos[4];

// Global States
// status: 0 - Idle, 1 - Filling Coins, 2 - Selecting Mode, 3 - Giving Coins
int filling_sum = 110, status = 0;
bool mode = false;
int bank_amount[4] = {5, 1, 0, 2}, total_sum = 0;

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
      lcd.print("Giving coins   ");

      lcd.setCursor(0, 1);
      lcd.print("Please wait...");

      return;
    
    default:
      return;
  }
}

int find_coin(int target_ind){
  if(target_ind == 4) return -1;

  if(target_ind == 0) {
    if(digitalRead(ir_pins[target_ind])) {
      int ret = find_coin(target_ind + 1);
      if(ret == -1) return target_ind;
      else return ret;
    }

    return -1;
  }

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

int minimize(int sum, int coin_ind, int buffer[]){
  if(sum == filling_sum) return 1;
  if(sum > filling_sum) return 0;
  if(coin_ind == -1) return 0;

  for(int i = bank_amount[ind[coin_ind]]; i >= 0; i--) {
    buffer[ind[coin_ind]] = i;
    if(minimize(sum + i * coins[ind[coin_ind]], coin_ind - 1, buffer)) return 1;
  }

  return 0;
}

int maximize(int sum, int coin_ind, int buffer[]){
  if(sum == filling_sum) return 1;
  if(sum > filling_sum) return 0;
  if(coin_ind == 4) return 0;

  for(int i = bank_amount[ind[coin_ind]]; i >= 0; i--) {
    buffer[ind[coin_ind]] = i;
    if(maximize(sum + i * coins[ind[coin_ind]], coin_ind + 1, buffer)) return 1;
  }

  return 0;
}

void calculate_coins(int buffer[]){
  switch (mode) {
    case 0:
      maximize(0, 0, buffer);
      return;
    case 1:
      minimize(0, 3, buffer);
      return;
    default:
      return;
  }
}

void loop(){ 
   // Make servo go to 0 degrees 
   Servo1.write(0); 
   delay(1000); 
   // Make servo go to 90 degrees 
   Servo1.write(110); 
   delay(1000); 
   // Make servo go to 0 degrees 
   Servo1.write(0); 
   delay(1000); 
}

void push_coin(int coin_ind){
  servos[coin_ind].write(0); 
  delay(1000);

  servos[coin_ind].write(110); 
  delay(1000);

  servos[coin_ind].write(0); 
  delay(1000); 
}

void give_coins(){
  // delay(5000);
  int push_coins[4] = {0};
  calculate_coins(push_coins);

  Serial.println("Bank Amount Before: ");
  for(int i = 0; i < 4; i++) {
    Serial.print(bank_amount[i]);
    Serial.print(" ");
  }

  Serial.println("Giving Coins: ");
  for(int i = 0; i < 4; i++) {
    Serial.print(push_coins[i]);
    Serial.print(" ");

    for(int j = 0; j < push_coins[i]; j++) push_coin(i);

    bank_amount[i] -= push_coins[i];
  }

  Serial.println("Bank Amount After: ");
  for(int i = 0; i < 4; i++) {
    Serial.print(bank_amount[i]);
    Serial.print(" ");
  }

}

void setup()
{
  for(int i = 0; i < 4; i++) pinMode(ir_pins[i], INPUT), servos[i].attach(motor_pins[i]);
  pinMode(mod_pin, INPUT_PULLUP);
  pinMode(confirm_pin, INPUT_PULLUP);
  pinMode(buzzer_pin, OUTPUT);

  lcd.init();
  lcd.setBacklight(1);

  print_screen();

  Serial.begin(9600);
}

void loop()
{
  int found_coin = find_coin(0);
  int mode_reading = digitalRead(mod_pin);
  int confirm_reading = digitalRead(confirm_pin);

  // Serial.println(confirm_reading);
  
  int is_confirmed = 0;

  if (mode_reading != last_mode_button_state) {
    last_mode_debounce_time = millis();
  }

  if ((millis() - last_mode_debounce_time) > debounce_delay) {
    if (mode_reading != mode_button_state) {
      mode_button_state = mode_reading;

      if (mode_button_state == LOW) {
        mode = !mode;
        print_screen();
      }
    }
  }

  if (confirm_reading != last_confirm_button_state) {
    last_confirm_debounce_time = millis();
  }

  if ((millis() - last_confirm_debounce_time) > debounce_delay) {
    if (confirm_reading != confirm_button_state) {
      confirm_button_state = confirm_reading;

      if (confirm_button_state == LOW) {
        is_confirmed = 1;
      }
    }
  }

  if(found_coin != -1) {
    status = 1;
    filling_sum += coins[found_coin];
    bank_amount[found_coin]++;

    print_screen();
  }

  // Serial.print("Current mode: ");
  // Serial.println(mode);

  if(is_confirmed) {
    // Serial.println("Pressed confirm");
    if(status == 1) status = 2;
    if(status == 2) {
      status = 3;
      print_screen();

      // Waiting Function
      give_coins();

      status = 0;
      filling_sum = 0;
      print_screen();
    }
  }

  last_mode_button_state = mode_reading;
  last_confirm_button_state = confirm_reading;
}