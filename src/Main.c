#include <X11/Xlib.h>
#include <X11/xpm.h>
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
const int WINDOW_BORDER_WIDTH = 1;
const int GAP_SIZE            = 4;
const int ICON_BOX_SIZE       = 40;
const int ICON_SIZE           = 32;
const int ICON_INSET          = (ICON_BOX_SIZE - ICON_SIZE) / 2;
const int PANEL_HEIGHT        = ICON_BOX_SIZE + 2 * GAP_SIZE;
const int PANEL_BOTTOM_OFFSET = 0;
const int ITEM_WIDTH          = 160;
const int ITEM_HEIGHT         = 24;

// Limits
const int ICON_COUNT_LIMIT = 16;
const int ICON_NAME_LIMIT  = 48;

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
  int itemCount;
  int id;
};

// Icon Node
struct IconNode
{
  Pixmap pixelMap;
  Pixmap mask;
  char* name;
  int id;
  struct IconNode* next;
};

// Window and X11 Settings
const bool  SHOW_UNDER     = false;
const char* X_DISPLAY_NAME = ":0";

// Debugging
const bool DEBUG_FUNCTIONS        = false;
const bool DEBUG_MOTION_FUNCTIONS = false;
const bool DEBUG_RENDER_ICON_IDS  = false;

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
void showMenuAt(int x, int y, int itemCount);
void clearMenu();
void hideMenu();

// Pointer Functions
void grabPointer();
void releasePointer();

// Render Functions
void renderIconAtIndex(int index);
void renderIconHoverAtIndex(int index);
void renderIcons();
void renderIconPixelMapAtIndex(int index);
void renderIconPixelMaps(struct IconNode* iconList);
void renderIconIdAtIndex(int index);
void renderIconIds(struct IconNode* iconList);
void renderMenuHoverAtIndex(int index);
void renderMenuItems(const char** menuItems, int itemCount);

// Calculation Functions
int calculatePanelWidth(int iconCount);
int calculateIconIndexFromMouseX(int relMouseX, int iconCount);
int calculateItemIndexFromMouseY(int relMouseY, int itemCount);

// Icon Functions
void             loadPixelMap(Pixmap* map, Pixmap* mask, const char* filePath, int* width, int* height);
void             scalePixelMap(Pixmap* baseMap, Pixmap* baseMask, int baseWidth, int baseHeight, int newWidth, int newHeight);
struct IconNode* createIcon(const char* name);
void             addIcon(const char* name);
struct IconNode* getIconByIndex(int index);
unsigned int     getIconCount();
void             moveIconToLeftByIndex(int index);
void             moveIconToRightByIndex(int index);
void             removeIconByIndex(int index);
unsigned int     generateIconId();

// Program Functions
void openTerminal();

// Utility Functions
unsigned long calculateRGB(uint8_t red, u_int8_t green, uint8_t blue);

// Cleanup Functions
void freePixelMaps();
void freeTexts();
void freeXObjects();

// Icon Linked List
struct IconNode* iconList = NULL;

