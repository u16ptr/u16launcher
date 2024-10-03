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
const unsigned int ICON_SIZE           = 40;
const unsigned int PANEL_HEIGHT        = ICON_SIZE + 2 * GAP_SIZE;
const unsigned int PANEL_BOTTOM_OFFSET = 0;
const unsigned int ITEM_WIDTH          = 160;
const unsigned int ITEM_HEIGHT         = 24;

// Limits
const unsigned int ICON_COUNT_LIMIT = 16;
const unsigned int ICON_NAME_LIMIT  = 48;

// Menu Texts
const char**       panelMenuTexts;
const unsigned int panelMenuItemCount = 2;
const int          panelMenuId = 0;
const char**       iconMenuTexts;
const unsigned int iconMenuItemCount = 4;
const int          iconMenuId = 1;

// Current Menu Struct
struct CurrentMenu
{
  const char*** texts;
  unsigned int itemCount;
  unsigned int id;
};

// Icon Node
struct IconNode
{
  char* name;
  unsigned int id;
  struct IconNode* next;
};

// Window and X11 Settings
const bool  SHOW_UNDER     = false;
const char* X_DISPLAY_NAME = ":0";

// Debugging
const bool DEBUG_FUNCTIONS        = true;
const bool DEBUG_MOTION_FUNCTIONS = false;

// Initializer Functions
void initializeColors();
void initializeDisplay();
void initilalizeMenuTexts();
void initializeMenu(int screenNum, unsigned long cBackground, unsigned int cBorder);
void initializePanel(int screenNum, int panelX, int panelY, int panelWidth, unsigned long cBackground, unsigned int cBorder);

// Visibility Functions
void showPanel();
void refreshPanel(int iconCount, int screenWidth, int screenHeight);
void showMenu();
void showMenuAt(int x, int y, const char** menuTexts, unsigned int itemCount);
void clearMenu();
void hideMenu();

// Render Functions
void renderIconAtIndex(int index);
void renderIconHoverAtIndex(int index);
void renderIcons(int hoveredIndex);
void renderIconIdAtIndex(int index);
void renderIconIds(struct IconNode* iconList);
void renderMenuHoverAtIndex(int index);
void renderMenuItems(const char** menuItems, unsigned int itemCount);

// Calculation Functions
int calculatePanelWidth(int iconCount);
int calculateIconIndexFromMouseX(int relMouseX, int iconCount);
int calculateItemIndexFromMouseY(int relMouseY, unsigned int itemCount);

// Icon Functions
struct IconNode* createIcon(const char* name);
void             addIcon(const char* name);
struct IconNode* getIconByIndex(int index);
unsigned int     getIconCount();

