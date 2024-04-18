#include <LiquidCrystal_I2C.h>
#include <Servo.h> 
#include <time.h> 

// Coins
// 20 -> 10 -> 50 -> 100
const int coins[4] = {20, 10, 50, 100};
const int ind[4] = {1, 0, 2, 3};
unsigned long last_mode_debounce_time = 0, last_confirm_debounce_time = 0, debounce_delay = 50;
int mode_button_state = HIGH, last_mode_button_state = HIGH;
int confirm_button_state, last_confirm_button_state = 0;

const int noteA = 440, noteB = 494, noteC = 523;
const int melody[] = {
  noteA, noteB, noteC, noteA, noteB, noteC, noteB, noteA, noteB, noteC, noteB, noteA, noteC, noteC, noteA
};

const float noteDurations[] = {
  4, 4, 4, 4, 4, 1.5, 4, 4, 4, 1.75, 4, 4, 4, 2, 2
};

// Defining Pins
const int ir_pins[4] = {8, 9, 10, 11};
const int motor_pins[4] = {47, 49, 51, 53};
const int mod_pin = 3, confirm_pin = 2, buzzer_pin = 12;
// const int x_pin = A0, y_pin = A1;

// Configuring LCD Screen
LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0x27, 26, 2);
Servo servos[4];

// Global States
// status: 0 - Idle, 1 - Filling Coins, 2 - Selecting Mode, 3 - Giving Coins, 4 - Successful, 5 - Casino Ready, 6 - Casino Playing, 7 - Casino Done
// mode: 0 - Small nominal, 1 - Big nominal, 2 - Bank Fillin, 3 - Casino
// casino_mode: 0 - not ready, 1 - ready
int filling_sum = 0, status = 0, mode = 0, casino_mode = 0;
int bank_amount[4] = {0, 0, 0, 0}, total_sum = 0;
const char luck_messages[][17] = {"Good Luck!    ", "Good Luck!.   ", "Good Luck!..  ", "Good Luck!... "};
char loading_message[] = "Please Wait   ", luck_message[] = "Good Luck!    ";
const double win_chance = 0.5, MAX_RAND = RAND_MAX;
int prev_casino_set[3] = {7, 7, 7};
int random_set[3] = {0};
const int casino_roll_delay = 50;
bool user_won = false;

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
      lcd.print("   ");
      
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
      lcd.print("        ");

      lcd.setCursor(0, 1);
      lcd.print("OK to continue");

      return;
    
    case 2:
      lcd.setCursor(0, 0);
      lcd.print("Mode: ");
      lcd.print(mode == 0 ? "Smaller      " : mode == 1 ? "Bigger     " : mode == 2 ? "Filling     " : "Casino   ");

      lcd.setCursor(0, 1);
      lcd.print("OK to continue");

      return;
    
    case 3:
      lcd.setCursor(0, 0);
      lcd.print("Giving coins      ");

      lcd.setCursor(0, 1);
      lcd.print(loading_message);

      return;
    
    case 4:
      lcd.setCursor(0, 0);
      lcd.print("Now you are rich!   ");

      lcd.setCursor(0, 1);
      lcd.print(user_won ? "(Actually Yes)" : "(Actually Not)");

      return;
    
    case 5:
      lcd.setCursor(0, 0);
      lcd.print("Roll: ");
      for(int i = 0; i < 3; i++) lcd.print(prev_casino_set[i]), lcd.print(" ");

      lcd.setCursor(0, 1);
      lcd.print("You Sure?: "), lcd.print(casino_mode ? "Yes   " : "No   ");

      return;

    case 6:

      lcd.setCursor(0, 0);
      lcd.print("Roll: ");
      for(int i = 0; i < 3; i++) lcd.print(random_set[i]), lcd.print(" ");

      lcd.setCursor(0, 1);
      lcd.print(luck_message);

      return;
    
    case 7:

      lcd.setCursor(0, 0);
      lcd.print("Roll: ");
      for(int i = 0; i < 3; i++) lcd.print(random_set[i]), lcd.print(" ");
      lcd.print(user_won ? "Yay!     " : "Sad(");

      lcd.setCursor(0, 1);
      lcd.print(user_won ? "OK to collect    " : "OK to continue");

      return;

    default:
      return;
  }
}

void play_audio(int full){
  int till = (full == 1) ? 15 : 6;
  for (int i = 0; i < till; i++) {
    int duration = 1000 / noteDurations[i];
    tone(buzzer_pin, melody[i], duration);

    // Pause between notes
    delay(duration * 1.1);
  }

  noTone(buzzer_pin);
}

int find_coin(int target_ind){
  if(target_ind == 4) return -1;

  if(target_ind == 0) {
    if(!digitalRead(ir_pins[target_ind])) {
      int ret = find_coin(target_ind + 1);
      if(ret == -1) return target_ind;
      else return ret;
    }

    return -1;
  }

  unsigned long start_time = millis();
  while(millis() < start_time + 300) {
    if(!digitalRead(ir_pins[target_ind])) {
      int ret = find_coin(target_ind + 1);
      if(ret == -1) return target_ind;
      else return ret;
    }
  }

  return -1;
}

int random_digit(){
	return floor(((double) rand() / MAX_RAND) * 10);
}

void calculate_random_set(){
	bool _777 = true;
	while(_777) {
		for(int i = 0; i < 3; i++) {
			const int dig = random_digit();
			random_set[i] = dig;
			_777 = _777 && (dig == 7);
		}
	}

	return;
}

