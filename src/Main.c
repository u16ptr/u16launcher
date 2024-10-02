#include <X11/Xlib.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>

const unsigned int WINDOW_BORDER_WIDTH = 2;
const unsigned int PANEL_WIDTH = 240;
const unsigned int PANEL_HEIGHT = 48;
const unsigned int PANEL_BOTTOM_OFFSET = 0;
const char* X_DISPLAY_NAME = ":0";

unsigned long calculateRGB(uint8_t red, u_int8_t green, uint8_t blue);

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

  Window panelWindow;
  int windowX = screenWidth / 2 - PANEL_WIDTH / 2;
  int windowY = screenHeight - PANEL_HEIGHT - PANEL_BOTTOM_OFFSET;
  unsigned int windowWidth = PANEL_WIDTH;
  unsigned int windowHeight = PANEL_HEIGHT;

  unsigned long cBackground = calculateRGB(17, 17, 17);
  unsigned long cBorder = calculateRGB(139, 212, 156);

  panelWindow = XCreateSimpleWindow(
    display,
    RootWindow(display, screenNum),
    windowX,
    windowY,
    windowWidth,
    windowHeight,
    WINDOW_BORDER_WIDTH,
    cBorder,
    cBackground
  );

  GC graphicsContext;
  graphicsContext = XCreateGC(display, panelWindow, 0, 0);

  XSetWindowAttributes windowAttributes;
  windowAttributes.override_redirect = true;
  XChangeWindowAttributes(display, panelWindow, CWOverrideRedirect, &windowAttributes);

  XLowerWindow(display, panelWindow);
  XSelectInput(display, panelWindow, ExposureMask | KeyPressMask);
  XMapWindow(display, panelWindow);
  XClearWindow(display, panelWindow);

  XEvent event;
  while (true)
  {
    XNextEvent(display, &event);
  }

  XFreeGC(display, graphicsContext);
  XCloseDisplay(display);
  return EXIT_SUCCESS;
}

unsigned long calculateRGB(uint8_t red, u_int8_t green, uint8_t blue)
{
  return blue + (green << 8) + (red << 16);
}
