#include "funshield.h"

struct DiceController {

  enum class Mode { NORMAL, CONF, RESULT };
  size_t throws = 1;
  size_t type = 4;
  size_t result = 0;
  size_t randomModifier = 1000;

  Mode currentMode = Mode::NORMAL;

  void switchMode(Mode mode) { currentMode = mode; }

  bool isInNormalMode() { return (currentMode == Mode::NORMAL) ? true : false; }
  bool isInConfMode() { return (currentMode == Mode::CONF) ? true : false; }
  bool isInResultMode() { return (currentMode == Mode::RESULT) ? true : false; }

  void changeDiceThrows() {
    ++throws;
    if (throws > 9)
      throws = 1;
  }

  void changeDiceType() {
    if (type == 12)
      type = 20;
    else if (type == 20)
      type = 100;
    else if (type == 100)
      type = 4;
    else
      type = type + 2;
  }

  size_t randomGenerator(size_t parameter) {
    size_t result = 1;
    for (size_t i = 0; i < type - 1; ++i) {
      if ((random(randomModifier)) < parameter)
        ++result;
    }
    return result;
  }

  void updateResult(unsigned long pressedTime) {
    size_t parameter = (pressedTime % randomModifier);
    result = 0;
    for (size_t i = 0; i < throws; ++i) {
      result = result + randomGenerator(parameter);
    }
  }

} Dice;

struct Display {

  byte D_GLYPH = 0b10100001;
  byte EMPTY_GLYPH = 0b11111111;
  byte digits[10]{0b11000000, 0b11111001, 0b10100100, 0b10110000, 0b10011001,
                  0b10010010, 0b10000010, 0b11111000, 0b10000000, 0b10010000};

  size_t position = 0;
  size_t positionsCount = 4;

  size_t animationTime = 500;
  unsigned long lastAnimationAt = 0;
  bool animationIndicator = true;

  bool animatinSwitch(unsigned long currentTime) {
    if (currentTime - lastAnimationAt >= animationTime) {
      lastAnimationAt = currentTime;
      animationIndicator = !animationIndicator;
    }
  }

  void setup() {
    pinMode(latch_pin, OUTPUT);
    pinMode(clock_pin, OUTPUT);
    pinMode(data_pin, OUTPUT);
  }

  size_t digitOnPosition(size_t number) {
    size_t pow = 1;
    for (size_t i = 0; i < position; i++) {
      pow *= 10;
    }

    return (number / pow) % 10;
  }

  size_t numberLength(size_t number) {
    size_t length = 0;

    while (number != 0) {
      ++length;
      number = number / 10;
    }
    return length;
  }

  void writeGlyph(byte glyph) {
    digitalWrite(latch_pin, LOW);
    shiftOut(data_pin, clock_pin, MSBFIRST, glyph);
    shiftOut(data_pin, clock_pin, MSBFIRST, 0b1000 >> position);
    digitalWrite(latch_pin, HIGH);

    position = (position + 1) % positionsCount;
  }

  void renderResult(size_t number) {
    size_t length = numberLength(number);
    size_t digit = digitOnPosition(number);
    byte glyph = digits[digit];

    if (position >= length && digit == 0) {
      glyph = EMPTY_GLYPH;
    }

    writeGlyph(glyph);
  }

  void renderConf(size_t throws, size_t type, unsigned long currentTime,
                  bool animate) {
    byte glyph = EMPTY_GLYPH;

    animatinSwitch(currentTime);

    if (position == 3)
      glyph = digits[throws];
    else if (position == 2 && animate)
      if (animationIndicator)
        glyph = D_GLYPH;
      else
        glyph = EMPTY_GLYPH;
    else if (position == 2)
      glyph = D_GLYPH;
    else if (position == 1 && numberLength(type) < 2)
      glyph = digits[type];
    else if (position == 0 && numberLength(type) < 2)
      glyph = EMPTY_GLYPH;
    else
      glyph = digits[digitOnPosition(type)];

    writeGlyph(glyph);
  }

} Display;

