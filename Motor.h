class Motor {
  public:
    unsigned int step_pin;
    unsigned int dir_pin;
    unsigned int endstop_pin;
    unsigned int step_resolution;
    unsigned int step_delay;
    unsigned int enable_pin;
    int dir;
    long delta;
    float home_ret;
    int home_dir;
    bool krok;


    Motor(unsigned int step_pin, unsigned int dir_pin, unsigned int enable_pin, unsigned int endstop_pin, float step_resolution, unsigned int step_delay, int home_dir) {
      this->step_pin = step_pin;
      this->dir_pin = dir_pin;
      this->step_delay = step_delay;
      this->step_resolution = step_resolution;
      this->enable_pin = enable_pin;
      this->endstop_pin = endstop_pin;
      this-> dir = 0;
      this-> delta = 0;
      this-> krok = 0;
      this-> home_ret = home_ret;
      this-> home_dir = home_dir;

      pinMode(this->step_pin, OUTPUT);
      pinMode(this->dir_pin,  OUTPUT);
      pinMode(this->enable_pin,  OUTPUT);
      pinMode(this->endstop_pin,  INPUT);
      //digitalWrite(this->dir_pin, dir ? HIGH : LOW);
      digitalWrite(this->enable_pin, LOW);
    }

    void make_step(int opoznienie_wl) {
      //digitalWrite(this->dir_pin, dir);
      //digitalWrite(this->enable_pin, LOW);
      digitalWrite(this->step_pin, HIGH);
      //delayMicroseconds(this->step_delay);
      delayMicroseconds(opoznienie_wl);
      digitalWrite(this->step_pin, LOW);
      //delayMicroseconds(this->step_delay);
      delayMicroseconds(opoznienie_wl);
    }

    void set_dir(bool dir) {
      digitalWrite(this->dir_pin, dir ? HIGH : LOW);
    }

    void set_delay(unsigned int new_delay) {
      this->step_delay = new_delay;
    }

    int home() {
      digitalWrite(this->dir_pin, home_dir);
      while (digitalRead(this->endstop_pin)) {

        digitalWrite(this->step_pin, HIGH);
        //delayMicroseconds(this->step_delay);
        delayMicroseconds(300);
        digitalWrite(this->step_pin, LOW);
        //delayMicroseconds(this->step_delay);
        delayMicroseconds(300);
      }

      digitalWrite(this->dir_pin, ((home_dir > 0) ? LOW : HIGH));
      while (!(digitalRead(this->endstop_pin))) {

        digitalWrite(this->step_pin, HIGH);
        //delayMicroseconds(this->step_delay);
        delayMicroseconds(300);
        digitalWrite(this->step_pin, LOW);
        //delayMicroseconds(this->step_delay);
        delayMicroseconds(300);
      }
      digitalWrite(this->dir_pin, home_dir);
      while (digitalRead(this->endstop_pin)) {

        digitalWrite(this->step_pin, HIGH);
        //delayMicroseconds(this->step_delay);
        delayMicroseconds(300);
        digitalWrite(this->step_pin, LOW);
        //delayMicroseconds(this->step_delay);
        delayMicroseconds(300);
      }
      return 0;
    }
};