int main()
{
  initializeColors();
  initializeDisplay();
  initilalizeMenuTexts();

  int screenNum = DefaultScreen(display);
  int screenWidth = DisplayWidth(display, screenNum);
  int screenHeight = DisplayHeight(display, screenNum);

  int panelWidth = calculatePanelWidth(0);
  initializePanel(
    screenNum,
    screenWidth / 2 - panelWidth / 2,
    screenHeight - PANEL_HEIGHT - PANEL_BOTTOM_OFFSET - WINDOW_BORDER_WIDTH,
    panelWidth,
    cPanelBackground,
    cPanelBorder
  );

  // Icon Linked List
  addIcon("Icon 1");
  addIcon("Icon 2");
  addIcon("Icon 3");
  addIcon("Icon 4");
  addIcon("Icon 5");

  int iconCount = getIconCount(iconList);
  panelWidth = calculatePanelWidth(iconCount);
  refreshPanel(iconCount, screenWidth, screenHeight);

  initializeMenu(
    screenNum,
    cPanelBackground,
    cPanelBorder
  );
  showPanel();

  XEvent event;
  int hoveredPanelIndex = -1;
  int hoveredMenuIndex = -1;
  int lastClickedPanelIndex = -1;
  struct CurrentMenu currentMenu =
  {
    .texts = NULL,
    .itemCount = 0,
    .id = -1,
  };
  bool running = true;
  bool menuShown = false;
  bool mouseInsideMenu = false;

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
            renderIconPixelMaps(iconList);
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
              renderIconPixelMapAtIndex(hoveredPanelIndex);
              renderIconIdAtIndex(hoveredPanelIndex);
              hoveredPanelIndex = calculatedIndex;
              renderIconHoverAtIndex(hoveredPanelIndex);
              renderIconPixelMapAtIndex(hoveredPanelIndex);
              renderIconIdAtIndex(hoveredPanelIndex);
            }
          }
          else if (event.xmotion.window == menuWindow && currentMenu.texts != NULL && mouseInsideMenu)
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
      case EnterNotify:
        {
          if (event.xcrossing.window == menuWindow)
          {
            mouseInsideMenu = true;
          }
          break;
        }
      case LeaveNotify:
        {
          if (event.xcrossing.window == panelWindow)
          {
            XSetForeground(display, panelGC, cIconBackground);
            renderIconAtIndex(hoveredPanelIndex);
            renderIconPixelMapAtIndex(hoveredPanelIndex);
            renderIconIdAtIndex(hoveredPanelIndex);
            hoveredPanelIndex = -1;
          }
          else if (event.xcrossing.window == menuWindow)
          {
            clearMenu();
            renderMenuItems(*(currentMenu.texts), currentMenu.itemCount);
            hoveredMenuIndex = -1;
            mouseInsideMenu = false;
          }
          break;
        }
      case ButtonPress:
        {
          if (event.xbutton.button == Button1)
          {
            if (event.xbutton.window == menuWindow && event.xbutton.y < currentMenu.itemCount * ITEM_HEIGHT && mouseInsideMenu)
            {
              int actionIndex = event.xbutton.y / ITEM_HEIGHT;
              if (currentMenu.id == iconMenuId)
              {
                if (actionIndex == 0)
                {
                  removeIconByIndex(lastClickedPanelIndex);
                  iconCount--;
                  lastClickedPanelIndex = -1;
                  refreshPanel(iconCount, screenWidth, screenHeight);
                }
                else if (actionIndex == 1)
                {
                  moveIconToLeftByIndex(lastClickedPanelIndex);
                  lastClickedPanelIndex = -1;
                  refreshPanel(iconCount, screenWidth, screenHeight);
                }
                else if (actionIndex == 2)
                {
                  moveIconToRightByIndex(lastClickedPanelIndex);
                  lastClickedPanelIndex = -1;
                  refreshPanel(iconCount, screenWidth, screenHeight);
                }
                else if (actionIndex == 3)
                {
                  openTerminal();
                }
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
              if (!menuShown)
              {
                openTerminal();
              }
              else
              {
                hideMenu();
                menuShown = false;
                hoveredMenuIndex = -1;
              }
            }
          }
          else if (event.xbutton.button == Button3)
          {
            if (menuShown)
            {
              if (!mouseInsideMenu)
              {
                hideMenu();
                menuShown = false;
                hoveredMenuIndex = -1;
              }
              break;
            }
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
            showMenuAt(event.xbutton.x_root, event.xbutton.y_root, currentMenu.itemCount);
            lastClickedPanelIndex = calculateIconIndexFromMouseX(event.xbutton.x, iconCount);
            menuShown = true;
            mouseInsideMenu = true;
          }
          break;
        }
    }
  }

  freePixelMaps();
  freeTexts();
  freeXObjects();
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
  XSelectInput(display, menuWindow, ExposureMask | ButtonPressMask | PointerMotionMask | EnterWindowMask | LeaveWindowMask);
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
  grabPointer();
}

void showMenuAt(int x, int y, int itemCount)
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
  releasePointer();
}

