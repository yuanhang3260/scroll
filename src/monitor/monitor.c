#include "common/io.h"
#include "monitor/monitor.h"

// The VGA framebuffer starts at 0xB8000.
uint16* video_memory = (uint16*)0xC00B8000;
// Stores the cursor position.
uint8 cursor_x = 0;
uint8 cursor_y = 0;

// The background colour is black (0), the foreground is white (15).
const uint8 backColour = COLOR_BLACK;
const uint8 foreColour = COLOR_WHITE;

// Updates the hardware cursor.
static void move_cursor() {
  // The screen is 80 characters wide.
  uint16 cursorLocation = cursor_y * 80 + cursor_x;
  outb(0x3D4, 14);                  // Tell the VGA board we are setting the high cursor byte.
  outb(0x3D5, cursorLocation >> 8); // Send the high cursor byte.
  outb(0x3D4, 15);                  // Tell the VGA board we are setting the low cursor byte.
  outb(0x3D5, cursorLocation);      // Send the low cursor byte.
}

// Scrolls the text on the screen up by one line.
static void scroll() {
  // The attribute byte is made up of two nibbles - the lower being the 
  // foreground colour, and the upper the background colour.
  const uint8 attributeByte = ((backColour << 4) | (foreColour & 0x0F));
  // blank character (black backgroud with nothing)
  const uint16 blank = (0x20 /* space */ | (attributeByte << 8));

  // Row 25 is the end, this means we need to scroll up
  if (cursor_y >= 25) {
    // Move the current text chunk that makes up the screen
    // back in the buffer by a line
    int i;
    for (i = 0*80; i < 24 * 80; i++) {
        video_memory[i] = video_memory[i + 80];
    }

    // The last line should now be blank. Do this by writing
    // 80 spaces to it.
    for (i = 24 * 80; i < 25 * 80; i++) {
        video_memory[i] = blank;
    }
    // The cursor should now be on the last line.
    cursor_y = 24;
  }
}

void monitor_put(char c) {
  monitor_put_with_color(c, COLOR_WHITE);
}

// Writes a single character out to the screen.
void monitor_put_with_color(char c, uint8 color) {
  // The attribute byte is made up of two nibbles - the lower being the 
  // foreground colour, and the upper the background colour.
  const uint8 attributeByte = ((backColour << 4) | (color & 0x0F));
  // The attribute byte is the top 8 bits of the word we have to send to the
  // VGA board.
  uint16 attribute = attributeByte << 8;

  // print
  uint16 *location;
  if (c == 0x08 && cursor_x) {
    // Handle a backspace, by moving the cursor back one space
    cursor_x--;
  } else if (c == 0x09) {
    // Handle a tab by increasing the cursor's X, but only to a point
    // where it is divisible by 8.
    cursor_x = (cursor_x + 8) & ~(8-1);
  } else if (c == '\r') {
    // Handle carriage return
    cursor_x = 0;
  } else if (c == '\n') {
    // Handle newline by moving cursor back to left and increasing the row
    cursor_x = 0;
    cursor_y++;
  } else if(c >= ' ') {
    // Handle any other printable character.
    location = video_memory + (cursor_y * 80 + cursor_x);
    *location = (c | attribute);
    cursor_x++;
  }

  // Check if we need to insert a new line because we have reached the end
  // of the screen.
  if (cursor_x >= 80) {
    cursor_x = 0;
    cursor_y++;
  }

  // Scroll the screen if needed.
  scroll();
  // Move the hardware cursor.
  move_cursor();
}

// Clears the screen, by copying lots of spaces to the framebuffer.
void monitor_clear() {
  // The attribute byte is made up of two nibbles - the lower being the 
  // foreground colour, and the upper the background colour.
  const uint8 attributeByte = ((backColour << 4) | (foreColour & 0x0F));
  // blank character (black backgroud with nothing)
  const uint16 blank = (0x20 /* space */ | (attributeByte << 8));

  int i;
  for (i = 0; i < 80 * 25; i++) {
    video_memory[i] = blank;
  }

  // Move the hardware cursor back to the start.
  cursor_x = 0;
  cursor_y = 0;
  move_cursor();
}

// Outputs a null-terminated ASCII string to the monitor.
void monitor_write(char *c) {
  monitor_write_with_color(c, COLOR_WHITE);  
}

void monitor_write_with_color(char *c, uint8 color) {
  int i = 0;
  while (c[i]) {
    monitor_put_with_color(c[i++], color);
  }
}

void monitor_write_hex(uint32 n) {
  int32 tmp;

  monitor_write("0x");

  char noZeroes = 1;
  int i;
  for (i = 28; i > 0; i -= 4) {
    tmp = (n >> i) & 0xF;
    if (tmp == 0 && noZeroes != 0) {
      continue;
    }

    if (tmp >= 0xA) {
      noZeroes = 0;
      monitor_put(tmp - 0xA + 'a' );
    } else {
      noZeroes = 0;
      monitor_put( tmp + '0' );
    }
  }
  
  tmp = n & 0xF;
  if (tmp >= 0xA) {
    monitor_put (tmp - 0xA + 'a');
  } else {
    monitor_put (tmp + '0');
  }
}

void monitor_write_dec(uint32 n) {
  if (n == 0) {
    monitor_put('0');
    return;
  }

  int32 acc = n;
  char c[32];
  int i = 0;
  while (acc > 0) {
    c[i] = '0' + acc % 10;
    acc /= 10;
    i++;
  }
  c[i] = 0;

  char c2[32];
  c2[i--] = 0;
  int j = 0;
  while(i >= 0) {
    c2[i--] = c[j++];
  }
  monitor_write(c2);
}
