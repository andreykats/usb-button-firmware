#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

TaskHandle_t myTaskHandleA = NULL;
TaskHandle_t myTaskHandleB = NULL;

void Demo_Task_A(void *arg)
{

    int count = 0;
    while (1)
    {
        count++;
        printf("Demo_Task_A printing...%d\n", count);
        if (count == 10)
        {
            // Resume Task B
            printf("Demo_Task_A resumed Demo_Task_B!\n");
            vTaskResume(myTaskHandleB);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void Demo_Task_B(void *arg)
{
    int count = 0;
    while (1)
    {
        count++;
        printf("Demo_Task_B printing...%d\n", count);
        if (count == 5)
        {
            // Suspend itself: the scheduler will not schedule this task until another task calls vTaskResume
            printf("Demo_Task_B is suspended itself!\n");
            vTaskSuspend(NULL);
        }
        if (count == 10)
        {
            // Self-delete: exit the task; stack and other resources will be reclaimed by the Idle task
            printf("Demo_Task_B is deleted itself!\n");
            vTaskDelete(NULL);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void app_main(void)
{
    // Create tasks and pin to specific cores; stack size 4096 bytes; both tasks have priority 10
    // A is pinned to core 0, B to core 1 (can run in parallel on dual-core)
    xTaskCreatePinnedToCore(Demo_Task_A, "Demo_Task_A", 4096, NULL, 10, &myTaskHandleA, 0);
    xTaskCreatePinnedToCore(Demo_Task_B, "Demo_Task_B", 4096, NULL, 10, &myTaskHandleB, 1);
}