#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

// Global Variables
Display* display;
Window panelWindow;
Window menuWindow;
GC panelGC;
GC menuGC;

// Colors
unsigned long cPanelBackground;
unsigned long cPanelBorder;
unsigned long cMenuBackground;
unsigned long cMenuBorder;
unsigned long cIconHover;
unsigned long cIconBackground;

// Dimensions
const unsigned int WINDOW_BORDER_WIDTH = 1;
const unsigned int GAP_SIZE            = 4;
const unsigned int ICON_COUNT          = 5;
const unsigned int ICON_SIZE           = 40;
const unsigned int PANEL_WIDTH         = GAP_SIZE * 2 + (ICON_COUNT - 1) * GAP_SIZE + ICON_COUNT * ICON_SIZE;
const unsigned int PANEL_HEIGHT        = ICON_SIZE + 2 * GAP_SIZE;
const unsigned int PANEL_BOTTOM_OFFSET = 0;
const unsigned int ITEM_WIDTH          = 160;
const unsigned int ITEM_HEIGHT         = 24;

// Texts
const char* TERMINATE_TEXT = "Terminate u16panel";

// Window and X11 Settings
const bool  SHOW_UNDER     = false;
const char* X_DISPLAY_NAME = ":0";

// Debugging
const bool DEBUG_FUNCTIONS = true;

// Initializer Functions
void initializeColors();
void initializeDisplay();
void initializeMenu(int screenNum, unsigned long cBackground, unsigned int cBorder);
void initializePanel(int screenNum, int panelX, int panelY, unsigned long cBackground, unsigned int cBorder);

// Visibility Functions
void showPanel();
void showMenu();
void showMenuAt(int x, int y);
void hideMenu();

// Render Functions
void renderIconAtIndex(int index);
void renderHoverAtIndex(int index);
void renderIcons(int hoveredIndex);

// Calculation Functions
int calculateIconIndexFromMouseX(int relMouseX);

// Utility Functions
unsigned long calculateRGB(uint8_t red, u_int8_t green, uint8_t blue);

// Cleanup Functions
void freeObjects();

int main()
{
  initializeColors();
  initializeDisplay();

  int screenNum = DefaultScreen(display);
  int screenWidth = DisplayWidth(display, screenNum);
  int screenHeight = DisplayHeight(display, screenNum);

  printf("%lu\n", cPanelBackground);

  initializePanel(
    screenNum,
    screenWidth / 2 - PANEL_WIDTH / 2,
    screenHeight - PANEL_HEIGHT - PANEL_BOTTOM_OFFSET - WINDOW_BORDER_WIDTH,
    cPanelBackground,
    cPanelBorder
  );
  initializeMenu(
    screenNum,
    cPanelBackground,
    cPanelBorder
  );
  showPanel();

  XEvent event;
  int hoveredIndex = -1;
  bool running = true;
  bool menuShown = false;

  while (running)
  {
    XNextEvent(display, &event);
    switch (event.type)
    {
      case Expose:
        {
          if (event.xexpose.window == panelWindow)
          {
            renderIcons(-1);
          }
          else if (event.xexpose.window == menuWindow)
          {
            XDrawString(display, menuWindow, menuGC, 6, 16, TERMINATE_TEXT, strlen(TERMINATE_TEXT));
          }
          break;
        }
      case MotionNotify:
        {
          if (event.xmotion.window != panelWindow || menuShown) break;
          int calculatedIndex = calculateIconIndexFromMouseX(event.xmotion.x);
          if (hoveredIndex != calculatedIndex)
          {
            renderIconAtIndex(hoveredIndex);
            hoveredIndex = calculatedIndex;
          }
          renderHoverAtIndex(hoveredIndex);
          break;
        }
      case LeaveNotify:
        {
          XSetForeground(display, panelGC, cIconBackground);
          renderIconAtIndex(hoveredIndex);
          hoveredIndex = -1;
          break;
        }
      case ButtonPress:
        {
          if (event.xbutton.button == Button1)
          {
            if (event.xbutton.window == menuWindow)
            {
              running = false;
              break;
            }
            else
            {
              if (!menuShown) break;
              hideMenu();
              menuShown = false;
            }
          }
          else if (event.xbutton.button == Button3)
          {
            showMenuAt(event.xbutton.x_root, event.xbutton.y_root - ITEM_HEIGHT);
            menuShown = true;
          }
          break;
        }
    }
  }

  freeObjects();
  return EXIT_SUCCESS;
}

void initializeColors()
{
  cPanelBackground = calculateRGB(17, 17, 17);
  cPanelBorder = calculateRGB(139, 212, 156);
  cMenuBackground = calculateRGB(17, 17, 17);
  cMenuBorder = calculateRGB(139, 212, 156);
  cIconBackground = calculateRGB(34, 34, 34);
  cIconHover = calculateRGB(51, 51, 51);
}

