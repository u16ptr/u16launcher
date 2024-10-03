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
unsigned long cMenuForeground;
unsigned long cMenuHover;
unsigned long cMenuBorder;
unsigned long cIconBackground;
unsigned long cIconHover;

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

// Menu Texts
const char**       panelMenuTexts;
const unsigned int panelMenuItemCount = 3;
const char**       iconMenuTexts;
const unsigned int iconMenuItemCount = 2;

// Current Menu Struct
struct CurrentMenu
{
  const char*** texts;
  unsigned int itemCount;
};

// Window and X11 Settings
const bool  SHOW_UNDER     = false;
const char* X_DISPLAY_NAME = ":0";

// Debugging
const bool DEBUG_FUNCTIONS = true;

// Initializer Functions
void initializeColors();
void initializeDisplay();
void initilalizeMenuTexts();
void initializeMenu(int screenNum, unsigned long cBackground, unsigned int cBorder);
void initializePanel(int screenNum, int panelX, int panelY, unsigned long cBackground, unsigned int cBorder);

// Visibility Functions
void showPanel();
void showMenu();
void showMenuAt(int x, int y, const char** menuTexts, unsigned int itemCount);
void clearMenu();
void hideMenu();

// Render Functions
void renderIconAtIndex(int index);
void renderIconHoverAtIndex(int index);
void renderIcons(int hoveredIndex);
void renderMenuHoverAtIndex(int index);
void renderMenuItems(const char** menuItems, unsigned int itemCount);

// Calculation Functions
int calculateIconIndexFromMouseX(int relMouseX);
int calculateItemIndexFromMouseY(int relMouseY, unsigned int itemCount);

// Utility Functions
unsigned long calculateRGB(uint8_t red, u_int8_t green, uint8_t blue);

// Cleanup Functions
void freeObjects();

int main()
{
  initializeColors();
  initializeDisplay();
  initilalizeMenuTexts();

  int screenNum = DefaultScreen(display);
  int screenWidth = DisplayWidth(display, screenNum);
  int screenHeight = DisplayHeight(display, screenNum);

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
  int hoveredPanelIndex = -1;
  int hoveredMenuIndex = -1;
  struct CurrentMenu currentMenu =
  {
    .texts = NULL,
    .itemCount = 0,
  };
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
          else if (event.xexpose.window == menuWindow && currentMenu.texts != NULL)
          {
            clearMenu();
            renderMenuItems(*currentMenu.texts, currentMenu.itemCount);
          }
          break;
        }
      case MotionNotify:
        {
          if (event.xmotion.window == panelWindow && !menuShown)
          {
            int calculatedIndex = calculateIconIndexFromMouseX(event.xmotion.x);
            if (hoveredPanelIndex != calculatedIndex)
            {
              renderIconAtIndex(hoveredPanelIndex);
              hoveredPanelIndex = calculatedIndex;
              renderIconHoverAtIndex(hoveredPanelIndex);
            }
          }
          else if (event.xmotion.window == menuWindow && currentMenu.texts != NULL)
          {
            int calculatedIndex = calculateItemIndexFromMouseY(event.xmotion.y, currentMenu.itemCount);
            if (hoveredMenuIndex != calculatedIndex)
            {
              hoveredMenuIndex = calculatedIndex;
              clearMenu();
              renderMenuHoverAtIndex(hoveredMenuIndex);
              renderMenuItems(*(currentMenu.texts), currentMenu.itemCount);
            }
          }
          break;
        }
      case LeaveNotify:
        {
          if (((XCrossingEvent*)&event)->window == panelWindow)
          {
            XSetForeground(display, panelGC, cIconBackground);
            renderIconAtIndex(hoveredPanelIndex);
            hoveredPanelIndex = -1;
          }
          else if (((XCrossingEvent*)&event)->window == menuWindow)
          {
            clearMenu();
            renderMenuItems(*(currentMenu.texts), currentMenu.itemCount);
            hoveredMenuIndex = -1;
          }
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
              hoveredMenuIndex = -1;
            }
          }
          else if (event.xbutton.button == Button3)
          {
            if (((XButtonEvent*)&event)->state & ShiftMask)
            {
              currentMenu.texts = &panelMenuTexts;
              currentMenu.itemCount = panelMenuItemCount;
            }
            else
            {
              currentMenu.texts = &iconMenuTexts;
              currentMenu.itemCount = iconMenuItemCount;
            }
            showMenuAt(event.xbutton.x_root, event.xbutton.y_root, *currentMenu.texts, currentMenu.itemCount);
            menuShown = true;
          }
          break;
        }
    }
  }

  free(panelMenuTexts);
  free(iconMenuTexts);
  freeObjects();
  return EXIT_SUCCESS;
}

