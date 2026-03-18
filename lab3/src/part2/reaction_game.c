#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "xparameters.h"
#include "xgpio.h"
#include "xil_printf.h"

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

#define SPEED_FAST_MS    1000
#define SPEED_MED_MS     1200

typedef enum {
    STATE_IDLE,
    STATE_WAIT,
    STATE_SHOW,
    STATE_RESULT,
    STATE_GAMEOVER
} GameState;

/* ================= CONTEXT ================= */

typedef struct {
    PmodOLED oledDevice;
    PmodKYPD kypdInst;
    XGpio    btnInst;
    XGpio    rgbInst;
    SSD      ssdInst;

    QueueHandle_t xKeyQueue;
    QueueHandle_t xBtnQueue;
    QueueHandle_t xTimeQueue;
    QueueHandle_t xColorQueue;
} SystemContext;

/* ================= CONSTANTS ================= */

static const char HEX_CHARS[] = "0123456789ABCDEF";

static const u8 SSD_SEGMENTS[] = {
    0b00111111, 0b00110000, 0b01011011, 0b01111001, 0b01110100,
    0b01101101, 0b01101111, 0b00111000, 0b01111111, 0b01111100
};

static u8 ssdEncodeDigit(u8 digit, u8 cathode)
{
    u8 segments = (digit <= 9) ? SSD_SEGMENTS[digit] : 0;
    if (cathode) segments |= 0x80;
    return segments;
}

/* ================= TASKS ================= */

