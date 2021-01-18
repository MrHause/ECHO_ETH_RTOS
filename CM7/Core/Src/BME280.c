/*
 * BME280.c
 *
 *  Created on: Apr 15, 2020
 *      Author: mrhau
 */
#include "main.h"
#include "BME280.h"
#include "stm32h7xx_hal.h"
#include "math.h"

//*****************pvt variables*****************************
I2C_HandleTypeDef *i2c_handler;

uint8_t _temperature_res, _pressure_oversampling, _huminidity_oversampling,  _mode, h1, h3;
int8_t h6;
int16_t t2, t3, p2, p3, p4, p5, p6, p7, p8, p9, h2, h4, h5;
uint16_t t1, p1;
int32_t t_fine;
static uint32_t altitude;

//****************functions**********************************
void BME280_setAltitude(uint32_t value){
	altitude = value;
}
uint32_t BME280_getAltitude(){
	return altitude;
}


uint8_t Read8bit(uint8_t reg_addr){
	uint8_t read_data;
	HAL_I2C_Mem_Read(i2c_handler, Dev_ADDR, reg_addr, 1, &read_data, 1, 10);
	return read_data;
}

uint16_t Read16bit(uint8_t reg_addr){
	uint8_t read_data[2];
	HAL_I2C_Mem_Read(i2c_handler, Dev_ADDR, reg_addr, 1, read_data, 2, 10);
	uint16_t temp = ((read_data[0]<<8)|read_data[1]);
	return temp;
}

uint32_t Read24bit(uint8_t reg_addr){
	uint8_t read_data[3];
	HAL_I2C_Mem_Read(i2c_handler, Dev_ADDR, reg_addr, 1, read_data, 3, 10);
	uint32_t temp = ((read_data[0]<<16)|(read_data[1]<<8)|read_data[2]);
	return temp;
}

uint16_t Read16bit_rev(uint8_t reg_addr){
	uint16_t temp;
	temp = Read16bit(reg_addr);
	return (temp>>8)|(temp<<8);
}

void Write8bit(uint8_t reg_addr, uint8_t towrite){
	HAL_I2C_Mem_Write(i2c_handler, Dev_ADDR, reg_addr, 1, &towrite, 1, 10);
}

uint8_t ReadingCalibrationCplt(void){
	uint8_t Status = Read8bit(BME280_STATUS);
	return ((Status & 1) != 0);
}

void BME280_SetCfg(uint8_t time, uint8_t filter){
	Write8bit(BME280_CONFIG, (uint8_t)(((time & 0x7) << 5) | ((filter & 0x7) << 2)) & 0xFC);
}

void BME280_init(I2C_HandleTypeDef *i2c_h, uint8_t temperature_resolution, uint8_t pressure_oversampling, uint8_t huminidity_oversampling, uint8_t mode){
	//store setting in global variable
	i2c_handler = i2c_h;
	_mode = mode;
	_pressure_oversampling = pressure_oversampling;
	_huminidity_oversampling = huminidity_oversampling;
	_temperature_res = temperature_resolution;

	while(Read8bit(BME280_CHIPID) != 0x60);
	Write8bit(BME280_SOFTRESET, 0xB6);
	HAL_Delay(30);

	while(ReadingCalibrationCplt())
		HAL_Delay(10);

	//*********GET CALIBRATION VARIABLES***********************
	t1 = Read16bit_rev(BME280_DIG_T1);
	t2 = Read16bit_rev(BME280_DIG_T2);
	t3 = Read16bit_rev(BME280_DIG_T3);

	p1 = Read16bit_rev(BME280_DIG_P1);
	p2 = Read16bit_rev(BME280_DIG_P2);
	p3 = Read16bit_rev(BME280_DIG_P3);
	p4 = Read16bit_rev(BME280_DIG_P4);
	p5 = Read16bit_rev(BME280_DIG_P5);
	p6 = Read16bit_rev(BME280_DIG_P6);
	p7 = Read16bit_rev(BME280_DIG_P7);
	p8 = Read16bit_rev(BME280_DIG_P8);
	p9 = Read16bit_rev(BME280_DIG_P9);

	h1 = Read8bit(BME280_DIG_H1);
	h2 = Read16bit_rev(BME280_DIG_H2);
	h3 = Read8bit(BME280_DIG_H3);
	h4 = ((Read8bit(BME280_DIG_H4) << 4 ) | (Read8bit(BME280_DIG_H4+1) & 0xF));
	h5 = ((Read8bit(BME280_DIG_H5+1) << 4) | (Read8bit(BME280_DIG_H5) >> 4));
	h6 = (int8_t)Read8bit(BME280_DIG_H6);

	uint8_t HumReg = 0;
	HumReg |= _huminidity_oversampling;
	Write8bit(BME280_HUM_CONTROL, HumReg); //enable humidity
	//set init in control register
	Write8bit(BME280_CONTROL, ((temperature_resolution<<5) | (pressure_oversampling<<2) | mode));

	//setup for normal mode. if set other mode user have to call that function BME280_SetCfg by yourself
	if(mode == BME280_NORMALMODE)
	{
		BME280_SetCfg(BME280_STANDBY_MS_0_5, BME280_FILTER_OFF);
	}
}

