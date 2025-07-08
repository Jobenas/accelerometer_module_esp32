#ifndef DATA_BUFFER_H
#define DATA_BUFFER_H

#include <Arduino.h>

// Buffer configuration
#define SAMPLE_RATE_HZ 1000
#define BUFFER_SIZE 1000  // 1 second worth of samples
#define SAMPLING_INTERVAL_US (1000000 / SAMPLE_RATE_HZ)  // 1000 microseconds

// Sample data structure
struct AccelSample {
  int32_t x;
  int32_t y;
  int32_t z;
  unsigned long timestamp_us;
};

// Buffer statistics
struct BufferStats {
  float avg_x;
  float avg_y;
  float avg_z;
  float max_x;
  float max_y;
  float max_z;
  float min_x;
  float min_y;
  float min_z;
  float rms_x;
  float rms_y;
  float rms_z;
  uint16_t sample_count;
  unsigned long duration_us;
};

class DataBuffer {
private:
  AccelSample* buffer;
  uint16_t write_index;
  uint16_t sample_count;
  bool buffer_full;
  unsigned long last_sample_time;
  unsigned long buffer_start_time;
  
public:
  DataBuffer();
  ~DataBuffer();
  
  // Buffer management
  bool begin();
  void reset();
  
  // Data collection
  bool addSample(int32_t x, int32_t y, int32_t z);
  bool shouldSample();
  
  // Buffer status
  bool isFull() const { return buffer_full; }
  uint16_t getSampleCount() const { return sample_count; }
  uint16_t getCapacity() const { return BUFFER_SIZE; }
  
  // Data processing
  void calculateStats(BufferStats& stats);
  void printStats(const BufferStats& stats);
  
  // Access samples
  const AccelSample* getSamples() const { return buffer; }
};

#endif // DATA_BUFFER_H
