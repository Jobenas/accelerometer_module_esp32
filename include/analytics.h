#ifndef ANALYTICS_H
#define ANALYTICS_H

#include <Arduino.h>
#include "data_buffer.h"

// Analytics data structure for running statistics
struct AnalyticsData {
  // Current window statistics (updated every second)
  float current_avg_x = 0.0;
  float current_avg_y = 0.0;
  float current_avg_z = 0.0;
  float current_max_x = 0.0;
  float current_max_y = 0.0;
  float current_max_z = 0.0;
  float current_min_x = 0.0;
  float current_min_y = 0.0;
  float current_min_z = 0.0;
  float current_std_x = 0.0;
  float current_std_y = 0.0;
  float current_std_z = 0.0;
  float current_rms_x = 0.0;
  float current_rms_y = 0.0;
  float current_rms_z = 0.0;
  
  // Running statistics (accumulated over time)
  float running_avg_x = 0.0;
  float running_avg_y = 0.0;
  float running_avg_z = 0.0;
  float running_std_x = 0.0;
  float running_std_y = 0.0;
  float running_std_z = 0.0;
  float running_rms_x = 0.0;
  float running_rms_y = 0.0;
  float running_rms_z = 0.0;
  float global_max_x = 0.0;
  float global_max_y = 0.0;
  float global_max_z = 0.0;
  float global_min_x = 0.0;
  float global_min_y = 0.0;
  float global_min_z = 0.0;
  
  // Metadata
  unsigned long window_count = 0;
  unsigned long last_update_time = 0;
  bool data_valid = false;
};

class Analytics {
private:
  AnalyticsData analytics_data;
  bool initialized;
  
public:
  Analytics();
  
  // Initialization
  bool begin();
  
  // Data processing
  void processBufferStats(const BufferStats& stats);
  void resetRunningStats();
  
  // Data access
  AnalyticsData getAnalyticsData() const { return analytics_data; }
  void printAnalytics() const;
  void printRunningStats() const;
  
  // Status
  bool isInitialized() const { return initialized; }
  unsigned long getWindowCount() const { return analytics_data.window_count; }
};

// Global analytics instance
extern Analytics analytics;

#endif // ANALYTICS_H