float BME280_GetTemperature(){

	if(_mode == BME280_FORCEDMODE){
		uint8_t mode;
		uint8_t ctrl_reg = Read8bit(BME280_CONTROL); //get control register
		ctrl_reg &= ~(0x03); //?
		ctrl_reg |= BME280_FORCEDMODE; //call forcemode messure
		Write8bit(BME280_CONTROL, ctrl_reg); //write data to register
		mode = Read8bit(BME280_CONTROL);
		mode &= 0x03;
		if(mode == BME280_FORCEDMODE){
			while(1){ //wait until end conversion
				mode = Read8bit(BME280_CONTROL);
				mode &= 0x03;
				if(mode == BME280_SLEEPMODE)
					break;
			}
		}
	}

	  int32_t var1, var2;
	  int32_t adc_T = Read24bit(BME280_TEMPDATA); //read data from register
	  if (adc_T == 0x800000)
		  return -99;

	  adc_T >>= 4;

	  var1  = ((((adc_T>>3) - ((int32_t)t1 <<1))) *
			  ((int32_t)t2)) >> 11;

	  var2  = (((((adc_T>>4) - ((int32_t)t1)) *
			  ((adc_T>>4) - ((int32_t)t1))) >> 12) *
			  ((int32_t)t3)) >> 14;

	  t_fine = var1 + var2;

	  float T  = (t_fine * 5 + 128) >> 8;
	  return T/100;

	  return -99;
}

float BME280_GetHuminidity(){
	BME280_GetTemperature();
	int32_t adc_H = Read16bit(BME280_HUMIDDATA);
	if (adc_H == 0x8000) // value in case humidity measurement was disabled
		   return -99; //error
		int32_t v_x1_u32r;

		v_x1_u32r = (t_fine - ((int32_t)76800));

		  v_x1_u32r = (((((adc_H << 14) - (((int32_t)h4) << 20) -
						  (((int32_t)h5) * v_x1_u32r)) + ((int32_t)16384)) >> 15) *
					   (((((((v_x1_u32r * ((int32_t)h6)) >> 10) *
							(((v_x1_u32r * ((int32_t)h3)) >> 11) + ((int32_t)32768))) >> 10) +
						  ((int32_t)2097152)) * ((int32_t)h2) + 8192) >> 14));

		  v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) *
									 ((int32_t)h1)) >> 4));

		  v_x1_u32r = (v_x1_u32r < 0) ? 0 : v_x1_u32r;
		  v_x1_u32r = (v_x1_u32r > 419430400) ? 419430400 : v_x1_u32r;
		  float h = (v_x1_u32r>>12);
		return  h / 1024.0;
}