void grabPointer()
{
  XGrabPointer(
    display,
    menuWindow,
    false,
    ButtonPressMask | ButtonReleaseMask | PointerMotionMask | EnterWindowMask | LeaveWindowMask,
    GrabModeAsync,
    GrabModeAsync,
    None,
    None,
    CurrentTime
  );
}

void releasePointer()
{
  XUngrabPointer(display, CurrentTime);
}

void renderIconAtIndex(int index)
{
  if (DEBUG_FUNCTIONS) printf("%s\n", __func__);
  int iconX = index * (ICON_BOX_SIZE + GAP_SIZE) + GAP_SIZE;
  int iconY = GAP_SIZE;
  XSetForeground(display, panelGC, cIconBackground);
  XFillRectangle(
    display,
    panelWindow,
    panelGC,
    iconX,
    iconY,
    ICON_BOX_SIZE,
    ICON_BOX_SIZE
  );
}

void renderIconHoverAtIndex(int index)
{
  if (DEBUG_FUNCTIONS) printf("%s\n", __func__);
  int hoverX = index * (ICON_BOX_SIZE + GAP_SIZE) + GAP_SIZE;
  int hoverY = GAP_SIZE;
  XSetForeground(display, panelGC, cIconHover);
  XFillRectangle(
    display,
    panelWindow,
    panelGC,
    hoverX,
    hoverY,
    ICON_BOX_SIZE,
    ICON_BOX_SIZE
  );
  XSetForeground(display, panelGC, cIconBackground);
}

void renderIcons()
{
  if (DEBUG_FUNCTIONS) printf("%s\n", __func__);
  XClearWindow(display, panelWindow);

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

void renderIconPixelMapAtIndex(int index)
{
  if (DEBUG_FUNCTIONS) printf("%s\n", __func__);
  struct IconNode* icon = getIconByIndex(index);
  if (icon == NULL) return;
  int iconX = index * (ICON_BOX_SIZE + GAP_SIZE) + GAP_SIZE + ICON_INSET;
  XSetClipMask(display, panelGC, icon->mask);
  XSetClipOrigin(display, panelGC, iconX, GAP_SIZE + ICON_INSET);
  XCopyArea(display, icon->pixelMap, panelWindow, panelGC, 0, 0, ICON_SIZE, ICON_SIZE, iconX, GAP_SIZE + ICON_INSET);
  XSetClipMask(display, panelGC, None);
}

void renderIconPixelMaps(struct IconNode* iconList)
{
  if (DEBUG_FUNCTIONS) printf("%s\n", __func__);
  if (iconList == NULL) return;

  struct IconNode* current = iconList;
  int index = 0;

  while (current != NULL)
  {
    renderIconPixelMapAtIndex(index);
    index++;
    current = current->next;
  }
}

void renderIconIdAtIndex(int index)
{
  if (!DEBUG_RENDER_ICON_IDS) return;
  if (DEBUG_FUNCTIONS) printf("%s\n", __func__);
  struct IconNode* icon = getIconByIndex(index);
  if (icon == NULL) return;
  XSetForeground(display, panelGC, cMenuForeground);
  int iconX = index * (ICON_BOX_SIZE + GAP_SIZE) + GAP_SIZE;
  char* idBuffer = (char*)malloc(sizeof(4));
  snprintf(idBuffer, 4, "%d", icon->id);
  XDrawString(display, panelWindow, panelGC, iconX + 2, GAP_SIZE + 10 + 2, idBuffer, strlen(idBuffer));
  free(idBuffer);
}

void renderIconIds(struct IconNode* iconList)
{
  if (!DEBUG_RENDER_ICON_IDS) return;
  if (DEBUG_FUNCTIONS) printf("%s\n", __func__);
  if (iconList == NULL) return;

  struct IconNode* current = iconList;
  int index = 0;

  while (current != NULL)
  {
    renderIconIdAtIndex(index);
    index++;
    current = current->next;
  }
}

void renderMenuHoverAtIndex(int index)
{
  if (DEBUG_FUNCTIONS) printf("%s\n", __func__);
  XSetForeground(display, menuGC, cMenuHover);
  XFillRectangle(
    display,
    menuWindow,
    menuGC,
    0,
    index * ITEM_HEIGHT,
    ITEM_WIDTH,
    ITEM_HEIGHT
  );
  XSetForeground(display, menuGC, cMenuForeground);
}

void renderMenuItems(const char** menuItems, int itemCount)
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
  return GAP_SIZE * 2 + (iconCount - 1) * GAP_SIZE + iconCount * ICON_BOX_SIZE;
}

