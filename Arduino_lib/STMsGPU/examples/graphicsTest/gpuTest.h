#define FS(x) (__FlashStringHelper*)(x)

const char Loremipsum2[] PROGMEM = "\
Lorem ipsum dolor sit amet, consectetur adipiscing elit. \
Curabitur adipiscing ante sed nibh tincidunt feugiat. \
Maecenas enim massa, fringilla sed malesuada et, malesuada sit amet turpis. \
Sed porttitor neque ut ante pretium vitae malesuada nunc bibendum. \
Nullam aliquet ultrices massa eu hendrerit. Ut sed nisi lorem. \
In vestibulum purus a tortor imperdiet posuere.\n\n";


void testdrawtext(void);
void testlines(void);
void testfastlines(void);
void testdrawrects(void);
void testfillrects(void);
void testfillcircles(void);
void testroundrects(void);
void testtriangles(void);

void drawRandPixels(void);
void drawRandLines(void);
void drawRandRect(void);
void drawRandFillRect(void);
void drawRandTriangles(void);
void drawRandRoundRect(void);
void drawRandRoundFillRect(void);
void drawRandCircle(void);
void drawRandFillCircle(void);
void matrixScreen(void);


void (*pArrExecGFXFunc[])(void) = {
    testdrawtext,
    testlines,
    testfastlines,
    //testdrawrects,
    testfillrects,
    testfillcircles,
    testroundrects,
    testtriangles,

    drawRandPixels,
    drawRandLines,
    drawRandRect,
    drawRandFillRect,
    drawRandTriangles,
    //drawRandRoundRect,
    //drawRandRoundFillRect,
    drawRandCircle,
    drawRandFillCircle,
    matrixScreen
  };

#define FUNC_TO_TEST_COUNT (sizeof(pArrExecGFXFunc)/sizeof(pArrExecGFXFunc[0]))

// --------------------------------------------------------- //
#define MAX_SPEED 0 

// can save ROM and increase speed, but its unfair!
// use it if you know end resolution
#if MAX_SPEED
 #define TFT_W 320
 #define TFT_H 240
#else
 #define TFT_W gpu.width()
 #define TFT_H gpu.height()
#endif /* MAX_SPEED */

#define TEST_SAMPLE_SIZE 2000
#define TEST_SAMPLE_SCREENS 2

#define BASE_RADIUS 10

// macro definition for random function
#define MIN_COLOR 32
#define MAX_COLOR 255
#define COLOR_RANGE (((MAX_COLOR + 1) - MIN_COLOR) + MIN_COLOR)
#define RND_COLOR (randNum() % COLOR_RANGE)

//#define RND_565COLOR(r, g, b)  (gpu.color565(r, g, b))
#define RND_565COLOR  (((RND_COLOR & 0xF8) << 8) | ((RND_COLOR & 0xFC) << 3) | (RND_COLOR >> 3))

#define RND_POSX(offset) (randNum() % (TFT_W-offset))
#define RND_POSY(offset) (randNum() % (TFT_H-offset)) 