int32_t BME280_GetPressure(){
	int64_t var1,var2,p;
	BME280_GetTemperature();
	int32_t adc_P = Read24bit(BME280_PRESSUREDATA);
	adc_P >>= 4;

	var1 = ((int64_t)t_fine) - 128000;
	var2 = var1 * var1 * (int64_t)p6;
	var2 = var2 + ((var1*(int64_t)p5)<<17);
	var2 = var2 + (((int64_t)p4)<<35);
	var1 = ((var1 * var1 * (int64_t)p3)>>8) +
			((var1 * (int64_t)p2)<<12);
	var1 = (((((int64_t)1)<<47)+var1))*((int64_t)p1)>>33;

	if (var1 == 0) {
		return 0;  // avoid exception caused by division by zero
	}
	p = 1048576 - adc_P;
	p = (((p<<31) - var2)*3125) / var1;
	var1 = (((int64_t)p9) * (p>>13) * (p>>13)) >> 25;
	var2 = (((int64_t)p8) * p) >> 19;

	p = ((p + var1 + var2) >> 8) + (((int64_t)p7)<<4);
	return (int32_t)p/256;
}
 uint8_t BME280_GetAll(float *temperature, int32_t *pressure, float *huminidity){
		int64_t var1, var2, p;
		// Must be done first to get the t_fine variable set up
		*temperature = BME280_GetTemperature();

		if(*temperature == -99)
		  return -1;

		int32_t adc_P = Read24bit(BME280_PRESSUREDATA);
		adc_P >>= 4;

		var1 = ((int64_t)t_fine) - 128000;
		var2 = var1 * var1 * (int64_t)p6;
		var2 = var2 + ((var1*(int64_t)p5)<<17);
		var2 = var2 + (((int64_t)p4)<<35);
		var1 = ((var1 * var1 * (int64_t)p3)>>8) +
				((var1 * (int64_t)p2)<<12);
		var1 = (((((int64_t)1)<<47)+var1))*((int64_t)p1)>>33;

		if (var1 == 0) {
			return 0;  // avoid exception caused by division by zero
		}
		p = 1048576 - adc_P;
		p = (((p<<31) - var2)*3125) / var1;
		var1 = (((int64_t)p9) * (p>>13) * (p>>13)) >> 25;
		var2 = (((int64_t)p8) * p) >> 19;

		p = ((p + var1 + var2) >> 8) + (((int64_t)p7)<<4);
		*pressure = (int32_t)p/256;

		// Pressure section
		int32_t adc_H = Read16bit(BME280_HUMIDDATA);
		if (adc_H == 0x8000) // value in case humidity measurement was disabled
			return -1; //error

		int32_t v_x1_u32r;

		v_x1_u32r = (t_fine - ((int32_t)76800));

		v_x1_u32r = (((((adc_H << 14) - (((int32_t)h4) << 20) -
					  (((int32_t)h5) * v_x1_u32r)) + ((int32_t)16384)) >> 15) *
				   (((((((v_x1_u32r * ((int32_t)h6)) >> 10) *
						(((v_x1_u32r * ((int32_t)h3)) >> 11) + ((int32_t)32768))) >> 10) +
					  ((int32_t)2097152)) * ((int32_t)h2) + 8192) >> 14));

		v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) *
								 ((int32_t)h1)) >> 4));

		v_x1_u32r = (v_x1_u32r < 0) ? 0 : v_x1_u32r;
		v_x1_u32r = (v_x1_u32r > 419430400) ? 419430400 : v_x1_u32r;
		float h = (v_x1_u32r>>12);
		*huminidity = h / 1024.0;

		return 0;
 }
 uint8_t BME280_GetPressure2(int32_t *pressure, float *temperature,float *press_out){
	 float temp,pres,mnpm;
	 mnpm = altitude;
	 temp = *temperature;
	 pres = (float)*pressure/100;
	 float to_exp = exp(-(0.0289644f*9.81f*mnpm/(8.31f*(temp+273.15f))));
	 *press_out = pres / to_exp;
	 return 0;
 }