struct Led {
  int pin;
  bool ledON = false;

  Led(int pin) { this->pin = pin; }

  void setup() {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, OFF);
  }

  void switchState() {
    if (ledON == false)
      turnOn();
    else
      turnOff();
  }

  void turnOn() {
    digitalWrite(pin, ON);
    ledON = true;
  }

  void turnOff() {
    digitalWrite(pin, OFF);
    ledON = false;
  }
};

struct LedSystem {
  Led ledArray[4]{Led(led4_pin), Led(led3_pin), Led(led2_pin), Led(led1_pin)};
  size_t ledCount = 4;
  bool ledsOFF = true;
  size_t animationTime = 300;
  unsigned long lastAnimationAt = 0;

  void setup() {
    for (size_t i = 0; i < ledCount; i++) {
      ledArray[i].setup();
    }
  }

  void animate(unsigned long currentTime) {
    ledsOFF = false;
    if (currentTime - lastAnimationAt >= animationTime) {
      lastAnimationAt = currentTime;
      for (size_t i = 0; i < ledCount; ++i) {
        ledArray[i].switchState();
      }
    }
  }

  void turnOff() {
    if (!ledsOFF) {
      for (size_t i = 0; i < ledCount; ++i) {
        ledArray[i].turnOff();
      }
      ledsOFF = true;
    }
  }
} Leds;

struct Button {

  int pin;
  unsigned long pressedAt = 0;

  enum class State { UP, DOWN };

  State currentState = State::UP;

  Button(int pin) { this->pin = pin; }

  void setup() { pinMode(pin, INPUT); }

  bool isDown() { return (currentState == State::DOWN) ? true : false; }

  size_t pressedTime(unsigned long currentTime) {
    return currentTime - pressedAt;
  }

  bool action(unsigned long currentTime) {
    int input = digitalRead(pin);

    if (input == ON) {
      if (currentState == State::UP) {
        pressedAt = currentTime;
        currentState = State::DOWN;
      }
      return false;
    } else {
      if (currentState == State::DOWN) {
        currentState = State::UP;
        return true;
      }
      return false;
    }
  }
};

struct ButtonSystem {
  Button buttonArray[3]{Button(button1_pin), Button(button2_pin),
                        Button(button3_pin)};
  size_t buttonCount = 3;

  void setup() {
    for (size_t i = 0; i < buttonCount; i++) {
      buttonArray[i].setup();
    }
  }
} Buttons;

void setup() {
  Buttons.setup();
  Display.setup();
  Leds.setup();
}

void loop() {

  unsigned long currentTime = millis();

  if (Buttons.buttonArray[0].action(currentTime)) {

    if (!Dice.isInNormalMode())
      Dice.switchMode(DiceController::Mode::NORMAL);
    else {
      Dice.switchMode(DiceController::Mode::RESULT);
      Dice.updateResult(Buttons.buttonArray[0].pressedTime(currentTime));
    }

  } else if (Buttons.buttonArray[1].action(currentTime)) {

    if (!Dice.isInConfMode())
      Dice.switchMode(DiceController::Mode::CONF);
    else
      Dice.changeDiceThrows();

  } else if (Buttons.buttonArray[2].action(currentTime)) {

    if (!Dice.isInConfMode())
      Dice.switchMode(DiceController::Mode::CONF);
    else
      Dice.changeDiceType();
  }

  if (Buttons.buttonArray[0].isDown() && Dice.isInNormalMode())
    Leds.animate(currentTime);
  else
    Leds.turnOff();

  if (Dice.isInResultMode())
    Display.renderResult(Dice.result);
  else if (Dice.isInConfMode())
    Display.renderConf(Dice.throws, Dice.type, currentTime, true);
  else
    Display.renderConf(Dice.throws, Dice.type, currentTime, false);
}