// Icon Linked List
struct IconNode* iconList = NULL;
unsigned int     currentIconId = -1;

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


  // Icon Linked List
  int currentId = 0;
  addIcon("Icon 1");
  addIcon("Icon 2");
  addIcon("Icon 3");
  addIcon("Icon 4");
  addIcon("Icon 5");

  int iconCount = getIconCount(iconList);
  int panelWidth = calculatePanelWidth(iconCount);

  initializePanel(
    screenNum,
    screenWidth / 2 - panelWidth / 2,
    screenHeight - PANEL_HEIGHT - PANEL_BOTTOM_OFFSET - WINDOW_BORDER_WIDTH,
    panelWidth,
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
    .id = -1,
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
            renderIconIds(iconList);
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
            int calculatedIndex = calculateIconIndexFromMouseX(event.xmotion.x, iconCount);
            if (hoveredPanelIndex != calculatedIndex)
            {
              renderIconAtIndex(hoveredPanelIndex);
              renderIconIdAtIndex(hoveredPanelIndex);
              hoveredPanelIndex = calculatedIndex;
              renderIconHoverAtIndex(hoveredPanelIndex);
              renderIconIdAtIndex(hoveredPanelIndex);
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
            renderIconIdAtIndex(hoveredPanelIndex);
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
            if (event.xbutton.window == menuWindow && event.xbutton.y < currentMenu.itemCount * ITEM_HEIGHT)
            {
              int actionIndex = event.xbutton.y / ITEM_HEIGHT;
              if (currentMenu.id == iconMenuId)
              {
                hideMenu();
                menuShown = false;
                hoveredMenuIndex = -1;
              }
              else if (currentMenu.id == panelMenuId)
              {
                if (actionIndex == 0 && iconCount < ICON_COUNT_LIMIT)
                {
                  addIcon("Icon");
                  iconCount++;
                  refreshPanel(iconCount, screenWidth, screenHeight);
                }
                else if (actionIndex == 1)
                {
                  running = false;
                }
                hideMenu();
                menuShown = false;
                hoveredMenuIndex = -1;
              }
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
              currentMenu.id = panelMenuId;
            }
            else
            {
              currentMenu.texts = &iconMenuTexts;
              currentMenu.itemCount = iconMenuItemCount;
              currentMenu.id = iconMenuId;
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
  panelMenuTexts = (const char**)malloc(2 * sizeof(char*));
  panelMenuTexts[0] = "Add item";
  panelMenuTexts[1] = "Terminate u16panel";
  iconMenuTexts = (const char**)malloc(4 * sizeof(char*));
  iconMenuTexts[0] = "Unpin";
  iconMenuTexts[1] = "Move left";
  iconMenuTexts[2] = "Move right";
  iconMenuTexts[3] = "Launch";
}

void initializePanel(int screenNum, int panelX, int panelY, int panelWidth, unsigned long cBackground, unsigned int cBorder)
{
  if (DEBUG_FUNCTIONS) printf("%s\n", __func__);
  panelWindow = XCreateSimpleWindow(
    display,
    RootWindow(display, screenNum),
    panelX,
    panelY,
    panelWidth,
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

void refreshPanel(int iconCount, int screenWidth, int screenHeight)
{
  if (DEBUG_FUNCTIONS) printf("%s\n", __func__);
  int panelWidth = calculatePanelWidth(iconCount);
  XResizeWindow(
    display,
    panelWindow,
    panelWidth,
    PANEL_HEIGHT
  );
  XMoveWindow(
    display,
    panelWindow,
    screenWidth / 2 - panelWidth / 2,
    screenHeight - PANEL_HEIGHT - PANEL_BOTTOM_OFFSET - WINDOW_BORDER_WIDTH
  );
  XClearWindow(display, panelWindow);
}

void showMenu()
{
  if (DEBUG_FUNCTIONS) printf("%s\n", __func__);
  XMapWindow(display, menuWindow);
}

void showMenuAt(int x, int y, const char** menuTexts, unsigned int itemCount)
{
  if (DEBUG_FUNCTIONS) printf("%s\n", __func__);
  XResizeWindow(display, menuWindow, ITEM_WIDTH, itemCount * ITEM_HEIGHT);
  XMoveWindow(display, menuWindow, x, y - itemCount * ITEM_HEIGHT);
  showMenu();
}

void clearMenu()
{
  if (DEBUG_FUNCTIONS) printf("%s\n", __func__);
  XClearWindow(display, menuWindow);
}

void hideMenu()
{
  if (DEBUG_FUNCTIONS) printf("%s\n", __func__);
  XUnmapWindow(display, menuWindow);
}

void renderIconAtIndex(int index)
{
  if (DEBUG_FUNCTIONS) printf("%s\n", __func__);
  int iconX = index * (ICON_SIZE + GAP_SIZE) + GAP_SIZE;
  int iconY = GAP_SIZE;
  XSetForeground(display, panelGC, cIconBackground);
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

  if (iconList == NULL) return;
  struct IconNode* current = iconList;
  int index = 0;
  while (current != NULL)
  {
    renderIconAtIndex(index);
    index++;
    current = current->next;
  }
}

void renderIconIdAtIndex(int index)
{
  if (DEBUG_FUNCTIONS) printf("%s\n", __func__);
  struct IconNode* icon = getIconByIndex(index);
  if (icon == NULL) return;
  XSetForeground(display, panelGC, cMenuForeground);
  int iconX = index * (ICON_SIZE + GAP_SIZE) + GAP_SIZE;
  char* idBuffer = (char*)malloc(sizeof(4));
  snprintf(idBuffer, 4, "%d", icon->id);
  XDrawString(display, panelWindow, panelGC, iconX + 4, GAP_SIZE + 10 + 4, idBuffer, strlen(idBuffer));
  free(idBuffer);
}

void renderIconIds(struct IconNode* iconList)
{
  if (DEBUG_FUNCTIONS) printf("%s\n", __func__);
  if (iconList == NULL) return;

  struct IconNode* current = iconList;
  int index = 0;
  int iconY = GAP_SIZE;

  while (current != NULL)
  {
    int iconX = index * (ICON_SIZE + GAP_SIZE) + GAP_SIZE;
    renderIconIdAtIndex(index);
    index++;
    current = current->next;
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

int calculatePanelWidth(int iconCount)
{
  if (DEBUG_FUNCTIONS) printf("%s\n", __func__);
  return GAP_SIZE * 2 + (iconCount - 1) * GAP_SIZE + iconCount * ICON_SIZE;
}

int calculateIconIndexFromMouseX(int relMouseX, int iconCount)
{
  if (DEBUG_MOTION_FUNCTIONS) printf("%s\n", __func__);
  int iconIndex = 0;
  if (relMouseX < 0) { return -1; }
  if (
    relMouseX > GAP_SIZE + GAP_SIZE / 2 + ICON_SIZE
  )
  { iconIndex = (relMouseX - GAP_SIZE / 2) / (ICON_SIZE + GAP_SIZE); }
  if (iconIndex == iconCount) iconIndex--;
  return iconIndex;
}

int calculateItemIndexFromMouseY(int relMouseY, unsigned int itemCount)
{
  if (DEBUG_MOTION_FUNCTIONS) printf("%s\n", __func__);
  if (relMouseY <= ITEM_HEIGHT) return 0;
  if (relMouseY < 0 || relMouseY > ITEM_HEIGHT * itemCount) return -1;
  return relMouseY / ITEM_HEIGHT;
}

struct IconNode* createIcon(const char* name)
{
  if (DEBUG_FUNCTIONS) printf("%s\n", __func__);
  char* allocatedName = malloc(ICON_NAME_LIMIT * sizeof(char));
  strcpy(allocatedName, name);
  struct IconNode* icon = (struct IconNode*)malloc(sizeof(struct IconNode));
  icon->name = allocatedName;
  icon->id = -1;
  icon->next = NULL;
  return icon;
}

void addIcon(const char* name)
{
  if (DEBUG_FUNCTIONS) printf("%s\n", __func__);
  struct IconNode* newNode = createIcon(name);
  currentIconId++;
  newNode->id = currentIconId;
  if (iconList == NULL)
  {
    iconList = newNode;
    return;
  }
  struct IconNode* current = iconList;
  while (current->next != NULL)
  {
    current = current->next;
  }
  current->next = newNode;
}

struct IconNode* getIconByIndex(int index)
{
  if (DEBUG_FUNCTIONS) printf("%s\n", __func__);
  int currentIndex = 0;
  if (iconList == NULL) return NULL;
  struct IconNode* current = iconList;
  while (current != NULL)
  {
    if (currentIndex == index)
    {
      return current;
    }
    current = current->next;
    currentIndex++;
  }
  return NULL;
}

unsigned int getIconCount()
{
  if (DEBUG_FUNCTIONS) printf("%s\n", __func__);
  int count = 0;
  if (iconList == NULL) return count;
  struct IconNode* current = iconList;
  while (current != NULL)
  {
    count++;
    current = current->next;
  }
  return count;
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
