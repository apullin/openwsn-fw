/**
\brief Drivers for the magnetometer of the GINA2.2b/c board.

\author Thomas Watteyne <watteyne@eecs.berkeley.edu>, August 2010
*/

#include "openwsn.h"
#include "magnetometer.h"

//=========================== variables =======================================

//register backup variables
uint8_t reg_CONF_A;
uint8_t reg_CONF_B;
uint8_t reg_MODE;
uint8_t reg_STATUS;
uint8_t reg_ID_A;
uint8_t reg_ID_B;
uint8_t reg_ID_C;
bool magnetometer_configured = FALSE;

//=========================== prototypes ======================================

//=========================== public ==========================================

void magnetometer_init() {
   i2c_write_register(1,MAGNETOMETER_I2C_ADDR,
         MAGNETOMETER_REG_CONF_A_ADDR,
         MAGNETOMETER_REG_CONF_A_SETTING);
   i2c_write_register(1,MAGNETOMETER_I2C_ADDR,
         MAGNETOMETER_REG_CONF_B_ADDR,
         MAGNETOMETER_REG_CONF_B_SETTING);
   magnetometer_enable();
   magnetometer_configured = TRUE;
}

void magnetometer_enable() {
   i2c_write_register(1,MAGNETOMETER_I2C_ADDR,
         MAGNETOMETER_REG_MODE_ADDR,
         MAGNETOMETER_REG_MODE_WAKEUP);
}

void magnetometer_disable() {
   i2c_write_register(1,MAGNETOMETER_I2C_ADDR,
                      MAGNETOMETER_REG_MODE_ADDR,
                      MAGNETOMETER_REG_MODE_SLEEP);
}

void magnetometer_get_config() {
   if (magnetometer_configured==TRUE) {
      i2c_read_registers(1,MAGNETOMETER_I2C_ADDR,MAGNETOMETER_REG_CONF_A_ADDR  ,1,&reg_CONF_A);
      i2c_read_registers(1,MAGNETOMETER_I2C_ADDR,MAGNETOMETER_REG_CONF_B_ADDR  ,1,&reg_CONF_B);
      i2c_read_registers(1,MAGNETOMETER_I2C_ADDR,MAGNETOMETER_REG_MODE_ADDR    ,1,&reg_MODE);
      i2c_read_registers(1,MAGNETOMETER_I2C_ADDR,MAGNETOMETER_REG_STATUS_ADDR  ,1,&reg_STATUS);
      i2c_read_registers(1,MAGNETOMETER_I2C_ADDR,MAGNETOMETER_REG_ID_A_ADDR    ,1,&reg_ID_A);
      i2c_read_registers(1,MAGNETOMETER_I2C_ADDR,MAGNETOMETER_REG_ID_B_ADDR    ,1,&reg_ID_B);
      i2c_read_registers(1,MAGNETOMETER_I2C_ADDR,MAGNETOMETER_REG_ID_C_ADDR    ,1,&reg_ID_C);
   }
}

void magnetometer_get_measurement(uint8_t* spaceToWrite) {
   uint8_t i;
   if (magnetometer_configured==TRUE) {
      i2c_read_registers(1,MAGNETOMETER_I2C_ADDR,MAGNETOMETER_REG_DATA_X_H_ADDR,6,spaceToWrite);
   } else {
      for (i=0;i<6;i++) {
         spaceToWrite[i] = 0;
      }
   }
}

//=========================== private =========================================