void initializeDisplay()
{
  if (DEBUG_FUNCTIONS) printf("%s\n", __func__);
  display = XOpenDisplay(X_DISPLAY_NAME);
  if (display == NULL)
  {
    fprintf(stderr, "Cannot connect to X server: %s!\n", X_DISPLAY_NAME);
    exit(EXIT_FAILURE);
  }
}

void initializePanel(int screenNum, int panelX, int panelY, unsigned long cBackground, unsigned int cBorder)
{
  if (DEBUG_FUNCTIONS) printf("%s\n", __func__);
  panelWindow = XCreateSimpleWindow(
    display,
    RootWindow(display, screenNum),
    panelX,
    panelY,
    PANEL_WIDTH,
    PANEL_HEIGHT,
    WINDOW_BORDER_WIDTH,
    cBorder,
    cBackground
  );
  panelGC = XCreateGC(display, panelWindow, 0, 0);

  XSetWindowAttributes panelAttributes;
  panelAttributes.override_redirect = true;
  XChangeWindowAttributes(display, panelWindow, CWOverrideRedirect, &panelAttributes);
  XSelectInput(display, panelWindow, ExposureMask | ButtonPressMask | PointerMotionMask | LeaveWindowMask);

  if (SHOW_UNDER) XLowerWindow(display, panelWindow);
}

void initializeMenu(int screenNum, unsigned long cBackground, unsigned int cBorder)
{
  if (DEBUG_FUNCTIONS) printf("%s\n", __func__);
  menuWindow = XCreateSimpleWindow(
    display,
    RootWindow(display, screenNum),
    0,
    0,
    ITEM_WIDTH,
    ITEM_HEIGHT,
    WINDOW_BORDER_WIDTH,
    cBorder,
    cBackground
  );
  menuGC = XCreateGC(display, menuWindow, 0, 0);
  XSetForeground(display, menuGC, calculateRGB(255, 255, 255));

  XSetWindowAttributes menuAttributes;
  menuAttributes.override_redirect = true;
  XChangeWindowAttributes(display, menuWindow, CWOverrideRedirect, &menuAttributes);
  XSelectInput(display, menuWindow, ExposureMask | ButtonPressMask);
}

void showPanel()
{
  if (DEBUG_FUNCTIONS) printf("%s\n", __func__);
  XMapWindow(display, panelWindow);
  XClearWindow(display, panelWindow);
}

void showMenu()
{
  if (DEBUG_FUNCTIONS) printf("%s\n", __func__);
  XMapWindow(display, menuWindow);
}

void showMenuAt(int x, int y)
{
  XMoveWindow(display, menuWindow, x, y);
  showMenu();
}

void hideMenu()
{
  XUnmapWindow(display, menuWindow);
}

void renderIconAtIndex(int index)
{
  if (DEBUG_FUNCTIONS) printf("%s\n", __func__);
  int iconX = index * (ICON_SIZE + GAP_SIZE) + GAP_SIZE;
  int iconY = GAP_SIZE;
  XFillRectangle(
    display,
    panelWindow,
    panelGC,
    iconX,
    iconY,
    ICON_SIZE,
    ICON_SIZE
  );
}

void renderHoverAtIndex(int index)
{
  if (DEBUG_FUNCTIONS) printf("%s\n", __func__);
  int hoverX = index * (ICON_SIZE + GAP_SIZE) + GAP_SIZE;
  int hoverY = GAP_SIZE;
  XSetForeground(display, panelGC, cIconHover);
  XFillRectangle(
    display,
    panelWindow,
    panelGC,
    hoverX,
    hoverY,
    ICON_SIZE,
    ICON_SIZE
  );
  XSetForeground(display, panelGC, cIconBackground);
}

void renderIcons(int hoveredIndex)
{
  if (DEBUG_FUNCTIONS) printf("%s\n", __func__);
  XClearWindow(display, panelWindow);

  int rootX, rootY;
  int winX, winY;
  Window rootReturn, childReturn;
  unsigned int mask;

  XSetForeground(display, panelGC, cIconBackground);
  for (int i = 0; i < ICON_COUNT; i++)
  {
    renderIconAtIndex(i);
  }
}

int calculateIconIndexFromMouseX(int relMouseX)
{
  if (DEBUG_FUNCTIONS) printf("%s\n", __func__);
  int iconIndex = 0;
  if (relMouseX < 0) { return -1; }
  if (
    relMouseX > GAP_SIZE + GAP_SIZE / 2 + ICON_SIZE
  )
  { iconIndex = (relMouseX - GAP_SIZE / 2) / (ICON_SIZE + GAP_SIZE); }
  if (iconIndex == ICON_COUNT) iconIndex--;
  return iconIndex;
}

unsigned long calculateRGB(uint8_t red, u_int8_t green, uint8_t blue)
{
  if (DEBUG_FUNCTIONS) printf("%s\n", __func__);
  return blue + (green << 8) + (red << 16);
}

void freeObjects()
{
  if (DEBUG_FUNCTIONS) printf("%s\n", __func__);
  XFreeGC(display, panelGC);
  XFreeGC(display, menuGC);
  XCloseDisplay(display);
}
