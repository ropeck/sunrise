
State machine
  ASLEEP, AWAKE

```

#define ASLEEP 0
#define AWAKE 1

Int toggle_state(s) {
  If (s == AWAKE) {
    Return ASLEEP;
  }
  Return AWAKE;
}

Static int state;
Static time_t[2] alarm_times;

setup() {
  State = AWAKE;
  Alarm_times[AWAKE] = 6:00am;
  alarm_times[ASLEEP] = 10:00pm;
}

loop() {
  Calculate light color;
  Button_pressed = check_button_status();

  Alarm = alarm_times[state];
  Calculate color;
  If check_button_status() {
    State = toggle_state(state);
    beep_alert(state);
    Set color = 0%
  }
   If the previous color != color {
      Adjust the color one step towards the new color;
    }
}

```

