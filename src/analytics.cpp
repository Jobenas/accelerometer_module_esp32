#include "analytics.h"

Analytics::Analytics() : initialized(false) {
  analytics_data = AnalyticsData();
}

bool Analytics::begin() {
  if (initialized) {
    return true;
  }
  
  resetRunningStats();
  initialized = true;
  Serial.println("Analytics initialized successfully");
  
  return true;
}

void Analytics::processBufferStats(const BufferStats& stats) {
  if (!initialized) {
    Serial.println("Analytics not initialized!");
    return;
  }

  // IMPORTANT: Convert raw ADXL355 values to g-values before processing
  const float ADXL355_SCALE_FACTOR = 256000.0f;
  
  float current_avg_x_g = stats.avg_x / ADXL355_SCALE_FACTOR;
  float current_avg_y_g = stats.avg_y / ADXL355_SCALE_FACTOR;
  float current_avg_z_g = stats.avg_z / ADXL355_SCALE_FACTOR;
  float current_max_x_g = stats.max_x / ADXL355_SCALE_FACTOR;
  float current_max_y_g = stats.max_y / ADXL355_SCALE_FACTOR;
  float current_max_z_g = stats.max_z / ADXL355_SCALE_FACTOR;
  float current_min_x_g = stats.min_x / ADXL355_SCALE_FACTOR;
  float current_min_y_g = stats.min_y / ADXL355_SCALE_FACTOR;
  float current_min_z_g = stats.min_z / ADXL355_SCALE_FACTOR;
  float current_rms_x_g = stats.rms_x / ADXL355_SCALE_FACTOR;
  float current_rms_y_g = stats.rms_y / ADXL355_SCALE_FACTOR;
  float current_rms_z_g = stats.rms_z / ADXL355_SCALE_FACTOR;
  
  // Calculate standard deviation from RMS and average
  // std = sqrt(RMS^2 - avg^2)
  float current_std_x_g = sqrt(fmax(0.0f, (current_rms_x_g * current_rms_x_g) - (current_avg_x_g * current_avg_x_g)));
  float current_std_y_g = sqrt(fmax(0.0f, (current_rms_y_g * current_rms_y_g) - (current_avg_y_g * current_avg_y_g)));
  float current_std_z_g = sqrt(fmax(0.0f, (current_rms_z_g * current_rms_z_g) - (current_avg_z_g * current_avg_z_g)));

  #if ENABLE_DEBUG_OUTPUT
  static unsigned long last_debug = 0;
  if (millis() - last_debug > 4000) {
    Serial.printf("[ANALYTICS] Raw: X=%.1f, Y=%.1f, Z=%.1f -> G: X=%.6f, Y=%.6f, Z=%.6f\n", 
                  stats.avg_x, stats.avg_y, stats.avg_z, current_avg_x_g, current_avg_y_g, current_avg_z_g);
    Serial.printf("[ANALYTICS] STD: X=%.6f, Y=%.6f, Z=%.6f | RMS: X=%.6f, Y=%.6f, Z=%.6f\n",
                  current_std_x_g, current_std_y_g, current_std_z_g, current_rms_x_g, current_rms_y_g, current_rms_z_g);
    last_debug = millis();
  }
  #endif

  analytics_data.current_avg_x = current_avg_x_g;
  analytics_data.current_avg_y = current_avg_y_g;
  analytics_data.current_avg_z = current_avg_z_g;
  analytics_data.current_max_x = current_max_x_g;
  analytics_data.current_max_y = current_max_y_g;
  analytics_data.current_max_z = current_max_z_g;
  analytics_data.current_min_x = current_min_x_g;
  analytics_data.current_min_y = current_min_y_g;
  analytics_data.current_min_z = current_min_z_g;
  analytics_data.current_std_x = current_std_x_g;
  analytics_data.current_std_y = current_std_y_g;
  analytics_data.current_std_z = current_std_z_g;
  analytics_data.current_rms_x = current_rms_x_g;
  analytics_data.current_rms_y = current_rms_y_g;
  analytics_data.current_rms_z = current_rms_z_g;
  
  const float alpha = 0.1f;
  if (analytics_data.window_count == 0) {
    analytics_data.running_avg_x = current_avg_x_g;
    analytics_data.running_avg_y = current_avg_y_g;
    analytics_data.running_avg_z = current_avg_z_g;
    analytics_data.running_std_x = current_std_x_g;
    analytics_data.running_std_y = current_std_y_g;
    analytics_data.running_std_z = current_std_z_g;
    analytics_data.running_rms_x = current_rms_x_g;
    analytics_data.running_rms_y = current_rms_y_g;
    analytics_data.running_rms_z = current_rms_z_g;
    analytics_data.global_max_x = current_max_x_g;
    analytics_data.global_max_y = current_max_y_g;
    analytics_data.global_max_z = current_max_z_g;
    analytics_data.global_min_x = current_min_x_g;
    analytics_data.global_min_y = current_min_y_g;
    analytics_data.global_min_z = current_min_z_g;
  } else {
    analytics_data.running_avg_x = (alpha * current_avg_x_g) + ((1.0f - alpha) * analytics_data.running_avg_x);
    analytics_data.running_avg_y = (alpha * current_avg_y_g) + ((1.0f - alpha) * analytics_data.running_avg_y);
    analytics_data.running_avg_z = (alpha * current_avg_z_g) + ((1.0f - alpha) * analytics_data.running_avg_z);
    analytics_data.running_std_x = (alpha * current_std_x_g) + ((1.0f - alpha) * analytics_data.running_std_x);
    analytics_data.running_std_y = (alpha * current_std_y_g) + ((1.0f - alpha) * analytics_data.running_std_y);
    analytics_data.running_std_z = (alpha * current_std_z_g) + ((1.0f - alpha) * analytics_data.running_std_z);
    analytics_data.running_rms_x = (alpha * current_rms_x_g) + ((1.0f - alpha) * analytics_data.running_rms_x);
    analytics_data.running_rms_y = (alpha * current_rms_y_g) + ((1.0f - alpha) * analytics_data.running_rms_y);
    analytics_data.running_rms_z = (alpha * current_rms_z_g) + ((1.0f - alpha) * analytics_data.running_rms_z);
    
    if (current_max_x_g > analytics_data.global_max_x) analytics_data.global_max_x = current_max_x_g;
    if (current_max_y_g > analytics_data.global_max_y) analytics_data.global_max_y = current_max_y_g;
    if (current_max_z_g > analytics_data.global_max_z) analytics_data.global_max_z = current_max_z_g;
    if (current_min_x_g < analytics_data.global_min_x) analytics_data.global_min_x = current_min_x_g;
    if (current_min_y_g < analytics_data.global_min_y) analytics_data.global_min_y = current_min_y_g;
    if (current_min_z_g < analytics_data.global_min_z) analytics_data.global_min_z = current_min_z_g;
  }
  
  analytics_data.window_count++;
  analytics_data.last_update_time = millis();
  analytics_data.data_valid = true;
  
  #if ENABLE_ANALYTICS_DEBUG
  Serial.print("Analytics updated - Window #"); 
  Serial.println(analytics_data.window_count);
  #endif
}

