#ifndef TASK_MANAGER_H
#define TASK_MANAGER_H

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "adxl355.h"
#include "data_buffer.h"
#include "analytics.h"

// Task configuration
#define SAMPLING_TASK_STACK_SIZE    4096
#define PROCESSING_TASK_STACK_SIZE  4096
#define ANALYTICS_TASK_STACK_SIZE   4096
#define MODBUS_TASK_STACK_SIZE      4096
#define SAMPLING_TASK_PRIORITY      3  // High priority for precise timing
#define PROCESSING_TASK_PRIORITY    2  // Lower priority for data processing
#define ANALYTICS_TASK_PRIORITY     1  // Lowest priority for analytics
#define MODBUS_TASK_PRIORITY        1  // Same as analytics priority
#define SAMPLING_TASK_CORE          1  // Core 1 for sampling
#define PROCESSING_TASK_CORE        0  // Core 0 for processing
#define ANALYTICS_TASK_CORE         0  // Core 0 for analytics
#define MODBUS_TASK_CORE            0  // Core 0 for modbus

// Task handles
extern TaskHandle_t sampling_task_handle;
extern TaskHandle_t processing_task_handle;
extern TaskHandle_t analytics_task_handle;
extern TaskHandle_t modbus_task_handle;

// Global objects (defined in main)
extern ADXL355 sensor;
extern DataBuffer dataBuffer;
extern Analytics analytics;

// Synchronization primitives
extern SemaphoreHandle_t buffer_mutex;
extern SemaphoreHandle_t buffer_ready_semaphore;
extern QueueHandle_t analytics_queue;

// Task functions
void samplingTask(void* parameter);
void processingTask(void* parameter);
void analyticsTask(void* parameter);
void modbusTask(void* parameter);

// Task management functions
bool startTasks();
void stopTasks();
void printTaskInfo();

// Task status monitoring
struct TaskManagerStatus {
  unsigned long sampling_loop_count = 0;
  unsigned long processing_loop_count = 0;
  unsigned long analytics_loop_count = 0;
  unsigned long modbus_loop_count = 0;
  unsigned long sampling_errors = 0;
  unsigned long processing_errors = 0;
  unsigned long analytics_errors = 0;
  unsigned long modbus_errors = 0;
  unsigned long last_sample_time = 0;
  unsigned long last_processing_time = 0;
  unsigned long last_analytics_time = 0;
  unsigned long last_modbus_time = 0;
  bool sampling_task_running = false;
  bool processing_task_running = false;
  bool analytics_task_running = false;
  bool modbus_task_running = false;
  unsigned long missed_samples = 0;
  float actual_sample_rate = 0.0;
};

extern TaskManagerStatus task_status;

#endif // TASK_MANAGER_H
