#include <Arduino.h>
#include <MUFFINS_Ranging.h>

const int SPI0_RX = 4;
const int SPI0_TX = 3;
const int SPI0_SCK = 2;

// Set to true if this is the master device and false if this is the slave device
#define MASTER_DEVICE true
#if MASTER_DEVICE

Ranging master;
const int slave_count = 1;
Ranging::Slave slaves[slave_count] = {
  {
    .address = 0x12345678
  }
};

Ranging::Config master_config
{
  .frequency = 2405.6,
  .cs = 15,
  .dio0 = 11,
  .dio1 = 12,
  .reset = 13,
  .sync_word = 0xF5,
  .tx_power = 10,
  .spreading = 10,
  .coding_rate = 7,
  .signal_bw = 406.25,
  .spi_bus = &SPI,
  .mode = Ranging::Mode::MASTER,
  .timeout = 200,
  .slave_count = slave_count,
  .slaves = &*slaves
};

void setup()
{
  Serial.begin(115200);
  while (!Serial)
  {
    delay(1000);
  }

  if (SPI.setRX(SPI0_RX) && SPI.setTX(SPI0_TX) && SPI.setSCK(SPI0_SCK))
  {
    SPI.begin();
  }

  if (!master.begin(master_config))
  {
    Serial.println("Master failed to initialize");
    while(1)
    ;
  }

  Serial.println("Master initialized");
}

void loop()
{
  if (master.run_master())
  {
    for (int i = 0; i < slave_count; i++)
    {
      Serial.println("SLAVE " + String(i) + " RESULT:");
      Serial.println("Received at: " + String(slaves[i].result.received_millis) + " ms");
      Serial.println("Distance: " + String(slaves[i].result.distance) + " m");
      Serial.println();
    }
  }
}
  
#endif

#if !MASTER_DEVICE
Ranging slave;

Ranging::Config slave_config
{
  .frequency = 2405.6,
  .cs = 15,
  .dio0 = 11,
  .dio1 = 12,
  .reset = 13,
  .sync_word = 0xF5,
  .tx_power = 10,
  .spreading = 10,
  .coding_rate = 7,
  .signal_bw = 406.25,
  .spi_bus = &SPI,
  .mode = Ranging::Mode::SLAVE,
  .timeout = 200,
  .address = 0x12345678
};

void setup()
{
  Serial.begin(115200);

  if (SPI.setRX(SPI0_RX) && SPI.setTX(SPI0_TX) && SPI.setSCK(SPI0_SCK))
  {
    SPI.begin();
  }

  if (!slave.begin(slave_config))
  {
    Serial.println("Slave failed to initialize");
    while(1)
    ;
  }

  Serial.println("Slave initialized");
}

void loop()
{
  slave.run_slave();
}

# endif