void Analytics::resetRunningStats() {
  analytics_data.running_avg_x = 0.0;
  analytics_data.running_avg_y = 0.0;
  analytics_data.running_avg_z = 0.0;
  analytics_data.running_std_x = 0.0;
  analytics_data.running_std_y = 0.0;
  analytics_data.running_std_z = 0.0;
  analytics_data.running_rms_x = 0.0;
  analytics_data.running_rms_y = 0.0;
  analytics_data.running_rms_z = 0.0;
  analytics_data.global_max_x = 0.0;
  analytics_data.global_max_y = 0.0;
  analytics_data.global_max_z = 0.0;
  analytics_data.global_min_x = 0.0;
  analytics_data.global_min_y = 0.0;
  analytics_data.global_min_z = 0.0;
  analytics_data.window_count = 0;
  analytics_data.last_update_time = 0;
  analytics_data.data_valid = false;
  
  Serial.println("Analytics running stats reset");
}

void Analytics::printAnalytics() const {
  #if ENABLE_ANALYTICS_DEBUG
  if (!analytics_data.data_valid) {
    Serial.println("No analytics data available yet");
    return;
  }
  
  Serial.println("\n=== Current Window Analytics ===");
  Serial.printf("Window #%lu (updated %lu ms ago)\n", 
    analytics_data.window_count, 
    millis() - analytics_data.last_update_time);
  
  Serial.println("Current Averages (g):");
  Serial.printf("  X: %8.4f  Y: %8.4f  Z: %8.4f\n", 
    analytics_data.current_avg_x, 
    analytics_data.current_avg_y, 
    analytics_data.current_avg_z);
  
  Serial.println("Current Maximums (g):");
  Serial.printf("  X: %8.4f  Y: %8.4f  Z: %8.4f\n", 
    analytics_data.current_max_x, 
    analytics_data.current_max_y, 
    analytics_data.current_max_z);
  
  Serial.println("Current Minimums (g):");
  Serial.printf("  X: %8.4f  Y: %8.4f  Z: %8.4f\n", 
    analytics_data.current_min_x, 
    analytics_data.current_min_y, 
    analytics_data.current_min_z);
  
  Serial.println("===============================");
  #endif
}

void Analytics::printRunningStats() const {
  #if ENABLE_ANALYTICS_DEBUG
  if (!analytics_data.data_valid) {
    Serial.println("No running stats available yet");
    return;
  }
  
  Serial.println("\n=== Running Analytics ===");
  Serial.printf("Total windows processed: %lu\n", analytics_data.window_count);
  Serial.printf("Data collection time: %.1f seconds\n", analytics_data.window_count * 1.0);
  
  Serial.println("Running Averages (g):");
  Serial.printf("  X: %8.4f  Y: %8.4f  Z: %8.4f\n", 
    analytics_data.running_avg_x, 
    analytics_data.running_avg_y, 
    analytics_data.running_avg_z);
  
  Serial.println("Global Maximums (g):");
  Serial.printf("  X: %8.4f  Y: %8.4f  Z: %8.4f\n", 
    analytics_data.global_max_x, 
    analytics_data.global_max_y, 
    analytics_data.global_max_z);
  
  Serial.println("Global Minimums (g):");
  Serial.printf("  X: %8.4f  Y: %8.4f  Z: %8.4f\n", 
    analytics_data.global_min_x, 
    analytics_data.global_min_y, 
    analytics_data.global_min_z);
  
  Serial.println("========================");
  #endif
}
