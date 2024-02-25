/*  ///    |    DN GASBAG C PROTOTYPE 2024.0
   / ///   |    (C) 2024, BRANISLAV JUHAS
  / / /    |    PART OF DN SOFTWARE HERITAGE
*/

// define the types for better prototyping of x86 assembly code
typedef unsigned char byte;
typedef unsigned int word;

// define the structure for the balloon
struct balloon {
  byte x;
  word y;
  byte cv[2];
  byte type;
  byte speed;
};

// define the structure for the bullet
struct bullet {
  word x;
  word y;
};

byte current_buffer = 0;

// x position of the player
word player = 0;

// number of bullets
byte bullets = 0;

// array of balloons
struct balloon balloons [10];

// seed for the random number generator
word seed = 0;

// time of the last frame
word last_time = 0;

int spawn_delay = 0;

char *hexa = "0123456789ABCDEF";

char *balloons_prototypes[] = {
  "\x0C\x04\xDC\x1F\xDB\x1F\xDB\x1F\xDB\x1F\xDC\x1F\x20\x1F\xDB\x1F\xDB\x1F\xDB\x1F\xDB\x1F\xDB\x1F\xDB\x17\xDF\x1F\xDB\x1F\xDB\x1F\xDB\x1F\xDF\x7F\xDB\x17\x20\x1F\x20\x1F\xDF\x1F\xDB\x17\xDF\x17\x20\x1F",
  "\x0C\x05\x20\x1F\xDC\x1F\xDC\x1F\xDC\x1F\x20\x1F\x20\x1F\xDB\x1F\xDB\x1F\xDB\x1F\xDB\x1F\xDB\x1F\xDC\x17\xDB\x1F\xDB\x1F\xDB\x1F\xDB\x1F\xDB\x1F\xDB\x17\x20\x1F\xDF\x1F\xDB\x1F\xDF\x7F\xDB\x17\xDF\x17\x20\x1F\x20\x1F\x20\x1F\xDF\x17\x20\x1F\x20\x1F"
};

// define the video memory
char far *VGA = (char far *)0xB8000000;

void prepare_time(void) {
  byte seconds;
  byte centiseconds;

  // get the time using the BIOS interrupt
  asm {
    mov ah, 0x2C
    int 0x21
    mov seconds, dh
    mov centiseconds, dl
  }

  // set the last time to the current time
  last_time = centiseconds + seconds * 100;
}

// function to get the time (seconds and centiseconds)
word get_delta_time(void) {
  byte seconds;
  byte centiseconds;
  word calculated_time;
  word delta;
  // get the time using the BIOS interrupt
  asm {
    mov ah, 0x2C
    int 0x21
    mov seconds, dh
    mov centiseconds, dl
  }
  // if the time has wrapped around, add calculate the remaining time from the
  // last time to the end of the minute and add it to the current time
  calculated_time = centiseconds + seconds * 100;
  if (calculated_time < last_time) {
    // set delta time to the remaining time to the minute and calculated time
    delta = calculated_time + 6000 - last_time;

    // set the last time and return delta time
    last_time = calculated_time;
    return delta;
  }

  // calculate the time since the last frame
  delta = calculated_time - last_time;

  // set the last time to the current time
  last_time = calculated_time;

  // return the time
  return delta;
}

// function to make random function more random
void randomize(void) {
  // get the time using the BIOS interrupt and use it as the seed
  asm {
    mov ah, 0x2C
    int 0x21
    mov seed, dx
  }
}

// function to generate a random number
byte random_byte(byte max) {
  // generate a random number using the seed
  seed = (seed * 0x15A4E + 1) & 0xFFFF;

  // return the random number (considering the maximum expected)
  return (byte)seed % max;
}

// function to read the keyboard port
byte read_keyboard(void) {
  // declare a variable to store the key
  byte key;

  // read the keyboard port
  asm {
    in al, 60h
    mov key, al
  }

  // return the key
  return key;
}

// function to clear the screen using the BIOS scroll
void clear_screen(void) {
  // clear the screen using the BIOS scroll
  asm {
    mov ah, 0x06
    mov al, 0
    mov bh, 0x1F
    mov cx, 0
    mov dh, 24
    mov dl, 79
    int 0x10
  }
}

// function to hide the cursor using the BIOS interrupt
void hide_cursor(void) {
  // hide the cursor
  asm {
    mov ah, 1
    mov cx, 0x2607
    int 10h
  }
}

void screen_stream(word offset, word data) {
  VGA[offset] = data;
}

