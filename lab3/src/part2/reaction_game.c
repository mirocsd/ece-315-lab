#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "xparameters.h"
#include "xgpio.h"
#include "xil_printf.h"
#include "xil_cache.h"

#include <stdlib.h>
#include <stdio.h>
#include "pmodkypd.h"
#include "sleep.h"
#include "PmodOLED.h"
#include "OLEDControllerCustom.h"
#include "ssd.h"
#include "rgb_led.h"



#define BTN_BASEADDR   XPAR_GPIO_INPUTS_BASEADDR
#define KYPD_BASEADDR  XPAR_GPIO_KEYPAD_BASEADDR
#define SSD_BASEADDR   XPAR_GPIO_SSD_BASEADDR

#define BTN_CHANNEL    1

#define DEFAULT_KEYTABLE "0FED789C456B123A"
#define MAX_ROUNDS       10
#define MAX_LIVES        3
#define TIMEOUT_MS       5000
#define RESULT_DISPLAY_MS 2000

#define SPEED_FAST_MS    200
#define SPEED_MED_MS     400

typedef enum {
    STATE_IDLE,
    STATE_WAIT,
    STATE_SHOW,
    STATE_RESULT,
    STATE_GAMEOVER
} GameState;

static PmodOLED oledDevice;
static PmodKYPD kypdInst;
static XGpio    btnInst;
static XGpio    rgbInst;
static SSD      ssdInst;

static QueueHandle_t xKeyQueue;
static QueueHandle_t xBtnQueue;
static QueueHandle_t xTimeQueue;
static QueueHandle_t xColorQueue;

static const char HEX_CHARS[] = "0123456789ABCDEF";

static const u8 SSD_SEGMENTS[] = {
    0b00111111, /* 0 */
    0b00110000, /* 1 */
    0b01011011, /* 2 */
    0b01111001, /* 3 */
    0b01110100, /* 4 */
    0b01101101, /* 5 */
    0b01101111, /* 6 */
    0b00111000, /* 7 */
    0b01111111, /* 8 */
    0b01111100  /* 9 */
};

static void vGameTask(void *pvParameters);
static void vKeypadTask(void *pvParameters);
static void vButtonTask(void *pvParameters);
static void vDisplayTask(void *pvParameters);
static void vRgbLedTask(void *pvParameters);

static u8 ssdEncodeDigit(u8 digit, u8 cathode)
{
    u8 segments = (digit <= 9) ? SSD_SEGMENTS[digit] : 0;
    if (cathode)
        segments |= 0x80;
    return segments;
}

int main(void)
{
    OLED_Begin(&oledDevice,
               XPAR_GPIO_OLED_BASEADDR,
               XPAR_SPI_OLED_BASEADDR,
               0x0, 0x1);

    KYPD_begin(&kypdInst, KYPD_BASEADDR);
    KYPD_loadKeyTable(&kypdInst, (u8 *)DEFAULT_KEYTABLE);

    XGpio_Initialize(&btnInst, BTN_BASEADDR);
    RGB_LED_begin(&rgbInst, RGB_LED_BASEADDR);
    SSD_begin(&ssdInst, SSD_BASEADDR);

    xKeyQueue   = xQueueCreate(4, sizeof(u8));
    xBtnQueue   = xQueueCreate(4, sizeof(u8));
    xTimeQueue  = xQueueCreate(1, sizeof(u16));
    xColorQueue = xQueueCreate(1, sizeof(u8));

    xTaskCreate(vGameTask,    "Game",    1024, NULL, tskIDLE_PRIORITY + 1, NULL);
    xTaskCreate(vKeypadTask,  "Keypad",  512,  NULL, tskIDLE_PRIORITY + 1, NULL);
    xTaskCreate(vButtonTask,  "Button",  256,  NULL, tskIDLE_PRIORITY + 1, NULL);
    xTaskCreate(vDisplayTask, "Display", 512,  NULL, tskIDLE_PRIORITY + 1, NULL);
    xTaskCreate(vRgbLedTask,  "RGB",     256,  NULL, tskIDLE_PRIORITY + 1, NULL);

    xil_printf("Reaction Game initialized!\r\n");
    vTaskStartScheduler();

    while (1);
    return 0;
}