void play_casino() {
  if(casino_mode == 0) {
    status = 3;
    print_screen();
    give_coins();

    reset_bank();

    return;
  }

  // casino_mode == 1	
  status = 6;
  user_won = ((double) rand() / MAX_RAND) < win_chance;

  unsigned long start_time = millis();
  unsigned long prev_mes_time = start_time;
  int lm = 0;
  while(millis() - start_time < 5000) {
    calculate_random_set();
    print_screen();

    if(millis() - prev_mes_time > 500) {
      // TO-DO
      strcpy(luck_message, luck_messages[lm]);
      lm = (lm + 1) % 4;
    }

    delay(50);
  }

  if(user_won) {
    for(int i = 0; i < 3; i++) random_set[i] = 7;
  }

  status = 7;
  print_screen();

  for(int i = 0; i < 3; i++) prev_casino_set[i] = random_set[i];
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
      Serial.println("SSSSHEHSD");
      minimize(0, 3, buffer);
      return;
  }
}

void push_coin(int coin_ind){
  // Serial.println("Before");
  int initial = servos[coin_ind].read();
  // Serial.println(initial);
  servos[coin_ind].write(initial + 180);
  // Serial.println("After");
  // Serial.println(servos[coin_ind].read());

  strcpy(loading_message, "Please Wait   ");
  print_screen();
  delay(500);
  strcpy(loading_message, "Please Wait.  ");
  print_screen();
  delay(500);

  servos[coin_ind].write(initial);

  strcpy(loading_message, "Please Wait.. ");
  print_screen();
  delay(500);
  strcpy(loading_message, "Please Wait...");
  print_screen();
  delay(500);
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

  Serial.println("\nGiving Coins: ");
  for(int i = 0; i < 4; i++) {
    Serial.print(push_coins[i]);
    Serial.print(" ");

    for(int j = 0; j < push_coins[i]; j++) push_coin(i);

    bank_amount[i] -= push_coins[i];
    total_sum -= (push_coins[i] * coins[i]);
  }

  Serial.println("\nBank Amount After: ");
  for(int i = 0; i < 4; i++) {
    Serial.print(bank_amount[i]);
    Serial.print(" ");
  }

}

void reset_bank(){
  status = 0;
  filling_sum = 0;
  print_screen();
}

void set_random_seed(void){
	srand(time(NULL));
	rand();
}

void setup()
{ 
  for(int i = 0; i < 4; i++) pinMode(ir_pins[i], INPUT_PULLUP), servos[i].attach(motor_pins[i], 0, 5980);
  pinMode(mod_pin, INPUT_PULLUP);
  pinMode(confirm_pin, INPUT_PULLUP);
  pinMode(buzzer_pin, OUTPUT);

  lcd.init();
  lcd.setBacklight(1);

  print_screen();

  servos[1].write(servos[1].read() - 20);

  set_random_seed();

  Serial.begin(9600);
}

int first_time = 1;

void loop()
{ 
  if(first_time == 1) {
    play_audio(1);

    first_time = 0;
  }
  int found_coin = find_coin(0);
  // if(found_coin != -1) Serial.println(found_coin);
  int mode_reading = digitalRead(mod_pin);
  int confirm_reading = digitalRead(confirm_pin);
  
  int is_confirmed = 0;

  if (mode_reading != last_mode_button_state) {
    last_mode_debounce_time = millis();
  }

  if ((millis() - last_mode_debounce_time) > debounce_delay) {
    if (mode_reading != mode_button_state) {
      mode_button_state = mode_reading;

      if (mode_button_state == LOW) {
        if(status == 5) {
          casino_mode = !casino_mode;
        } else {
          mode = (mode + 1) % 4;
        }

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
    Serial.println("Coin Found: ");
    Serial.println(coins[found_coin]);
    // Serial.println(found_coin);

    status = 1;
    filling_sum += coins[found_coin];
    bank_amount[found_coin]++;
    total_sum += coins[found_coin];

    for(int i = 0; i < 4; i++) Serial.print(coins[i]), Serial.print(" ");
    Serial.println();
    for(int i = 0; i < 4; i++) Serial.print(bank_amount[i]), Serial.print(" ");
    Serial.println();

    print_screen();
  }

  if(is_confirmed) {
  	// Filling Coins
    if(status == 1) {
      status = 2;
      print_screen();
    }
	  // Selecting Mode
    else if(status == 2) {
		// Small or Big nominal or Filling Bank
      if(mode == 0 || mode == 1) {
        status = 3;
        print_screen();
        give_coins();

        status = 4;
        print_screen();
        play_audio(0);

        reset_bank();
      }
      if(mode == 2) {
        reset_bank();
      }
		
      // Casino
      if(mode == 3) {
        status = 5;
        print_screen();
      }
    }
    // Getting ready to play Casino
    else if(status == 5) {
      play_casino();
    }
    // Done playing Casino
    else if(status == 7) {
      Serial.println("Pressed");
      if(user_won == 1) {
        filling_sum = min(filling_sum * 2, total_sum);
        status = 3;
        print_screen();
        give_coins();

        status = 4;
        print_screen();
        play_audio(0);

        user_won = false;
      }
      
      reset_bank();

    }
  }

  last_mode_button_state = mode_reading;
  last_confirm_button_state = confirm_reading;
}
