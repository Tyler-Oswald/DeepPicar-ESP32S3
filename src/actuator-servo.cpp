#ifdef CONFIG_ACTUATOR_SERVO

#include <Arduino.h>

static int fr_dir_Pin = 2;
static int fr_pwm_Pin = 4;
static int steeringServoPin = 6;

static const int STEER_CH = 2;
static const int MOTOR_CH = 4;

static const int STEER_FREQ = 50;
static const int STEER_BITS = 14;
static const int STEER_PERIOD_US = 1000000 / STEER_FREQ;   // 20000

static const int PWM_FREQ = 30000;
static const int PWM_BITS = 10;
static const int PWM_MAX = (1 << PWM_BITS) - 1;

static const int STEER_MAX_DEG = 45;
static const int STEER_MIN_US = 1000;
static const int STEER_CENTER_US = 1550;
static const int STEER_MAX_US = 2000;

static int throttle_cmd = 0;

void set_steering(int steering_deg);

static int steering_cmd = 0;
int get_steering_deg() { return steering_cmd; }
int get_throttle_pct() { return throttle_cmd; }

static void write_servo_us(int us) {
    uint32_t duty = ((uint32_t)us << STEER_BITS) / STEER_PERIOD_US;
    ledcWrite(STEER_CH, duty);
}

void setup_control() {
    pinMode(fr_dir_Pin, OUTPUT);

    ledcSetup(MOTOR_CH, PWM_FREQ, PWM_BITS);
    ledcAttachPin(fr_pwm_Pin, MOTOR_CH);
    ledcWrite(MOTOR_CH, 0);

    ledcSetup(STEER_CH, STEER_FREQ, STEER_BITS);
    ledcAttachPin(steeringServoPin, STEER_CH);

    set_steering(0);
}

void set_throttle(int throttle_pct) {
    throttle_cmd = constrain(throttle_pct, -100, 100);

    digitalWrite(fr_dir_Pin, throttle_cmd < 0 ? LOW : HIGH);
    ledcWrite(MOTOR_CH, abs(throttle_cmd) * PWM_MAX / 100);
}

void set_steering(int steering_deg) {
    steering_cmd = constrain(steering_deg, -STEER_MAX_DEG, STEER_MAX_DEG);
    int us = STEER_CENTER_US + steering_cmd * (STEER_MAX_US - STEER_MIN_US) / (2 * STEER_MAX_DEG);
    write_servo_us(constrain(us, STEER_MIN_US, STEER_MAX_US));
}

void throttleup()   { set_throttle(throttle_cmd + 2); }
void throttledown() { set_throttle(throttle_cmd - 2); }

void nomove() {
    throttle_cmd = 0;
    ledcWrite(MOTOR_CH, 0);
}

#endif