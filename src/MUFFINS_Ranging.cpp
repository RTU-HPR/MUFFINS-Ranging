#include "MUFFINS_Ranging.h"

// For the interrupt functionality of RadioLib
// For some reason, I couldn't properly include this in the header file
// So I had to include it here
// It is something to do with volatile variables
volatile bool _currently_ranging = false;
namespace RadioLib_Interupt
{
  /**
   * @brief Interrupt function for setting currently ranging when ranging has been received 
   */
  void ranging_done(void)
  {
    _currently_ranging = false;
  }
};

Ranging::Ranging(String component_name, void (*info_function)(String), void (*error_function)(String)) : Component_Base(component_name, info_function, error_function)
{
  _runtime_state.action_status_code = RADIOLIB_ERR_NONE;
}

Ranging::~Ranging()
{
  return;
}

bool Ranging::begin(const Config &config)
{
  // Copy the config to the local one
  _config = config;

  if (_config.timeout < 200)
  {
    error("Timeout is too low: " + String(_config.timeout) + " ms. Setting to 200 ms");
    _config.timeout = 200;
  }

  _radio = new Module(_config.cs, _config.dio1, _config.reset, _config.dio0, *(_config.spi_bus));

  // Try to initialize communication with LoRa
  _runtime_state.action_status_code = _radio.begin();

  // If initialization failed, print error
  if (_runtime_state.action_status_code != RADIOLIB_ERR_NONE)
  {
    error("Initialization failed with status code: " + String(_runtime_state.action_status_code));
    return false;
  }
  // Set interrupt behaviour
  _radio.setDio1Action(RadioLib_Interupt::ranging_done);

  if (!_configure())
  {
    error("Configuration failed!");
    return false;
  }
  info("Configured");

  // Set that radio has been initialized
  set_initialized(true);
  info("Initialized");

  return true;
}

bool Ranging::_configure()
{
  if (_radio.setFrequency(_config.frequency) == RADIOLIB_ERR_INVALID_FREQUENCY)
  {
    error("Frequency is invalid: " + String(_config.frequency));
    return false;
  };

  if (_radio.setOutputPower(_config.tx_power) == RADIOLIB_ERR_INVALID_OUTPUT_POWER)
  {
    error("Transmit power is invalid: " + String(_config.tx_power));
    return false;
  };

  if (_radio.setSpreadingFactor(_config.spreading) == RADIOLIB_ERR_INVALID_SPREADING_FACTOR)
  {
    error("Spreading factor is invalid: " + String(_config.spreading));
    return false;
  };

  if (_radio.setCodingRate(_config.coding_rate) == RADIOLIB_ERR_INVALID_CODING_RATE)
  {
    error("Coding rate is invalid: " + String(_config.coding_rate));
    return false;
  };

  if (_radio.setBandwidth(_config.signal_bw) == RADIOLIB_ERR_INVALID_BANDWIDTH)
  {
    error("Signal bandwidth is invalid: " + String(_config.signal_bw));
    return false;
  };

  if (_radio.setSyncWord(_config.sync_word) == RADIOLIB_ERR_INVALID_SYNC_WORD)
  {
    error("Sync word is invalid: " + String(_config.sync_word));
    return false;
  };

  return true;
}

bool Ranging::run_master()
{
  if (!initialized())
  {
    return false;
  }

  if (_config.mode != Mode::MASTER)
  {
    error("Master function can only be called in Master mode");
    return false;
  }

  // If already ranging, check if has timed out
  bool timeout = false;
  if (_currently_ranging)
  {
    if (millis() - _runtime_state.ranging_start_time > _config.timeout)
    {
      _currently_ranging = false;
      timeout = true;
    }
  }

  // If ranging has been received or timed out, read results if available
  if (!_currently_ranging)
  {
    if (!timeout)
    {
      _config.slaves[_runtime_state.current_slave_index].result.distance = _radio.getRangingResult();
      _config.slaves[_runtime_state.current_slave_index].result.received_millis = millis();
    }

    // Clean up from the previous time
    _radio.clearDio1Action();
    _radio.finishTransmit();

    // Move to the next slave
    _runtime_state.current_slave_index++;
    if (_runtime_state.current_slave_index >= _config.slave_count)
    {
      _runtime_state.current_slave_index = 0;
    }

    // Start transmitting
    _radio.setDio1Action(RadioLib_Interupt::ranging_done);
    _runtime_state.action_status_code = _radio.startRanging(true, _config.slaves[_runtime_state.current_slave_index].address);

    // If transmit failed, print error
    if (_runtime_state.action_status_code != RADIOLIB_ERR_NONE)
    {
      error("Starting ranging failed with status code: " + String(_runtime_state.action_status_code));
      return false;
    }
    _currently_ranging = true;
    _runtime_state.ranging_start_time = millis();

    // Return true only if we have received the signal. If timeout, return false
    if (!timeout)
    {
      return true;
    }
    return false;
  }
  return false;
}

bool Ranging::run_slave()
{
  if (!initialized())
  {
    return false;
  }

  if (_config.mode != Mode::SLAVE)
  {
    error("Transmit slave function can only be called in Slave mode");
    return false;
  }

  // If already ranging, check if has timed out
  bool timeout = false;
  if (_currently_ranging)
  {
    if (millis() - _runtime_state.ranging_start_time > _config.timeout)
    {
      _currently_ranging = false;
      timeout = true;
    }
  }

  // If ranging has been received or timed out, start ranging as slave
  if (!_currently_ranging)
  {
    // Clean up from the previous time
    _radio.clearDio1Action();
    _radio.finishTransmit();

    // Start transmitting
    _radio.setDio1Action(RadioLib_Interupt::ranging_done);
    _runtime_state.action_status_code = _radio.startRanging(false, _config.address);

    // If transmit failed, print error
    if (_runtime_state.action_status_code != RADIOLIB_ERR_NONE)
    {
      error("Starting ranging failed with status code: " + String(_runtime_state.action_status_code));
      return false;
    }

    _runtime_state.ranging_start_time = millis();
    _currently_ranging = true;

    // Return true only if we have received the signal. If timeout, return false
    if (!timeout)
    {
      return true;
    }
    return false;
  }
  return false;
}