static void vKeypadTask(void *pvParameters)
{
    u16 keystate;
    XStatus status, lastStatus = KYPD_NO_KEY;
    u8 key;

    while (1) {
        keystate = KYPD_getKeyStates(&kypdInst);
        status = KYPD_getKeyPressed(&kypdInst, keystate, &key);

        if (status == KYPD_SINGLE_KEY && lastStatus == KYPD_NO_KEY) {
            xQueueSend(xKeyQueue, &key, 0);
        }

        lastStatus = status;
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

static void vButtonTask(void *pvParameters)
{
    u8 btnVal, lastVal = 0;

    while (1) {
        btnVal = (u8)XGpio_DiscreteRead(&btnInst, BTN_CHANNEL);

        if (btnVal != 0 && lastVal == 0) {
            xQueueSend(xBtnQueue, &btnVal, 0);
        }

        lastVal = btnVal;
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

static void vDisplayTask(void *pvParameters)
{
    u16 reactionMs = 0;
    u8 hasTime = 0;
    u8 phase = 0;
    TickType_t phaseTimer = 0;

    while (1) {
        u16 newTime;
        if (xQueueReceive(xTimeQueue, &newTime, 0) == pdTRUE) {
            reactionMs = newTime;
            hasTime = 1;
            phase = 0;
            phaseTimer = xTaskGetTickCount();
        }

        if (hasTime) {
            u8 d3 = (reactionMs / 1000) % 10;
            u8 d2 = (reactionMs / 100) % 10;
            u8 d1 = (reactionMs / 10) % 10;
            u8 d0 = reactionMs % 10;

            if ((xTaskGetTickCount() - phaseTimer) > pdMS_TO_TICKS(500)) {
                phase = !phase;
                phaseTimer = xTaskGetTickCount();
            }

            u8 left, right;
            if (phase == 0) {
                left  = ssdEncodeDigit(d3, 0);
                right = ssdEncodeDigit(d2, 1);
            } else {
                left  = ssdEncodeDigit(d1, 0);
                right = ssdEncodeDigit(d0, 1);
            }

            SSD_setSSD(&ssdInst, left);
            vTaskDelay(pdMS_TO_TICKS(10));
            SSD_setSSD(&ssdInst, right);
            vTaskDelay(pdMS_TO_TICKS(10));
        } else {
            SSD_setSSD(&ssdInst, 0);
            vTaskDelay(pdMS_TO_TICKS(50));
        }
    }
}

static void vRgbLedTask(void *pvParameters)
{
    u8 color;

    while (1) {
        if (xQueueReceive(xColorQueue, &color, portMAX_DELAY) == pdTRUE) {
            XGpio_DiscreteWrite(&rgbInst, RGB_CHANNEL, color);
            vTaskDelay(pdMS_TO_TICKS(2000));
            XGpio_DiscreteWrite(&rgbInst, RGB_CHANNEL, RGB_OFF);
        }
    }
}

static void vGameTask(void *pvParameters)
{
    GameState state = STATE_IDLE;
    u8 btnVal, keyVal;
    char targetChar;
    TickType_t startTick;
    u16 reactionMs = 0;
    int score = 0;
    int lives = MAX_LIVES;
    int round = 0;
    u32 totalTime = 0;
    char buf[20];

    OLED_SetCharUpdate(&oledDevice, 0);
    srand((unsigned)xTaskGetTickCount());

    while (1) {
        switch (state) {

        case STATE_IDLE:
            OLED_ClearBuffer(&oledDevice);
            OLED_SetCursor(&oledDevice, 1, 0);
            OLED_PutString(&oledDevice, "Reaction Game!");
            OLED_SetCursor(&oledDevice, 0, 2);
            OLED_PutString(&oledDevice, "Press BTN0 start");
            OLED_Update(&oledDevice);

            score = 0;
            lives = MAX_LIVES;
            round = 0;
            totalTime = 0;

            while (1) {
                if (xQueueReceive(xBtnQueue, &btnVal, portMAX_DELAY) == pdTRUE) {
                    if (btnVal & 0x01) break;
                }
            }
            state = STATE_WAIT;
            break;

        case STATE_WAIT:
            round++;
            OLED_ClearBuffer(&oledDevice);
            OLED_SetCursor(&oledDevice, 0, 0);
            sprintf(buf, "Rnd %d  Lives:%d", round, lives);
            OLED_PutString(&oledDevice, buf);
            OLED_SetCursor(&oledDevice, 2, 2);
            OLED_PutString(&oledDevice, "Get ready...");
            OLED_Update(&oledDevice);

            while (xQueueReceive(xKeyQueue, &keyVal, 0) == pdTRUE);

            srand((unsigned)xTaskGetTickCount());
            vTaskDelay(pdMS_TO_TICKS(1000 + (rand() % 2001)));

            state = STATE_SHOW;
            break;

        case STATE_SHOW: {
            int idx = rand() % 16;
            targetChar = HEX_CHARS[idx];

            OLED_ClearBuffer(&oledDevice);
            OLED_SetCursor(&oledDevice, 0, 0);
            sprintf(buf, "Rnd %d  Lives:%d", round, lives);
            OLED_PutString(&oledDevice, buf);
            OLED_SetCursor(&oledDevice, 6, 2);
            sprintf(buf, ">> %c <<", targetChar);
            OLED_PutString(&oledDevice, buf);
            OLED_Update(&oledDevice);

            startTick = xTaskGetTickCount();

            if (xQueueReceive(xKeyQueue, &keyVal, pdMS_TO_TICKS(TIMEOUT_MS)) == pdTRUE) {
                reactionMs = (u16)((xTaskGetTickCount() - startTick) * portTICK_PERIOD_MS);

                if (keyVal == (u8)targetChar) {
                    score++;
                    totalTime += reactionMs;

                    xQueueOverwrite(xTimeQueue, &reactionMs);

                    u8 color;
                    if (reactionMs < SPEED_FAST_MS)
                        color = RGB_GREEN;
                    else if (reactionMs < SPEED_MED_MS)
                        color = RGB_YELLOW;
                    else
                        color = RGB_RED;
                    xQueueOverwrite(xColorQueue, &color);

                    xil_printf("Hit! %dms\r\n", reactionMs);
                } else {
                    lives--;
                    reactionMs = 0;
                    u8 color = RGB_RED;
                    xQueueOverwrite(xColorQueue, &color);
                    xil_printf("Wrong! Expected %c got %c\r\n", targetChar, keyVal);
                }
            } else {
                lives--;
                reactionMs = 9999;
                u8 color = RGB_RED;
                xQueueOverwrite(xColorQueue, &color);
                xil_printf("Timeout!\r\n");
            }

            state = STATE_RESULT;
            break;
        }

        case STATE_RESULT:
            OLED_ClearBuffer(&oledDevice);
            OLED_SetCursor(&oledDevice, 0, 0);
            sprintf(buf, "Score:%d Lives:%d", score, lives);
            OLED_PutString(&oledDevice, buf);
            OLED_SetCursor(&oledDevice, 0, 2);

            if (reactionMs == 0) {
                OLED_PutString(&oledDevice, "Wrong key!");
            } else if (reactionMs >= TIMEOUT_MS) {
                OLED_PutString(&oledDevice, "Too slow!");
            } else {
                sprintf(buf, "Time: %dms", reactionMs);
                OLED_PutString(&oledDevice, buf);
            }
            OLED_Update(&oledDevice);

            vTaskDelay(pdMS_TO_TICKS(RESULT_DISPLAY_MS));

            if (lives <= 0 || round >= MAX_ROUNDS)
                state = STATE_GAMEOVER;
            else
                state = STATE_WAIT;
            break;

        case STATE_GAMEOVER:
            OLED_ClearBuffer(&oledDevice);
            OLED_SetCursor(&oledDevice, 2, 0);
            OLED_PutString(&oledDevice, "Game Over!");
            OLED_SetCursor(&oledDevice, 0, 1);
            sprintf(buf, "Score: %d/%d", score, round);
            OLED_PutString(&oledDevice, buf);
            OLED_SetCursor(&oledDevice, 0, 2);
            if (score > 0)
                sprintf(buf, "Avg: %lums", totalTime / (u32)score);
            else
                sprintf(buf, "Avg: N/A");
            OLED_PutString(&oledDevice, buf);
            OLED_SetCursor(&oledDevice, 0, 3);
            OLED_PutString(&oledDevice, "BTN3 to restart");
            OLED_Update(&oledDevice);

            while (1) {
                if (xQueueReceive(xBtnQueue, &btnVal, portMAX_DELAY) == pdTRUE) {
                    if (btnVal & 0x08) break;
                }
            }
            state = STATE_IDLE;
            break;
        }
    }
}
