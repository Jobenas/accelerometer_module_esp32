#include "data_buffer.h"
#include <math.h>

DataBuffer::DataBuffer() : buffer(nullptr), write_index(0), sample_count(0), 
                          buffer_full(false), last_sample_time(0), buffer_start_time(0) {
}

DataBuffer::~DataBuffer() {
  if (buffer) {
    delete[] buffer;
  }
}

bool DataBuffer::begin() {
  // Allocate buffer memory
  buffer = new AccelSample[BUFFER_SIZE];
  if (!buffer) {
    Serial.println("Failed to allocate buffer memory!");
    return false;
  }
  
  reset();
  Serial.print("Data buffer initialized: ");
  Serial.print(BUFFER_SIZE);
  Serial.print(" samples @ ");
  Serial.print(SAMPLE_RATE_HZ);
  Serial.println(" Hz");
  
  return true;
}

void DataBuffer::reset() {
  write_index = 0;
  sample_count = 0;
  buffer_full = false;
  last_sample_time = 0;
  buffer_start_time = micros();
}

bool DataBuffer::shouldSample() {
  unsigned long current_time = micros();
  
  // Check if enough time has passed since last sample
  if (last_sample_time == 0 || (current_time - last_sample_time) >= SAMPLING_INTERVAL_US) {
    return true;
  }
  
  return false;
}

bool DataBuffer::addSample(int32_t x, int32_t y, int32_t z) {
  if (!buffer || buffer_full) {
    return false;
  }
  
  unsigned long current_time = micros();
  
  // Store sample
  buffer[write_index].x = x;
  buffer[write_index].y = y;
  buffer[write_index].z = z;
  buffer[write_index].timestamp_us = current_time;
  
  // Update indices
  write_index++;
  sample_count++;
  last_sample_time = current_time;
  
  // Check if buffer is full
  if (write_index >= BUFFER_SIZE) {
    buffer_full = true;
    write_index = 0;  // Reset for next buffer cycle
  }
  
  return true;
}

void DataBuffer::calculateStats(BufferStats& stats) {
  if (!buffer || sample_count == 0) {
    memset(&stats, 0, sizeof(stats));
    return;
  }
  
  // Initialize min/max values
  stats.min_x = stats.max_x = buffer[0].x;
  stats.min_y = stats.max_y = buffer[0].y;
  stats.min_z = stats.max_z = buffer[0].z;
  
  // Calculate sums for averages
  long long sum_x = 0, sum_y = 0, sum_z = 0;
  long long sum_sq_x = 0, sum_sq_y = 0, sum_sq_z = 0;
  
  for (uint16_t i = 0; i < sample_count; i++) {
    int32_t x = buffer[i].x;
    int32_t y = buffer[i].y;
    int32_t z = buffer[i].z;
    
    // Sum for averages
    sum_x += x;
    sum_y += y;
    sum_z += z;
    
    // Sum of squares for RMS
    sum_sq_x += (long long)x * x;
    sum_sq_y += (long long)y * y;
    sum_sq_z += (long long)z * z;
    
    // Min/max
    if (x < stats.min_x) stats.min_x = x;
    if (x > stats.max_x) stats.max_x = x;
    if (y < stats.min_y) stats.min_y = y;
    if (y > stats.max_y) stats.max_y = y;
    if (z < stats.min_z) stats.min_z = z;
    if (z > stats.max_z) stats.max_z = z;
  }
  
  // Calculate averages
  stats.avg_x = (float)sum_x / sample_count;
  stats.avg_y = (float)sum_y / sample_count;
  stats.avg_z = (float)sum_z / sample_count;

  #if ENABLE_DEBUG_OUTPUT
  static unsigned long last_buffer_debug = 0;
  if (millis() - last_buffer_debug > 5000) {  // Debug every 5 seconds
    Serial.printf("[BUFFER-CALC] Sample count: %d\n", sample_count);
    Serial.printf("[BUFFER-CALC] Sum values: X=%lld, Y=%lld, Z=%lld\n", sum_x, sum_y, sum_z);
    Serial.printf("[BUFFER-CALC] Calculated averages: X=%.3f, Y=%.3f, Z=%.3f\n", 
                  stats.avg_x, stats.avg_y, stats.avg_z);
    Serial.printf("[BUFFER-CALC] Min/Max: X=[%.1f,%.1f], Y=[%.1f,%.1f], Z=[%.1f,%.1f]\n", 
                  stats.min_x, stats.max_x, stats.min_y, stats.max_y, stats.min_z, stats.max_z);
    last_buffer_debug = millis();
  }
  #endif
  
  // Calculate RMS
  stats.rms_x = sqrt((float)sum_sq_x / sample_count);
  stats.rms_y = sqrt((float)sum_sq_y / sample_count);
  stats.rms_z = sqrt((float)sum_sq_z / sample_count);
  
  // Other stats
  stats.sample_count = sample_count;
  if (sample_count > 0) {
    stats.duration_us = buffer[sample_count-1].timestamp_us - buffer[0].timestamp_us;
  } else {
    stats.duration_us = 0;
  }
}

void DataBuffer::printStats(const BufferStats& stats) {
  Serial.println("\n=== Buffer Statistics ===");
  Serial.print("Samples: "); Serial.print(stats.sample_count);
  Serial.print(" / Duration: "); Serial.print(stats.duration_us / 1000.0, 1); Serial.println(" ms");
  
  float actual_rate = (stats.sample_count * 1000000.0) / stats.duration_us;
  Serial.print("Actual sample rate: "); Serial.print(actual_rate, 1); Serial.println(" Hz");
  
  Serial.println("--- Averages ---");
  Serial.print("X: "); Serial.print(stats.avg_x, 1);
  Serial.print("\tY: "); Serial.print(stats.avg_y, 1);
  Serial.print("\tZ: "); Serial.println(stats.avg_z, 1);
  
  Serial.println("--- Min Values ---");
  Serial.print("X: "); Serial.print(stats.min_x);
  Serial.print("\tY: "); Serial.print(stats.min_y);
  Serial.print("\tZ: "); Serial.println(stats.min_z);
  
  Serial.println("--- Max Values ---");
  Serial.print("X: "); Serial.print(stats.max_x);
  Serial.print("\tY: "); Serial.print(stats.max_y);
  Serial.print("\tZ: "); Serial.println(stats.max_z);
  
  Serial.println("--- RMS Values ---");
  Serial.print("X: "); Serial.print(stats.rms_x, 1);
  Serial.print("\tY: "); Serial.print(stats.rms_y, 1);
  Serial.print("\tZ: "); Serial.println(stats.rms_z, 1);
  
  Serial.println("========================\n");
}