void initializeColors()
{
  if (DEBUG_FUNCTIONS) printf("%s\n", __func__);
  cPanelBackground = calculateRGB(17, 17, 17);
  cPanelBorder = calculateRGB(139, 212, 156);
  cMenuBackground = calculateRGB(17, 17, 17);
  cMenuForeground = calculateRGB(255, 255, 255);
  cMenuHover = calculateRGB(34, 34, 34);
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

void initilalizeMenuTexts()
{
  if (DEBUG_FUNCTIONS) printf("%s\n", __func__);
  panelMenuTexts = (const char**)malloc(3 * sizeof(char*));
  panelMenuTexts[0] = "Add item";
  panelMenuTexts[1] = "Terminate u16panel";
  panelMenuTexts[2] = "Terminate u16panel";
  iconMenuTexts = (const char**)malloc(2 * sizeof(char*));
  iconMenuTexts[0] = "Unpin";
  iconMenuTexts[1] = "Launch";
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
  XSelectInput(display, panelWindow, ExposureMask | ShiftMask | ButtonPressMask | PointerMotionMask | LeaveWindowMask);

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
    100,
    WINDOW_BORDER_WIDTH,
    cBorder,
    cBackground
  );
  menuGC = XCreateGC(display, menuWindow, 0, 0);
  XSetForeground(display, menuGC, cMenuForeground);

  XSetWindowAttributes menuAttributes;
  menuAttributes.override_redirect = true;
  XChangeWindowAttributes(display, menuWindow, CWOverrideRedirect, &menuAttributes);
  XSelectInput(display, menuWindow, ExposureMask | ButtonPressMask | PointerMotionMask | LeaveWindowMask);
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

void showMenuAt(int x, int y, const char** menuTexts, unsigned int itemCount)
{
  XResizeWindow(display, menuWindow, ITEM_WIDTH, itemCount * ITEM_HEIGHT);
  XMoveWindow(display, menuWindow, x, y - itemCount * ITEM_HEIGHT);
  showMenu();
}

void clearMenu()
{
  XClearWindow(display, menuWindow);
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

void renderIconHoverAtIndex(int index)
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

void renderMenuHoverAtIndex(int index)
{
  if (DEBUG_FUNCTIONS) printf("%s\n", __func__);
  int hoverY = GAP_SIZE;
  XSetForeground(display, menuGC, cMenuHover);
  XFillRectangle(
    display,
    menuWindow,
    panelGC,
    0,
    index * ITEM_HEIGHT,
    ITEM_WIDTH,
    ITEM_HEIGHT
  );
  XSetForeground(display, menuGC, cMenuForeground);
}

void renderMenuItems(const char** menuItems, unsigned int itemCount)
{
  if (DEBUG_FUNCTIONS) printf("%s\n", __func__);
  for (int i = 0; i < itemCount; i++)
  {
    XDrawString(display, menuWindow, menuGC, 6, 16 + i * ITEM_HEIGHT, menuItems[i], strlen(menuItems[i]));
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

int calculateItemIndexFromMouseY(int relMouseY, unsigned int itemCount)
{
  if (DEBUG_FUNCTIONS) printf("%s\n", __func__);
  if (relMouseY <= ITEM_HEIGHT) return 0;
  if (relMouseY < 0 || relMouseY > ITEM_HEIGHT * itemCount) return -1;
  return relMouseY / ITEM_HEIGHT;
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
