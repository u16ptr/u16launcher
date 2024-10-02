#include <X11/Xlib.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>

const unsigned int WINDOW_BORDER_WIDTH = 2;
const unsigned int PANEL_WIDTH = 240;
const unsigned int PANEL_HEIGHT = 48;
const unsigned int PANEL_BOTTOM_OFFSET = 0;
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
  int windowX = screenWidth / 2 - PANEL_WIDTH / 2;
  int windowY = screenHeight - PANEL_HEIGHT - PANEL_BOTTOM_OFFSET;
  unsigned int windowWidth = PANEL_WIDTH;
  unsigned int windowHeight = PANEL_HEIGHT;

  testWindow = XCreateSimpleWindow(
    display,
    RootWindow(display, screenNum),
    windowX,
    windowY,
    windowWidth,
    windowHeight,
    WINDOW_BORDER_WIDTH,
    BlackPixel(display, screenNum),
    WhitePixel(display, screenNum)
  );

  XSetWindowAttributes windowAttributes;
  windowAttributes.override_redirect = true;
  XChangeWindowAttributes(display, testWindow, CWOverrideRedirect, &windowAttributes);

  XSelectInput(display, testWindow, KeyPressMask);
  XMapWindow(display, testWindow);

  XEvent event;
  while (true)
  {
    XNextEvent(display, &event);
  }

  XCloseDisplay(display);
  return EXIT_SUCCESS;
}