int calculateIconIndexFromMouseX(int relMouseX, int iconCount)
{
  if (DEBUG_MOTION_FUNCTIONS) printf("%s\n", __func__);
  int iconIndex = 0;
  if (relMouseX < 0) { return -1; }
  if (
    relMouseX > GAP_SIZE + GAP_SIZE / 2 + ICON_BOX_SIZE
  )
  { iconIndex = (relMouseX - GAP_SIZE / 2) / (ICON_BOX_SIZE + GAP_SIZE); }
  if (iconIndex == iconCount) iconIndex--;
  return iconIndex;
}

int calculateItemIndexFromMouseY(int relMouseY, int itemCount)
{
  if (DEBUG_MOTION_FUNCTIONS) printf("%s\n", __func__);
  if (relMouseY <= ITEM_HEIGHT) return 0;
  if (relMouseY < 0 || relMouseY > ITEM_HEIGHT * itemCount) return -1;
  return relMouseY / ITEM_HEIGHT;
}

void loadPixelMap(Pixmap* map, Pixmap* mask, const char* filePath, int* width, int* height)
{
  if (DEBUG_FUNCTIONS) printf("%s\n", __func__);

  XpmAttributes attributes;
  attributes.valuemask = XpmReturnPixels;

  int result = XpmReadFileToPixmap(display, panelWindow, filePath, map, mask, &attributes);
  if (result == XpmSuccess)
  {
    *width = attributes.width;
    *height = attributes.height;
    return;
  }
  fprintf(stderr, "Failed to load icon: %s!\n", filePath);
}

void scalePixelMap(Pixmap* baseMap, Pixmap* baseMask, int baseWidth, int baseHeight, int newWidth, int newHeight)
{
  if (DEBUG_FUNCTIONS) printf("%s\n", __func__);

  Pixmap scaledPixmap = XCreatePixmap(display, panelWindow, newWidth, newHeight, DefaultDepth(display, DefaultScreen(display)));
  Pixmap scaledMask = XCreatePixmap(display, panelWindow, newWidth, newHeight, 1);
  GC gc = XCreateGC(display, scaledPixmap, 0, NULL);
  GC maskGC = XCreateGC(display, scaledMask, 0, NULL);

  float xScale = (float)newWidth / (float)baseWidth;
  float yScale = (float)newHeight / (float)baseHeight;

  for (int y = 0; y < newHeight; y++)
  {
    for (int x = 0; x < newWidth; x++)
    {
      int sourceX = (int)(x / xScale);
      int sourceY = (int)(y / yScale);
      XCopyArea(display, *baseMap, scaledPixmap, gc, sourceX, sourceY, 1, 1, x, y);
      XCopyArea(display, *baseMask, scaledMask, maskGC, sourceX, sourceY, 1, 1, x, y);
    }
  }

  XFreeGC(display, gc);
  XFreeGC(display, maskGC);
  XFreePixmap(display, *baseMap);
  XFreePixmap(display, *baseMask);
  *baseMap = scaledPixmap;
  *baseMask = scaledMask;
}

struct IconNode* createIcon(const char* name)
{
  if (DEBUG_FUNCTIONS) printf("%s\n", __func__);
  char* allocatedName = malloc(ICON_NAME_LIMIT * sizeof(char));
  strcpy(allocatedName, name);
  struct IconNode* icon = (struct IconNode*)malloc(sizeof(struct IconNode));
  icon->pixelMap = None;
  icon->mask = None;
  icon->name = allocatedName;
  icon->id = -1;
  icon->next = NULL;
  return icon;
}

