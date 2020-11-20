/*
 * WAsys_adesto_AT45DB321E_flash_spi.h
 *
 *  (this)File created on: Nov 18, 2020
 *  Code created on: Sept 13, 2019 @ 1:47 AM
 *      Author: https://github.com/911992
 */

#ifndef WASYS_ADESTO_AT45DB321E_FLASH_SPI_H_
#define WASYS_ADESTO_AT45DB321E_FLASH_SPI_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef WAsys_adesto_AT45DB321E_flash_spi_DEBUG
#include "Wasys_logging.h" //https://github.com/911992/WAsys_lib_log
#define __WAsys_flash_DEBUG__STATUS__
#endif

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*
Possible values as 528 and 512. 512 for compatibility
*/
#define WAsys_FLASH_PAGE_SIZE 512

/**
 * the for loop that does nothing(as no op/sleep time) when needed
 */
#define WAsys_NO_OP_FOR_LOOP_COUNT 10177

/*
Total number of available pages
device: adesto AT45DB321E
*/
#define WAsys_FLASH_TOTAL_PAGE_COUNT 8192

/*
Device status, needs two bytes, so here two bytes for you
*/
typedef uint8_t WAsys_FLASH_DEVICE_STATUS_T[2];

/*
Reads slave status, and save it into arg_status
If arg_status, then teh result will be saved on local global var only(internal using)
*/
void WAsys_flash_get_status_bytes(WAsys_FLASH_DEVICE_STATUS_T arg_status);

/**
 * Flash state type
 */
typedef enum WAsys_FLASH_STATUS{
WAsys_FLASH_STATUS_READY = 0,
WAsys_FLASH_STATUS_BUSY = 1
}WAsys_FLASH_STATUS_T;

/**
 * Config type
 */
typedef struct{
	void(*chip_select)(uint8_t);
	int(*spi_read)(uint8_t*,size_t);
	int(*spi_write)(uint8_t*,size_t);
}WAsys_FLASH_CONFIG_T;

/*
Return WAsys_SPI_FLASH_STATUS_READY if flash slave is ready for read/write.
*/
WAsys_FLASH_STATUS_T WAsys_flash_is_ready(void);

/*
Initialize the lib/ctx
*/
void WAsys_flash_init(WAsys_FLASH_CONFIG_T arg_config_t);

/*
Returns WAsys_FLASH_TOTAL_PAGE_COUNT
*/
uint16_t WAsys_flash_total_page_count(void);

/*
Returns macro WAsys_FLASH_PAGE_SIZE (no surprise)
*/
uint16_t WAsys_flash_page_size(void);

/*
Returns the total number of available bytes on target SPI flash memory
NOTE: each page at physical memory contains 528 bytes, but this function calcule/assume each page for 512(WAsys_FLASH_PAGE_SIZE)
Returns 8192(WAsys_FLASH_TOTAL_PAGE_COUNT) x 512(WAsys_FLASH_PAGE_SIZE) (4194304)
NOTE: only for adesto AT45DB321E
*/
uint32_t WAsys_flash_memory_array_size_byte(void);

/*
Converst any-length integer value from LE to BE
Does not nothing if the device/host is BE itself
NOTE: no bit op, only byte shifting
NOte: bit shifting is not required for adesto AT45DB321E, but byte shifting
*/
void WAsys_BE_to_LE(uint8_t* arg_buff,size_t arg_len);


/*
returns 0 for success
Reads from arg_page_offset in flash for arg_page_len sectors
e.g. arg_page_offset := 2 , arg_page_len:=4
Reads 4 sectors (4 * 512) from second sector (1024), put the result on arg_to_buff
This function is blocking
*/
int WAsys_flash_read(uint16_t arg_page_offset, uint16_t arg_page_len,uint8_t* arg_to_buff);

/*
returns 0 for success
Reads from arg_from_buffer index (whole memory array) for arg_byte_len bytes, and save it to arg_to_buffer
e.g. arg_from_offset := 1939 , arg_byte_len := 718
Reads from arg_to_buff fro 718 bytes, and write them to main memory array from offset 1939 to 1939 + 718
NOTE: extra 16 bytes for each page will be ignorred!
This function is blocking
*/
int WAsys_flash_read_bytes(uint32_t arg_from_offset, uint16_t arg_byte_len,uint8_t* arg_to_buff);