// function to render the balloon with the smart rendering
void smart_render(byte i) {
    word j, c, deoffset;
    byte w, h, half;
    // calculate the vertical position of the balloon
    word offset = (balloons[i].y / 1000);
    // calculate the difference in vertical position
    byte difference = offset - balloons[i].cv;

    // if the vertical position hasn't changed, no need to render
    if (difference == 0) {
        return;
    }

    // calculate the offset for rendering
    deoffset = balloons[i].cv / 2;
    deoffset *= 160;
    deoffset += balloons[i].x * 2;

    // update the render position of the balloon
    balloons[i].cv = offset;

    // calculate the number of lines to render
    difference = difference / 2 + 1;

    // ensure the maximum difference is 4 lines
    difference = (difference > 5) ? 5 : difference;

    // adjust the offset for rendering
    offset /= 2;
    offset *= 160;
    offset += balloons[i].x * 2;

    half = (balloons[i].y / 1000) % 2;

    w = balloons_prototypes[(balloons[i].type - 1) / 2 + half][0];
    h = balloons_prototypes[(balloons[i].type - 1) / 2 + half][1];

    // move offsets by 4 to up (unsafe: writes data to unknown memory locations)
    offset -= balloons_prototypes[(balloons[i].type - 1) / 2][1] * 160;;
    deoffset -= balloons_prototypes[(balloons[i].type - 1) / 2][1] * 160;;

    // render each line of the balloon
    for (j = 0; j < difference; j++) {
        for (c = 0; c < w / 2; c++) {
            VGA[deoffset + c * 2] = ' '; // clear the character
            VGA[deoffset + c * 2 + 1] = 0x1F; // set the color attribute
        }
        deoffset += 160; // move to the next line
    }

    for (j = 0; j < h; j++) {
        for (c = 0; c < w; c++) {
            VGA[offset + c] = balloons_prototypes[(balloons[i].type - 1) / 2 + half][j * 12 + c + 2];
        }
        offset += 160; // move to the next line
    }
}

// function to render the balloon with the full rendering
void full_render(byte i) {
  word j, c;
  byte w, h, half;
  // calculate the vertical position of the balloon
  word offset = balloons[i].y / 1000;

  // calculate the offset for choosing the prototype
  half = (balloons[i].y / 1000) % 2;

  // get the width and height of the balloon prototype
  w = balloons_prototypes[(balloons[i].type - 1) / 2 + half][0];
  h = balloons_prototypes[(balloons[i].type - 1) / 2 + half][1];

  // adjust the offset for rendering
  offset /= 2;
  offset *= 160;
  offset += balloons[i].x * 2;

  offset -= balloons_prototypes[(balloons[i].type - 1) / 2][1] * 160;

  // render each line of the balloon
  for (j = 0; j < h; j++) {
    for (c = 0; c < w; c++) {
      VGA[offset + c] = balloons_prototypes[(balloons[i].type - 1) / 2 + half][j * 12 + c + 2];
    }
    offset += 160; // move to the next line
  }
}

// function to fully derender balloon
void full_derender(byte i) {
  word j, c;
  byte w, h, half;
  // calculate the vertical position of the balloon
  word offset = balloons[i].cv;

  // calculate the offset for choosing the prototype
  half = (balloons[i].cv) % 2;

  // get the width and height of the balloon prototype
  w = balloons_prototypes[(balloons[i].type - 1) / 2 + half][0];
  h = balloons_prototypes[(balloons[i].type - 1) / 2 + half][1] + 1;

  // adjust the offset for rendering
  offset /= 2;
  offset *= 160;
  offset += balloons[i].x * 2;

  offset -= (h - 1) * 160;

  // render each line of the balloon
  for (j = 0; j < h; j++) {
    for (c = 0; c < w; c+= 2) {
      VGA[offset + c] = ' ';
      VGA[offset + c + 1] = 0x1F;
    }
    offset += 160; // move to the next line
  }
}

// function to kill the balloon
void kill_balloon(byte i) {
  // set the type of the balloon to 0 (empty) and derender it
  balloons[i].type = 0;
  full_derender(i);
}

// function to calculate horizontal distance between two objects
byte horizontal_distance(byte i, byte c) {
  signed char hdst = (signed char)i - (signed char)c;

  // return the absolute value of the difference in hdst
  hdst = (hdst < 0) ? hdst * -1 : hdst;
  return (byte)hdst;
}

// function to calculate the ideal speed for the balloon new balloon
byte ideal_speed(byte c) {
  // ASSUMPTION: v1 = v2 * d1 / d2

  // declare a variable to store the speed
  word speed = 0;
  // calculate the distance of the existing balloon from the end of the screen
  byte distance = 50 - balloons[c].cv;

  // calculate the speed of the new balloon
  speed = balloons[c].speed * 42;
  speed /= distance;

  // return the speed
  return (byte)speed;
}

