#pragma once
#include <Arduino.h>
#include <RadioLib.h>
#include <MUFFINS_Component_Base.h>

class Ranging : public Component_Base
{
private:
  struct Result
  {
    double distance;
    unsigned long received_millis;
  };

  struct Runtime_State
  {
    int action_status_code;
    unsigned long ranging_start_time;
    int current_slave_index;
  } _runtime_state;

public:
  enum Mode
  {
    SLAVE,
    MASTER
  };

  struct Slave
  {
    Result result;
    uint32_t address;
  };

  struct Config
  {
    float frequency;
    int cs;
    int dio0;
    int dio1;
    int reset;
    int sync_word;
    int tx_power; // dBm
    int spreading;
    int coding_rate;
    float signal_bw; // kHz
    SPIClass *spi_bus; // Example &SPI
    Mode mode;         // Master or Slave
    int timeout;       // Timeout for ranging in ms (The absoulute minimum time is 180 ms, but taking into account other factors, 200 ms is the minimum. If the timeout is set to less than 200 ms, it will be automatically set to 200 ms!) 
    int slave_count;   // Number of slaves (For master mode)
    Slave *slaves;     // Array of slaves (For master mode)
    uint32_t address;  // Address of the slave (For slave mode)
  };

private:
  // Radio object
  SX1280 _radio = new Module(-1, -1, -1, -1);

  // Local objects
  Config _config;

  /**
   * @brief Configure radio module modulation parameters (frequency, power, etc.) for exact things that are set check the function
   *
   * @return true If configured successfully
   */
  bool _configure();

public:
  /**
   * @brief Construct a new Ranging object
   */
  Ranging(String component_name = "Ranging", void (*info_function)(String) = nullptr, void (*error_function)(String) = nullptr);

  /**
   * @brief Destroy the Ranging object
   */
  ~Ranging();

  /**
   * @brief
   *
   * @param config Ranging config struct to be used
   * @return true If configured successfully
   */
  bool begin(const Config &config);

  /**
   * @brief Run ranging process (receive/transmit) as the master
   *
   * @return true If ranging data is received successfully
   * @note Run this function only on the master device
   */
  bool run_master();

  /**
   * @brief Run ranging process (receive/transmit) as the slave
   *
   * @return true If ranging signal from master is received successfully
   * @note Run this function only on the slave device
   */
  bool run_slave();
};