static void vKeypadTask(void *pvParameters)
{
    SystemContext *ctx = (SystemContext *)pvParameters;

    u16 keystate;
    XStatus status, lastStatus = KYPD_NO_KEY;
    u8 key;

    while (1) {
        keystate = KYPD_getKeyStates(&ctx->kypdInst);
        status = KYPD_getKeyPressed(&ctx->kypdInst, keystate, &key);

        if (status == KYPD_SINGLE_KEY && lastStatus == KYPD_NO_KEY) {
            xQueueSend(ctx->xKeyQueue, &key, 0);
        }

        lastStatus = status;
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

static void vButtonTask(void *pvParameters)
{
    SystemContext *ctx = (SystemContext *)pvParameters;

    u8 btnVal, lastVal = 0;

    while (1) {
        btnVal = (u8)XGpio_DiscreteRead(&ctx->btnInst, BTN_CHANNEL);

        if (btnVal != 0 && lastVal == 0) {
            xQueueSend(ctx->xBtnQueue, &btnVal, 0);
        }

        lastVal = btnVal;
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

static void vDisplayTask(void *pvParameters)
{
    SystemContext *ctx = (SystemContext *)pvParameters;

    u16 reactionMs = 0;
    u8 hasTime = 0;
    u8 phase = 0;
    TickType_t phaseTimer = 0;

    while (1) {
        u16 newTime;

        if (xQueueReceive(ctx->xTimeQueue, &newTime, 0) == pdTRUE) {
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

            SSD_setSSD(&ctx->ssdInst, left);
            vTaskDelay(pdMS_TO_TICKS(10));
            SSD_setSSD(&ctx->ssdInst, right);
            vTaskDelay(pdMS_TO_TICKS(10));

        } else {
            SSD_setSSD(&ctx->ssdInst, 0);
            vTaskDelay(pdMS_TO_TICKS(50));
        }
    }
}

static void vRgbLedTask(void *pvParameters)
{
    SystemContext *ctx = (SystemContext *)pvParameters;

    u8 color;

    while (1) {
        if (xQueueReceive(ctx->xColorQueue, &color, portMAX_DELAY) == pdTRUE) {
            XGpio_DiscreteWrite(&ctx->rgbInst, RGB_CHANNEL, color);
            vTaskDelay(pdMS_TO_TICKS(2000));
            XGpio_DiscreteWrite(&ctx->rgbInst, RGB_CHANNEL, RGB_OFF);
        }
    }
}

static void vGameTask(void *pvParameters)
{
    SystemContext *ctx = (SystemContext *)pvParameters;

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

    OLED_SetCharUpdate(&ctx->oledDevice, 0);
    srand((unsigned)xTaskGetTickCount());

    while (1) {
        switch (state) {

        case STATE_IDLE:
            OLED_ClearBuffer(&ctx->oledDevice);
            OLED_SetCursor(&ctx->oledDevice, 1, 0);
            OLED_PutString(&ctx->oledDevice, "Reaction Game!");
            OLED_SetCursor(&ctx->oledDevice, 0, 2);
            OLED_PutString(&ctx->oledDevice, "Press BTN0 start");
            OLED_Update(&ctx->oledDevice);

            score = 0;
            lives = MAX_LIVES;
            round = 0;
            totalTime = 0;

            while (1) {
                if (xQueueReceive(ctx->xBtnQueue, &btnVal, portMAX_DELAY)) {
                    if (btnVal & 0x01) break;
                }
            }
            state = STATE_WAIT;
            break;

        case STATE_WAIT:
            round++;
            OLED_ClearBuffer(&ctx->oledDevice);
            OLED_SetCursor(&ctx->oledDevice, 0, 0);
            sprintf(buf, "Rnd %d Lives:%d", round, lives);
            OLED_PutString(&ctx->oledDevice, buf);
            OLED_Update(&ctx->oledDevice);

            while (xQueueReceive(ctx->xKeyQueue, &keyVal, 0));
            vTaskDelay(pdMS_TO_TICKS(1000 + rand() % 2000));

            state = STATE_SHOW;
            break;

        case STATE_SHOW: {
            targetChar = HEX_CHARS[rand() % 16];

            OLED_ClearBuffer(&ctx->oledDevice);
            OLED_SetCursor(&ctx->oledDevice, 6, 2);
            sprintf(buf, ">> %c <<", targetChar);
            OLED_PutString(&ctx->oledDevice, buf);
            OLED_Update(&ctx->oledDevice);

            startTick = xTaskGetTickCount();

            if (xQueueReceive(ctx->xKeyQueue, &keyVal, pdMS_TO_TICKS(TIMEOUT_MS))) {
                reactionMs = (xTaskGetTickCount() - startTick) * portTICK_PERIOD_MS;

                if (keyVal == targetChar) {
                    score++;
                    totalTime += reactionMs;

                    xQueueOverwrite(ctx->xTimeQueue, &reactionMs);

                    u8 color;
                    if (reactionMs < SPEED_FAST_MS)
                        color = RGB_GREEN;
                    else if (reactionMs < SPEED_MED_MS)
                        color = RGB_YELLOW;
                    else
                        color = RGB_RED;

                    xQueueOverwrite(ctx->xColorQueue, &color);

                } else {
                    lives--;
                    reactionMs = 0;
                    u8 color = RGB_RED;
                    xQueueOverwrite(ctx->xColorQueue, &color);
                }

            } else {
                lives--;
                reactionMs = 9999;
                u8 color = RGB_RED;
                xQueueOverwrite(ctx->xColorQueue, &color);
            }

            state = STATE_RESULT;
            break;
        }

        case STATE_RESULT:
            vTaskDelay(pdMS_TO_TICKS(RESULT_DISPLAY_MS));

            if (lives <= 0 || round >= MAX_ROUNDS)
                state = STATE_GAMEOVER;
            else
                state = STATE_WAIT;
            break;

        case STATE_GAMEOVER:
            OLED_ClearBuffer(&ctx->oledDevice);
            OLED_SetCursor(&ctx->oledDevice, 2, 0);
            OLED_PutString(&ctx->oledDevice, "Game Over!");
            OLED_Update(&ctx->oledDevice);

            while (1) {
                if (xQueueReceive(ctx->xBtnQueue, &btnVal, portMAX_DELAY)) {
                    if (btnVal & 0x08) break;
                }
            }
            state = STATE_IDLE;
            break;
        }
    }
}

/* ================= MAIN ================= */

int main(void)
{
    static SystemContext ctx;

    OLED_Begin(&ctx.oledDevice,
               XPAR_GPIO_OLED_BASEADDR,
               XPAR_SPI_OLED_BASEADDR,
               0x2, 0x0);

    KYPD_begin(&ctx.kypdInst, KYPD_BASEADDR);
    KYPD_loadKeyTable(&ctx.kypdInst, (u8 *)DEFAULT_KEYTABLE);

    XGpio_Initialize(&ctx.btnInst, BTN_BASEADDR);
    RGB_LED_begin(&ctx.rgbInst, RGB_LED_BASEADDR);
    SSD_begin(&ctx.ssdInst, SSD_BASEADDR);

    XGpio_DiscreteWrite(&ctx.rgbInst, RGB_CHANNEL, RGB_OFF);

    ctx.xKeyQueue   = xQueueCreate(4, sizeof(u8));
    ctx.xBtnQueue   = xQueueCreate(4, sizeof(u8));
    ctx.xTimeQueue  = xQueueCreate(1, sizeof(u16));
    ctx.xColorQueue = xQueueCreate(1, sizeof(u8));

    xTaskCreate(vGameTask, "Game", 1024, &ctx, 1, NULL);
    xTaskCreate(vKeypadTask, "Keypad", 512, &ctx, 1, NULL);
    xTaskCreate(vButtonTask, "Button", 256, &ctx, 1, NULL);
    xTaskCreate(vDisplayTask, "Display", 512, &ctx, 1, NULL);
    xTaskCreate(vRgbLedTask, "RGB", 256, &ctx, 1, NULL);

    xil_printf("Reaction Game initialized!\n");

    vTaskStartScheduler();

    while (1);
}