// function to spawn a new balloon
void spawn_balloon(void)
{
  // define the pointer to the first empty balloon
  byte i = 0;
  byte c = 0;

  // iterate through the balloons and find the first empty one
  for (i = 0; i < 10; i++)
  {
    if (balloons[i].type == 0)
    {
      break;
    }
  }

  // randomize for better result
  randomize();

  // set the type of the balloon to 1 (full)
  balloons[i].type = 1;
  balloons[i].x = random_byte(75);
  balloons[i].y = 0;
  balloons[i].speed = 5 + random_byte(70);

  // for each balloon, check, if their horizontal distance is not less than 7
  for (c = 0; c < 10; c++) {
    // skip all empty balloons and self
    if (c == i) { continue; }
    if (balloons[c].type == 0) { continue; }

    if (horizontal_distance(balloons[i].x, balloons[c].x) < 7) {
      // if the balloons y is lower than 4000, move balloon 7 to right
      if (balloons[c].y <= 15000) {
        balloons[i].x += 7;
        balloons[i].x = (balloons[i].x > 74) ? 1 : balloons[i].x;
        
        // restart the loop to make a new check and continue
        c = 0;
        continue;
      }

      // if the new balloon's speed is greather than the original balloon's, set
      // it to the original's one
      if (balloons[i].speed > ideal_speed(c)) {
        balloons[i].speed = ideal_speed(c);
      }
    }
  }
}

// function to clear screen using the DOS interrupt (for the end of the game)
void clear_screen_dos(void) {
  // clear the screen using the DOS interrupt
  asm {
    mov ah, 0x06
    mov al, 0
    mov bh, 0x07
    mov cx, 0
    mov dh, 24
    mov dl, 79
    int 0x10
    mov ah, 0x02
    mov bh, 0
    mov dh, 0
    mov dl, 0
    int 0x10
  }
}

// function to print a word to the screen
void print_word(word w, word offset) {
  // declare a variable to store the digits
  byte digits[5];
  byte i;

  // iterate through the digits of the number
  for (i = 0; i < 5; i++)
  {
    // get the last digit of the number
    digits[i] = w % 10;

    // remove the last digit from the number
    w /= 10;
  }

  // iterate through the digits of the number
  for (i = 0; i < 5; i++)
  {
    // print the digit to the screen
    VGA[offset + i * 2] = digits[4 - i] + 0x30;
  }

}

// function to print a word to the screen in hexadecimal
void print_word_hex(word w, word offset) {
  // declare a variable to store the digits
  byte digits[4];
  byte i;

  // iterate through the digits of the number
  for (i = 0; i < 4; i++)
  {
    // get the last digit of the number
    digits[i] = w % 16;

    // remove the last digit from the number
    w /= 16;
  }

  // iterate through the digits of the number
  for (i = 0; i < 4; i++)
  {
    // print the digit to the screen
    VGA[offset + i * 2] = hexa[digits[4 - i]];
  }

}

// function to handle the main game process
int main() {
  byte key, i;
  word delta_time, addition;
  byte debug = 0;

  // clear the screen and hide the cursor
  clear_screen();
  hide_cursor();

  // set the last time to the current time
  prepare_time();

  // main loop (while the user doesn't press the escape key (1)
  while (key != 1) {
    // get the time since the last frame
    delta_time = get_delta_time();

    // get the key from the keyboard
    key = read_keyboard();

    // process key events
    switch (key) {
      // F1 - exit debug mode
      case 59:
        debug = 0;
        clear_screen();
        break;
      // F2 - enter debug mode 1
      case 60:
        debug = 1;
        break;
      // F3 - enter debug mode 2
      case 61:
        debug = 2;
        break;
      // F4 - enter debug mode 3
      case 62:
        debug = 3;
        break;
      default:
        break;
    }

    // if no time has passed, continue
    if (delta_time == 0) {
      continue;
    }

    // update spawn delay
    if (spawn_delay >= delta_time) {
      spawn_delay -= delta_time;
    } else {
      spawn_delay = 0;
    }
    
    if (spawn_delay <= 0) {
      spawn_balloon();
      spawn_delay = 250 + random_byte(70);
    }

    // update balloon positions and handle debug mode
    for (i = 0; i < 10; i++) {
      if (balloons[i].type == 0) {
        continue;
        }

      addition = delta_time * balloons[i].speed;
      balloons[i].y += addition;

      // handle debug mode
      if (debug == 1) {
        // print the balloon data to the screen
        VGA[i * 160] = 0x30 + i;
        VGA[i * 160 + 4] = 0x30 + balloons[i].type;
        VGA[i * 160 + 8] = 0x30 + balloons[i].x;
        VGA[i * 160 + 12] = 0x30 + balloons[i].cv;
        print_word(balloons[i].y, i * 160 + 16);
      } else if (debug == 2 && delta_time > 0) {
        // print the framerate to the screen
        print_word(100 / delta_time, 0);
      } else if (debug == 3)
      {
        print_word(spawn_delay, 0);
      }
      

      if (balloons[i].y > 50000) {
        // kill the balloon if it's too low
        kill_balloon(i);
        continue;
      }

      smart_render(i);
    }
  }

  // clear the screen in DOS mode before exiting
  clear_screen_dos();

  // return to ms dos console
  return 0;
}
