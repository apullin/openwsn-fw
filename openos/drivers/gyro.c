/*
 * Drivers for the gyroscope of the GINA2.2b/c board.
 *
 * Author:
 * Thomas Watteyne <watteyne@eecs.berkeley.edu>, August 2010
 */

#include "openwsn.h"
#include "gyro.h"

//===================================== variables =============================

//register backup variables
uint8_t reg_WHO_AM_I;
uint8_t reg_SMPLRT_DIV;
uint8_t reg_DLPF_FS;
uint8_t reg_INT_CFG;
uint8_t reg_INT_STATUS;
uint8_t reg_PWR_MGM;
bool gyro_configured = FALSE;

//===================================== public ================================

void gyro_init() {
   i2c_write_register(GYRO_I2C_ADDR, GYRO_REG_SMPLRT_DIV_ADDR, GYRO_REG_SMPLRT_DIV_SETTING);
   i2c_write_register(GYRO_I2C_ADDR, GYRO_REG_DLPF_FS_ADDR,    GYRO_REG_DLPF_FS_SETTING);
   i2c_write_register(GYRO_I2C_ADDR, GYRO_REG_INT_CFG_ADDR,    GYRO_REG_INT_CFG_SETTING);
   i2c_write_register(GYRO_I2C_ADDR, GYRO_REG_PWR_MGM_ADDR,    GYRO_REG_PWR_MGM_SETTING);
   gyro_configured = TRUE;
}

void gyro_disable() {
   i2c_write_register(GYRO_I2C_ADDR, GYRO_REG_PWR_MGM_ADDR,    GYRO_REG_PWR_MGM_SLEEP);
}

void gyro_get_config() {
   if (gyro_configured==TRUE) {
      i2c_read_registers(GYRO_I2C_ADDR, GYRO_REG_WHO_AM_I_ADDR   ,1,&reg_WHO_AM_I);
      i2c_read_registers(GYRO_I2C_ADDR, GYRO_REG_SMPLRT_DIV_ADDR ,1,&reg_SMPLRT_DIV);
      i2c_read_registers(GYRO_I2C_ADDR, GYRO_REG_DLPF_FS_ADDR    ,1,&reg_DLPF_FS);
      i2c_read_registers(GYRO_I2C_ADDR, GYRO_REG_INT_CFG_ADDR    ,1,&reg_INT_CFG);
      i2c_read_registers(GYRO_I2C_ADDR, GYRO_REG_INT_STATUS_ADDR ,1,&reg_INT_STATUS);
      i2c_read_registers(GYRO_I2C_ADDR, GYRO_REG_PWR_MGM_ADDR    ,1,&reg_PWR_MGM);
   }
}

void gyro_get_measurement(uint8_t* spaceToWrite) {
   uint8_t i;
   if (gyro_configured==TRUE) {
      i2c_read_registers(GYRO_I2C_ADDR, GYRO_REG_TEMP_OUT_H_ADDR,8,spaceToWrite);
   } else {
      for (i=0;i<8;i++) {
         spaceToWrite[i] = 0;
      }
   }
}
