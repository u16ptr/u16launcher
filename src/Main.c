#include <X11/Xlib.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>

const char* X_DISPLAY_NAME = ":0";

int main()
{
  Display* display;
  display = XOpenDisplay(X_DISPLAY_NAME);

  if (display == NULL)
  {
    fprintf(stderr, "Cannot connect to X server: %s!\n", X_DISPLAY_NAME);
    return EXIT_FAILURE;
  }

  int screenNum = DefaultScreen(display);
  int screenWidth = DisplayWidth(display, screenNum);
  int screenHeight = DisplayHeight(display, screenNum);

  printf("Width: %d\n", screenWidth);
  printf("Height: %d\n", screenHeight);

  Window testWindow;
  int windowX = 16;
  int windowY = 16;
  unsigned int windowWidth = 128;
  unsigned int windowHeight = 128;

  testWindow = XCreateSimpleWindow(
    display,
    RootWindow(display, screenNum),
    windowX,
    windowY,
    windowWidth,
    windowHeight,
    0,
    BlackPixel(display, screenNum),
    WhitePixel(display, screenNum)
  );
  XMapWindow(display, testWindow);
  XSync(display, false);

  sleep(5);

  XCloseDisplay(display);
  return EXIT_SUCCESS;
}