/*
returns 0 for success
Writes from arg_from_buff to arg_to_sector for arg_page_len secors
e.g. arg_to_sector := 4 , arg_page_len := 2
Wrtites for 2 pages (1024 bytes) from arg_from_buff to flash at page 4 (2048) to 2048 + 1024
NOTE: writing to main-array will change status of buffer1(second buffer)
This function is blocking, and garauntees the target slave/flash complete the write op successfully
*/
int WAsys_flash_write(uint16_t arg_to_sector,uint16_t arg_page_len, uint8_t* arg_from_buff);

/*
returns 0 for success
Writes arg_from_buff from index arg_memory_offset for arg_write_len length
e.g. arg_memory_offset := 1948 arg_write_len := 911 arg_from_buff ={...}
Writes 911 bytes of arg_from_buff from index 1948 in whole memory array
NOTE: extra 16 bytes for each page will be ignorred!
NOTE: writing to main-array will change status of buffer1(second buffer)
This function is blocking, and garauntees the target slave/flash complete the write op successfull
*/
int WAsys_flash_write_bytes(uint32_t arg_memory_offset,uint16_t arg_write_len, uint8_t* arg_from_buff);

/*
Available buffer/sram on target device
*/
typedef enum WAsys_FLASH_BUFFER_ID{
	WAsys_FLASH_BUFFER_ID_BUFFER0 = 0,
	WAsys_FLASH_BUFFER_ID_BUFFER1 = 1
}WAsys_FLASH_BUFFER_ID_T;

/*
Possible values as 528 and 512. 512 for compatibility
*/
#define WAsys_FLASH_BUFFER_SIZE WAsys_FLASH_PAGE_SIZE

/*
returns 0 for success
Writes arg_from_buff from index arg_buff_offset for arg_write_len length to target arg_buff_id
e.g. arg_memory_offset := 256 arg_write_len := 256 arg_from_buff ={...}
Writes 256 bytes of arg_from_buff from index 256 in whole memory array
NOTE: buffer length is limited, so writing op should not exceed buffer length (arg_buff_offset + arg_write_len) < WAsys_FLASH_BUFFER_SIZE
NOTE: extra 16 bytes for each page will be ignorred!
NOTE: exceeded values are not checked and validated by this function, mind the buffer write overlap!
This function is blocking, and garauntees the target slave/flash complete the write op successfully
*/
int WAsys_flash_buffer_write_bytes(WAsys_FLASH_BUFFER_ID_T arg_buff_id,uint16_t arg_buff_offset, uint16_t arg_write_len, uint8_t* arg_from_buff);

/*
returns 0 for success
Reads target buffer arg_buff_id, from arg_from_buffer index for arg_byte_len bytes, and save it to arg_to_buffer
e.g. arg_from_offset := 128 , arg_byte_len := 64
Reads from arg_to_buff for 18 bytes, and write them to main memory array from offset 128 to 128 + 64
NOTE: buffer length is limited, so writing op should not exceed buffer length (arg_buff_offset + arg_write_len) < WAsys_FLASH_BUFFER_SIZE
NOTE: exceeded values are not checked and validated by this function, mind the buffer read overlap!
NOTE: extra 16 bytes for each page will be ignorred!
This function is blocking
*/
int WAsys_flash_buffer_read_bytes(WAsys_FLASH_BUFFER_ID_T arg_buff_id,uint16_t arg_from_offset, uint16_t arg_byte_len,uint8_t* arg_to_buff);


/*
returns 0 for success
Loads the target arg_main_array_page page into the target arg_buff_id
Use WAsys_flash_buffer_read_bytes for reading loaded data from buffer, this function doesn't response any data
This function is used (internally) when changing(program) a page incompleted, which is started from zero (blame target auto page rewrite, section 25 in doc)
*/
int WAsys_flash_buffer_fill_from_main_array(WAsys_FLASH_BUFFER_ID_T arg_buff_id,uint16_t arg_main_array_page);

/*
Writes/flush content of arg_buff_id at page arg_main_array_page in memory array
Use WAsys_flash_buffer_read_bytes for reading loaded data from buffer, this function doesn't response any data
This function is used (internally) when changing(program) a page incompleted, which is started from zero (blame target auto page rewrite, section 25 in doc)
*/
int WAsys_flash_buffer_write_to_main_array(WAsys_FLASH_BUFFER_ID_T arg_buff_id,uint16_t arg_main_array_page);

/*
returns 0 for success
Erease main memory page from arg_page_offset for arg_page_len pages
*/
int WAsys_flash_erase(uint16_t arg_page_offset,uint16_t arg_page_len);

#ifdef __cplusplus
}
#endif

#endif /* WASYS_ADESTO_AT45DB321E_FLASH_SPI_H_ */
