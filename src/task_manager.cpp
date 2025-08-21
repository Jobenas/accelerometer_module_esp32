#include "task_manager.h"
#include "modbus_interface.h"
#include "accelerometer_interface.h"

// Task handles
TaskHandle_t sampling_task_handle = nullptr;
TaskHandle_t processing_task_handle = nullptr;
TaskHandle_t analytics_task_handle = nullptr;
TaskHandle_t modbus_task_handle = nullptr;

// Synchronization primitives
SemaphoreHandle_t buffer_mutex = nullptr;
SemaphoreHandle_t buffer_ready_semaphore = nullptr;
QueueHandle_t analytics_queue = nullptr;

// Task status
TaskManagerStatus task_status;

void samplingTask(void* parameter) {
  Serial.println("Sampling task started on core " + String(xPortGetCoreID()));
  task_status.sampling_task_running = true;
  
  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xFrequency = pdMS_TO_TICKS(1); // 1ms = 1000Hz
  
  unsigned long sample_count = 0;
  unsigned long start_time = millis();
  
  while (true) {
    task_status.sampling_loop_count++;
    
    try {
      // Take mutex to access buffer safely
      if (xSemaphoreTake(buffer_mutex, pdMS_TO_TICKS(1)) == pdTRUE) {
        
        // Check if buffer has space
        if (!dataBuffer.isFull()) {
          // Read sensor data using abstraction layer
          AccelData accelData;
          if (accelerometer.readData(accelData) && accelData.valid) {
            // Convert g-force to raw values for compatibility with existing buffer system
            // Assuming scale factor similar to ADXL355 (can be adjusted)
            int32_t x = (int32_t)(accelData.x * 256000.0f);
            int32_t y = (int32_t)(accelData.y * 256000.0f);
            int32_t z = (int32_t)(accelData.z * 256000.0f);
            
            #if ENABLE_DEBUG_OUTPUT
            static unsigned long last_sensor_debug = 0;
            if (millis() - last_sensor_debug > 3000) {  // Debug every 3 seconds
              Serial.printf("[SENSOR-RAW] Raw values: X=%ld, Y=%ld, Z=%ld\n", x, y, z);
              Serial.printf("[SENSOR-G] G-values: X=%.6f, Y=%.6f, Z=%.6f (%s)\n", 
                           accelData.x, accelData.y, accelData.z, accelerometer.getSensorName());
              last_sensor_debug = millis();
            }
            #endif
            
            // Add sample to buffer
            if (dataBuffer.addSample(x, y, z)) {
              sample_count++;
              task_status.last_sample_time = millis();
              
              // Check if buffer is now full
              if (dataBuffer.isFull()) {
                // Signal processing task that buffer is ready
                xSemaphoreGive(buffer_ready_semaphore);
              }
            } else {
              task_status.sampling_errors++;
            }
          } else {
            // Failed to read sensor data
            task_status.sampling_errors++;
          }
        } else {
          // Buffer is full and hasn't been processed yet
          task_status.missed_samples++;
        }
        
        xSemaphoreGive(buffer_mutex);
        
      } else {
        // Couldn't get mutex in time
        task_status.missed_samples++;
      }
      
      // Calculate actual sample rate every second
      if (sample_count > 0 && (sample_count % 1000) == 0) {
        unsigned long elapsed = millis() - start_time;
        task_status.actual_sample_rate = (sample_count * 1000.0) / elapsed;
      }
      
    } catch (...) {
      task_status.sampling_errors++;
    }
    
    // Wait for next sample time (maintains 1kHz rate)
    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}

void processingTask(void* parameter) {
  Serial.println("Processing task started on core " + String(xPortGetCoreID()));
  task_status.processing_task_running = true;
  
  while (true) {
    task_status.processing_loop_count++;
    
    // Wait for buffer to be ready (blocks until signaled)
    if (xSemaphoreTake(buffer_ready_semaphore, portMAX_DELAY) == pdTRUE) {
      
      try {
        // Take mutex to access buffer safely
        if (xSemaphoreTake(buffer_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
          
          // Process the buffer data
          if (dataBuffer.isFull()) {
            BufferStats stats;
            dataBuffer.calculateStats(stats);
            // dataBuffer.printStats(stats);
            
            // Send stats to analytics task via queue
            if (analytics_queue != nullptr) {
              if (xQueueSend(analytics_queue, &stats, pdMS_TO_TICKS(10)) != pdTRUE) {
                Serial.println("Failed to send stats to analytics queue");
                task_status.processing_errors++;
              }
            }
            
            // Reset buffer for next collection cycle
            dataBuffer.reset();
            
            task_status.last_processing_time = millis();
          }
          
          xSemaphoreGive(buffer_mutex);
          
        } else {
          Serial.println("Processing task: Failed to get buffer mutex");
          task_status.processing_errors++;
        }
        
      } catch (...) {
        task_status.processing_errors++;
      }
    }
  }
}

void analyticsTask(void* parameter) {
  Serial.println("Analytics task started on core " + String(xPortGetCoreID()));
  task_status.analytics_task_running = true;
  
  BufferStats stats;
  
  while (true) {
    task_status.analytics_loop_count++;
    
    // Wait for buffer statistics from processing task
    if (xQueueReceive(analytics_queue, &stats, portMAX_DELAY) == pdTRUE) {
      
      try {
        // Process the statistics with analytics
        analytics.processBufferStats(stats);
        
        task_status.last_analytics_time = millis();
        
        // Print analytics every 10 windows (10 seconds)
        if (analytics.getWindowCount() % 10 == 0) {
          analytics.printRunningStats();
        }
        
      } catch (...) {
        task_status.analytics_errors++;
        Serial.println("Analytics task: Exception occurred");
      }
    }
  }
}

void modbusTask(void* parameter) {
  #if ENABLE_MODBUS_INTERFACE
  Serial.println("Modbus task started on core " + String(xPortGetCoreID()));
  task_status.modbus_task_running = true;
  
  while (true) {
    task_status.modbus_loop_count++;
    
    try {
      // Update Modbus interface (process requests and update registers)
      modbusInterface.update();
      
      task_status.last_modbus_time = millis();
      
    } catch (...) {
      task_status.modbus_errors++;
      Serial.println("Modbus task: Exception occurred");
    }
    
    // Small delay to prevent overwhelming the system
    vTaskDelay(pdMS_TO_TICKS(10)); // 10ms delay = 100Hz update rate
  }
  #else
  Serial.println("Modbus task disabled");
  vTaskDelete(NULL);  // Delete this task since Modbus is disabled
  #endif
}

bool startTasks() {
  Serial.println("Initializing FreeRTOS tasks...");
  
  // Create mutex for buffer access
  buffer_mutex = xSemaphoreCreateMutex();
  if (buffer_mutex == nullptr) {
    Serial.println("Failed to create buffer mutex!");
    return false;
  }
  
  // Create semaphore for buffer ready signal
  buffer_ready_semaphore = xSemaphoreCreateBinary();
  if (buffer_ready_semaphore == nullptr) {
    Serial.println("Failed to create buffer ready semaphore!");
    return false;
  }
  
  // Create queue for analytics data (hold up to 3 BufferStats)
  analytics_queue = xQueueCreate(3, sizeof(BufferStats));
  if (analytics_queue == nullptr) {
    Serial.println("Failed to create analytics queue!");
    return false;
  }
  
  // Create sampling task (high priority, core 1)
  BaseType_t result1 = xTaskCreatePinnedToCore(
    samplingTask,                    // Task function
    "SamplingTask",                  // Task name
    SAMPLING_TASK_STACK_SIZE,        // Stack size
    nullptr,                         // Parameter
    SAMPLING_TASK_PRIORITY,          // Priority
    &sampling_task_handle,           // Task handle
    SAMPLING_TASK_CORE               // Core
  );
  
  // Create processing task (lower priority, core 0)
  BaseType_t result2 = xTaskCreatePinnedToCore(
    processingTask,                  // Task function
    "ProcessingTask",                // Task name
    PROCESSING_TASK_STACK_SIZE,      // Stack size
    nullptr,                         // Parameter
    PROCESSING_TASK_PRIORITY,        // Priority
    &processing_task_handle,         // Task handle
    PROCESSING_TASK_CORE             // Core
  );
  
  // Create analytics task (lowest priority, core 0)
  BaseType_t result3 = xTaskCreatePinnedToCore(
    analyticsTask,                   // Task function
    "AnalyticsTask",                 // Task name
    ANALYTICS_TASK_STACK_SIZE,       // Stack size
    nullptr,                         // Parameter
    ANALYTICS_TASK_PRIORITY,         // Priority
    &analytics_task_handle,          // Task handle
    ANALYTICS_TASK_CORE              // Core
  );
  
  #if ENABLE_MODBUS_INTERFACE
  // Create modbus task (same priority as analytics, core 0)
  BaseType_t result4 = xTaskCreatePinnedToCore(
    modbusTask,                      // Task function
    "ModbusTask",                    // Task name
    MODBUS_TASK_STACK_SIZE,          // Stack size
    nullptr,                         // Parameter
    MODBUS_TASK_PRIORITY,            // Priority
    &modbus_task_handle,             // Task handle
    MODBUS_TASK_CORE                 // Core
  );
  #else
  BaseType_t result4 = pdPASS;  // Dummy success result when Modbus is disabled
  #endif
  
  if (result1 == pdPASS && result2 == pdPASS && result3 == pdPASS && result4 == pdPASS) {
    Serial.println("All tasks created successfully");
    return true;
  } else {
    Serial.println("Failed to create tasks");
    return false;
  }
}

void stopTasks() {
  if (sampling_task_handle != nullptr) {
    vTaskDelete(sampling_task_handle);
    sampling_task_handle = nullptr;
    task_status.sampling_task_running = false;
  }
  
  if (processing_task_handle != nullptr) {
    vTaskDelete(processing_task_handle);
    processing_task_handle = nullptr;
    task_status.processing_task_running = false;
  }
  
  if (analytics_task_handle != nullptr) {
    vTaskDelete(analytics_task_handle);
    analytics_task_handle = nullptr;
    task_status.analytics_task_running = false;
  }
  
  if (modbus_task_handle != nullptr) {
    vTaskDelete(modbus_task_handle);
    modbus_task_handle = nullptr;
    task_status.modbus_task_running = false;
  }
  
  if (buffer_mutex != nullptr) {
    vSemaphoreDelete(buffer_mutex);
    buffer_mutex = nullptr;
  }
  
  if (buffer_ready_semaphore != nullptr) {
    vSemaphoreDelete(buffer_ready_semaphore);
    buffer_ready_semaphore = nullptr;
  }
  
  if (analytics_queue != nullptr) {
    vQueueDelete(analytics_queue);
    analytics_queue = nullptr;
  }
  
  Serial.println("All tasks stopped");
}

void printTaskInfo() {
  Serial.println("\n=== Task Status ===");
  Serial.print("Sampling task running: "); Serial.println(task_status.sampling_task_running ? "Yes" : "No");
  Serial.print("Processing task running: "); Serial.println(task_status.processing_task_running ? "Yes" : "No");
  Serial.print("Analytics task running: "); Serial.println(task_status.analytics_task_running ? "Yes" : "No");
  Serial.print("Modbus task running: "); Serial.println(task_status.modbus_task_running ? "Yes" : "No");
  Serial.print("Sampling loops: "); Serial.println(task_status.sampling_loop_count);
  Serial.print("Processing loops: "); Serial.println(task_status.processing_loop_count);
  Serial.print("Analytics loops: "); Serial.println(task_status.analytics_loop_count);
  Serial.print("Modbus loops: "); Serial.println(task_status.modbus_loop_count);
  Serial.print("Sampling errors: "); Serial.println(task_status.sampling_errors);
  Serial.print("Processing errors: "); Serial.println(task_status.processing_errors);
  Serial.print("Analytics errors: "); Serial.println(task_status.analytics_errors);
  Serial.print("Modbus errors: "); Serial.println(task_status.modbus_errors);
  Serial.print("Missed samples: "); Serial.println(task_status.missed_samples);
  Serial.print("Actual sample rate: "); Serial.print(task_status.actual_sample_rate, 1); Serial.println(" Hz");
  Serial.print("Last sample: "); Serial.print(millis() - task_status.last_sample_time); Serial.println(" ms ago");
  Serial.print("Last processing: "); Serial.print(millis() - task_status.last_processing_time); Serial.println(" ms ago");
  Serial.print("Last analytics: "); Serial.print(millis() - task_status.last_analytics_time); Serial.println(" ms ago");
  Serial.print("Last modbus: "); Serial.print(millis() - task_status.last_modbus_time); Serial.println(" ms ago");
  
  // Queue status
  if (analytics_queue != nullptr) {
    UBaseType_t queueSpaces = uxQueueSpacesAvailable(analytics_queue);
    Serial.print("Analytics queue free spaces: "); Serial.println(queueSpaces);
  }
  
  // FreeRTOS task info
  if (sampling_task_handle != nullptr) {
    UBaseType_t highWaterMark = uxTaskGetStackHighWaterMark(sampling_task_handle);
    Serial.print("Sampling task free stack: "); Serial.print(highWaterMark * 4); Serial.println(" bytes");
  }
  
  if (processing_task_handle != nullptr) {
    UBaseType_t highWaterMark = uxTaskGetStackHighWaterMark(processing_task_handle);
    Serial.print("Processing task free stack: "); Serial.print(highWaterMark * 4); Serial.println(" bytes");
  }
  
  if (analytics_task_handle != nullptr) {
    UBaseType_t highWaterMark = uxTaskGetStackHighWaterMark(analytics_task_handle);
    Serial.print("Analytics task free stack: "); Serial.print(highWaterMark * 4); Serial.println(" bytes");
  }
  
  if (modbus_task_handle != nullptr) {
    UBaseType_t highWaterMark = uxTaskGetStackHighWaterMark(modbus_task_handle);
    Serial.print("Modbus task free stack: "); Serial.print(highWaterMark * 4); Serial.println(" bytes");
  }
  
  Serial.print("Free heap: "); Serial.print(ESP.getFreeHeap()); Serial.println(" bytes");
  Serial.println("==================");
}