void addIcon(const char* name)
{
  if (DEBUG_FUNCTIONS) printf("%s\n", __func__);
  struct IconNode* newNode = createIcon(name);
  newNode->id = generateIconId();

  Pixmap map = None;
  Pixmap mask = None;
  int mapWidth = 0;
  int mapHeight = 0;
  loadPixelMap(&map, &mask, "icon.xpm", &mapWidth, &mapHeight);
  scalePixelMap(&map, &mask, mapWidth, mapHeight, ICON_SIZE, ICON_SIZE);
  newNode->pixelMap = map;
  newNode->mask = mask;
 
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

void moveIconToLeftByIndex(int index)
{
  if (DEBUG_FUNCTIONS) printf("%s\n", __func__);
  if (index <= 0 || iconList == NULL || iconList->next == NULL) return;

  struct IconNode* previous = NULL;
  struct IconNode* current = iconList;
  struct IconNode* previousPrevious = NULL;

  int currentIndex = 0;
  while (current != NULL && currentIndex != index)
  {
    previousPrevious = previous;
    previous = current;
    current = current->next;
    currentIndex++;
  }

  if (current == NULL) return;

  if (previousPrevious != NULL)
  {
    previousPrevious->next = current;
  }
  else
  {
    iconList = current;
  }

  previous->next = current->next;
  current->next = previous;
}

void moveIconToRightByIndex(int index)
{
  if (DEBUG_FUNCTIONS) printf("%s\n", __func__);
  if (iconList == NULL || iconList->next == NULL) return;

  struct IconNode* previous = NULL;
  struct IconNode* current = iconList;

  int currentIndex = 0;

  while (current != NULL && currentIndex < index)
  {
    previous = current;
    current = current->next;
    currentIndex++;
  }

  if (current == NULL || current->next == NULL) return;

  struct IconNode* nextNode = current->next;

  if (previous != NULL)
  {
    previous->next = nextNode;
  }
  else
  {
    iconList = nextNode;
  }

  current->next = nextNode->next;
  nextNode->next = current;
}

void removeIconByIndex(int index)
{
  if (DEBUG_FUNCTIONS) printf("%s\n", __func__);
  struct IconNode* temporary = iconList;
  if (temporary == NULL) return;
  if (index == 0)
  {
    temporary = temporary->next;
    free(iconList);
    iconList = temporary;
    return;
  }
  int currentIndex = 0;
  struct IconNode* previous = iconList;
  while (currentIndex != index)
  {
    if (temporary == NULL) return;
    previous = temporary;
    temporary = temporary->next;
    currentIndex++;
  }
  previous->next = temporary->next;
  free(temporary);
}

unsigned int generateIconId()
{
  if (DEBUG_FUNCTIONS) printf("%s\n", __func__);
  int usedId = 0;
  if (iconList == NULL) return usedId;
  struct IconNode* current = iconList;
  while (current != NULL)
  {
    if (current->id == usedId)
    {
      usedId++;
      current = iconList;
    }
    else
    {
      current = current->next;
    }
  }
  return usedId;
}

void openTerminal()
{
  system("cd $HOME && alacritty &");
}

unsigned long calculateRGB(uint8_t red, u_int8_t green, uint8_t blue)
{
  if (DEBUG_FUNCTIONS) printf("%s\n", __func__);
  return blue + (green << 8) + (red << 16);
}

void freePixelMaps()
{
  if (DEBUG_FUNCTIONS) printf("%s\n", __func__);
  struct IconNode* current = iconList;
  while (current != NULL)
  {
    XFreePixmap(display, current->pixelMap);
    current = current->next;
  }
}

void freeTexts()
{
  if (DEBUG_FUNCTIONS) printf("%s\n", __func__);
  free(panelMenuTexts);
  free(iconMenuTexts);
}

void freeXObjects()
{
  if (DEBUG_FUNCTIONS) printf("%s\n", __func__);
  XFreeGC(display, panelGC);
  XFreeGC(display, menuGC);
  XCloseDisplay(display);
}
