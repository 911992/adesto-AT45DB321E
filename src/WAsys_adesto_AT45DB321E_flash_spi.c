/*
 * WAsys_adesto_AT45DB321E_flash_spi.c
 *
 *  Created on: Nov 18, 2020
 *  Code created on: Sept 13, 2019 @ 1:47 AM
 *      Author: https://github.com/911992
 */

/*
///////////////////////////////////______
NOTE NOTE NOTE: device: adesto AT45DB321E
NOTE: functions are not thread-safe
///////////////////////////////////^^^^^^
*/
#include "WAsys_adesto_AT45DB321E_flash_spi.h"


#ifdef __cplusplus
extern "C" {
#endif

#define _WAsys_LSB_FOR_MSB 1 //must be 1
#define _WAsys_SPI_HOST_IS_MSB 0//should be 0 probably, unless the host doesn't perform the MSB itself

WAsys_FLASH_CONFIG_T G_config_file;

/**
 * Prepare the way chip talks addressing
 */
uint32_t _WAsys_LSB_MSB(uint32_t arg_val, size_t arg_len) {
	uint32_t _res = 0;
	uint32_t _one = 1;
	//uint8_t _len = (sizeof(uint16_t) * 8);
	uint8_t _len = arg_len*8;
	//uint8_t _len = 16;
	for (int _a = 0; _a < _len; _a++) {
		if ((arg_val & (_one << _a)) > 0) {
			_res = _res | (_one << (_len - _a - 1));
		}
	}
	return _res;
}

#define _WAsys_UINT8_MSB(__arg) _WAsys_LSB_MSB(__arg,sizeof(uint8_t))
#define _WAsys_UINT16_MSB(__arg) _WAsys_LSB_MSB(__arg,sizeof(uint16_t))
#define _WAsys_UINT32_MSB(__arg) _WAsys_LSB_MSB(__arg,sizeof(uint32_t))

//#define __WAsys_flash_DEBUG__
#ifdef __WAsys_flash_DEBUG__
	#define __WAsys_flash_DEBUG__STATUS__
#endif

/*delaying*/
#define WAsys_DELAY for(int __a=0;__a<WAsys_NO_OP_FOR_LOOP_COUNT;__a++){}

/*
Pulldown the target pin (PD2) for selecting chip SS/CS
*/
void _WAsys_flash_enable_CS(uint8_t arg_enable){
	G_config_file.chip_select(arg_enable);
}
#define _CS_ON _WAsys_flash_enable_CS(1);
#define _CS_OFF _WAsys_flash_enable_CS(0);
/*default timeout*/
#define _def_to 1024

static int _WAsys_read(uint8_t* arg_buff,size_t arg_len){
	return G_config_file.spi_read(arg_buff,arg_len);
}

static int _WAsys_write(uint8_t* arg_buff,size_t arg_len){
	return G_config_file.spi_write(arg_buff,arg_len);
}


WAsys_FLASH_DEVICE_STATUS_T _G_last_known_slave_status;

#define _WAsys_FLASH_STATUS_CMD 0xD7
void WAsys_flash_get_status_bytes(WAsys_FLASH_DEVICE_STATUS_T arg_status){
	uint8_t _cmd;
	//setting flag, only flag needed, no dummy stuffs
	_cmd = _WAsys_FLASH_STATUS_CMD;
	_CS_ON //enabling CS
	#ifdef __WAsys_flash_DEBUG__STATUS__
		log_serial("Flash get sttus before sending cmd...");
	#endif
	_WAsys_write(&_cmd,sizeof(_cmd));
	#ifdef __WAsys_flash_DEBUG__STATUS__
		log_serial("Flash get sttus after sending cmd.../before result");
	#endif
	//WAsys_DELAY;

	_WAsys_read(_G_last_known_slave_status,sizeof(WAsys_FLASH_STATUS_T));
	#ifdef __WAsys_flash_DEBUG__STATUS__
		log_serial("Flash get sttus after gettin result");
	#endif
	_CS_OFF //disabling CS
	if(arg_status!=NULL){
		arg_status[0]=_G_last_known_slave_status[0];
		arg_status[1]=_G_last_known_slave_status[1];
	}
	#ifdef __WAsys_flash_DEBUG__STATUS__
	sprintf(msg,"STATUS 0:%x 1:%x",_G_last_known_slave_status[0],_G_last_known_slave_status[1]);
		log_serial(msg);
	#endif
}

//#define _WAsys_CONTANT_READY_STATUS
WAsys_FLASH_STATUS_T WAsys_flash_is_ready(void){
	#ifdef _WAsys_CONTANT_READY_STATUS
		return  WAsys_FLASH_STATUS_READY;
	#else
	WAsys_flash_get_status_bytes(NULL);
	WAsys_FLASH_STATUS_T _res;
	if((_G_last_known_slave_status[0]&0x80)){
		_res=WAsys_FLASH_STATUS_READY;
	}else{
		_res=WAsys_FLASH_STATUS_BUSY;
	}
	#ifdef __WAsys_flash_DEBUG__STATUS__
	sprintf(msg,"STATUS__0 0:%x 1:%x",_G_last_known_slave_status[0],_G_last_known_slave_status[1]);
		log_serial(msg);
	#endif
	return _res;
	#endif
}

/*
Wait till device get ready (not too long)
*/
#define _WAIT_BUSY_ while(WAsys_flash_is_ready()){\
	/*for(int a=0;a<1024;a++){}*/\
	WAsys_DELAY;\
}



void WAsys_flash_init(WAsys_FLASH_CONFIG_T arg_config_t){
	//setting working config file
	G_config_file=arg_config_t;

	//Setting page size to 512
	uint8_t _cmd[4];
	_cmd[0]=0x3D;
	_cmd[1]=0x2A;
	_cmd[2]=0x80;
	_cmd[3]=0xA6;
	_WAIT_BUSY_
	_CS_ON //enabling CS
	_WAsys_write(_cmd,sizeof(_cmd));
	_CS_OFF //disabling CS
}

uint16_t WAsys_flash_total_page_count(void){
	return WAsys_FLASH_TOTAL_PAGE_COUNT;
}

uint16_t WAsys_flash_page_size(void){
	return WAsys_FLASH_PAGE_SIZE;
}

uint32_t WAsys_flash_memory_array_size_byte(void){
	return WAsys_FLASH_PAGE_SIZE * WAsys_FLASH_TOTAL_PAGE_COUNT;
}

//#define _WAsys_NO_ENDIANESS
void WAsys_BE_to_LE(uint8_t* arg_buff,size_t arg_len){
	#ifdef _WAsys_NO_ENDIANESS
		return;
	#else

	static uint32_t _i=1;
	static char* _ia=(char*)&_i;
	if(_ia[0]==0){/*host is BE itself*/
		return;
	}
	uint8_t _tmp;
	for(size_t a=0,b=arg_len-1;a<(arg_len/2);a++,b--){
		_tmp=arg_buff[b];
		arg_buff[b]=arg_buff[a];
		arg_buff[a]=_tmp;
	}
	#endif
}

#define _BE_int16(_arg_x_) WAsys_BE_to_LE(((uint8_t*)&_arg_x_),sizeof(uint16_t))
#define _BE_int32(_arg_x_) WAsys_BE_to_LE(((uint8_t*)&_arg_x_),sizeof(uint32_t))
#define _BE_int64(_arg_x_) WAsys_BE_to_LE(((uint8_t*)&_arg_x_),sizeof(uint64_t))

/*
Common type for requesting
*/
typedef struct{
	uint8_t cmd;
	uint16_t page_address;
	uint16_t byte_address;
	/*Not always 4, somtimes 0, but the maximum of 4*/
	uint8_t dummy[4];
}_WAsys_CMD_ADDRESS_T;

#define _WAsys_FLASH_PAGE_ADDRESS_MASK 0x1FFF
//#define _WAsys_FLASH_PAGE_ADDRESS_SHIFT 9 //when page is 528
//#define _WAsys_FLASH_PAGE_ADDRESS_SHIFT 10 //page 512
#define _WAsys_FLASH_PAGE_ADDRESS_SHIFT 7 //page 512
//#define _WAsys_FLASH_PAGE_ADDRESS_SHIFT 8 //page 512
//#define _WAsys_FLASH_BYTE_ADDRESS_MASK 0x3FF //when page is 528
#define _WAsys_FLASH_BYTE_ADDRESS_MASK 0x1FF //page 512
//#define _WAsys_FLASH_BYTE_ADDRESS_SHIFT 16 //page 512
//#define _WAsys_FLASH_BYTE_ADDRESS_SHIFT 24 //page 512
#define _WAsys_FLASH_BYTE_ADDRESS_SHIFT 23 //page 512

//#define _WAsys_BAD_ASS_LOGGGG



//#define _WAsys_LSB_FOR_MSB 0 ?/ MOVED TO TOP!
////#define _WAsys_NO_LE_MSB


uint32_t _WAsys_preapre_cmd_address(_WAsys_CMD_ADDRESS_T* arg_input){
	uint32_t _res;
	#ifdef _WAsys_NO_LE_MSB

	#else


	if(/******************************************************* && */_WAsys_LSB_FOR_MSB){
	_res = arg_input->cmd;
	//page address
	uint16_t _tmp = (arg_input->page_address & _WAsys_FLASH_PAGE_ADDRESS_MASK);
	//BE to LE if required
	_BE_int16(_tmp);
	//_tmp=_WAsys_LSB_MSB(_tmp);
	//shifting the BE page address (A9 -> A21) 512 page size
	uint32_t _tmp_damn_dev=(uint32_t)_tmp;
	_tmp_damn_dev = (_tmp_damn_dev << _WAsys_FLASH_PAGE_ADDRESS_SHIFT);
	//_res = _res | (_tmp << _WAsys_FLASH_PAGE_ADDRESS_SHIFT);
	_res = _res | _tmp_damn_dev;

	//byte address
	uint16_t _tmp1 = (arg_input->byte_address & _WAsys_FLASH_BYTE_ADDRESS_MASK);
	_BE_int16(_tmp1);
	//_tmp1=_WAsys_LSB_MSB(_tmp1);
	_tmp_damn_dev = (uint32_t)_tmp1;
	_tmp_damn_dev = (_tmp_damn_dev << _WAsys_FLASH_BYTE_ADDRESS_SHIFT);

	//shifting the BE byte address (A0 -> A8) 512 page size
	//_res = _res | (_tmp << _WAsys_FLASH_BYTE_ADDRESS_SHIFT);
	_res = _res | _tmp_damn_dev;
	#ifdef _WAsys_BAD_ASS_LOGGGG
	sprintf(msg,"CMD_PRE address:%x address(BE):%x byte_ADDR(BE):%x(%x), CMD:%x",arg_input->page_address,_tmp,arg_input->byte_address,_tmp1,_res);
	log_serial(msg);
	#endif
	}else{
		#ifdef _WAsys_BAD_ASS_LOGGGG
		log_serial("CMD/ADD MSB ver");
		sprintf(msg,"sizes: %d , %d , %d ",sizeof(arg_input->cmd),sizeof(arg_input->page_address),sizeof(arg_input->byte_address));
		log_serial(msg);
		#endif
		uint8_t* ptr=(uint8_t*)&_res;
		//ptr[0]=arg_input->cmd;
		ptr[0]=_WAsys_LSB_MSB(arg_input->cmd,sizeof(arg_input->cmd));
		//ptr[0]=arg_input->cmd;
//		_res = _WAsys_LSB_MSB(arg_input->cmd,sizeof(arg_input->cmd));
		uint16_t _page_addr=_WAsys_LSB_MSB(arg_input->page_address,sizeof(arg_input->page_address));
		uint16_t _byte_addr=_WAsys_LSB_MSB(arg_input->byte_address,sizeof(arg_input->byte_address));
		_res = _res | (_byte_addr<<16);
		_res = _res | (_page_addr<<7);
		if(_WAsys_SPI_HOST_IS_MSB){
			for(size_t a=0;a<sizeof(_res);a++){
				ptr[a]=_WAsys_UINT8_MSB(ptr[a]);
			}
		}
		#ifdef _WAsys_BAD_ASS_LOGGGG
		sprintf(msg,"CMD_PRE address(MSB):%x(%x) byte_ADDR(MSB):%x(%x), CMD:%x",arg_input->page_address,_page_addr,arg_input->byte_address,_byte_addr,_res);
		log_serial(msg);
		#endif
	}

	#endif
	return _res;//Ready for transfer! Happy MOSI time! :D
}

uint16_t _WAsys_MIN(uint16_t arg_a ,uint16_t arg_b){
	if(arg_a<arg_b){
		return arg_a;
	}
	return arg_b;
}

/*=========================generic read/write buff/main-array func=========================*/

/*typical page programming typical time (in ms) needed by slave to complete WRITE op*/
#define _WAsys_FLASH_TYPICAL_TP_PAGE_PROGRAMMING_TIME 5


/*Mode 0 is using flash internal sram, while mode two is like fetch, local change and write mode*/
#define _WAsys_PARTIAL_WRITE_MODE_0 0
#define _WAsys_PARTIAL_WRITE_MODE_1 1
#define _WAsys_PARTIAL_WRITE_MODE _WAsys_PARTIAL_WRITE_MODE_1
#if (_WAsys_PARTIAL_WRITE_MODE == _WAsys_PARTIAL_WRITE_MODE_1)
uint8_t _WAsys_tmp_array[WAsys_FLASH_PAGE_SIZE];
#endif

/*Generic read/write function*/
//int _WAsys_flash_generic_read_write_bytes(uint8_t arg_cmd,uint8_t arg_dummy_count,uint8_t arg_is_read,uint8_t arg_is_buffer,uint16_t arg_from_offset, uint16_t arg_byte_len,uint8_t* arg_from_to_buff)
int _WAsys_flash_generic_read_write_bytes(uint8_t arg_cmd,uint8_t arg_dummy_count,uint8_t arg_is_read,uint8_t arg_is_buffer,uint32_t arg_from_offset, uint16_t arg_byte_len,uint8_t* arg_from_to_buff){
//	//how many off the page
//	uint16_t _extra_offset_byte = arg_from_offset % WAsys_FLASH_PAGE_SIZE;
//	//target on-page byte index
//	uint16_t _target_byte = arg_from_offset - _extra_offset_byte;
//	//target page address
//	uint16_t _target_page = _target_byte / WAsys_FLASH_PAGE_SIZE;

	if(arg_byte_len==0){
		return 0xffffffff;
	}

	#ifdef __WAsys_flash_DEBUG__
	log_serial("_WAsys_flash_generic_read_write_bytes");
	zeromsg();sprintf(msg,"cmd:%x , dummy:%d , from: %d , len:%d , read-mode:%d",arg_cmd,arg_dummy_count,arg_from_offset,arg_byte_len,arg_is_read);log_serial(msg);
	//HAL_Delay(1024);
	#endif

	/*target byte start*/
	uint16_t _target_byte = arg_from_offset % WAsys_FLASH_PAGE_SIZE;
	#ifdef __WAsys_flash_DEBUG__
	zeromsg();sprintf(msg,"target_byte:%d",_target_byte);log_serial(msg);
	//HAL_Delay(1024);
	#endif
	/*target page*/
	uint16_t _target_page = (arg_from_offset-_target_byte) / WAsys_FLASH_PAGE_SIZE;

	_WAsys_CMD_ADDRESS_T _cmd_st;
	_cmd_st.cmd=arg_cmd;
	_cmd_st.page_address=_target_page;
	_cmd_st.byte_address=_target_byte;
	//_cmd_st.byte_address=_extra_offset_byte;

	//get preapred command + address
	uint32_t _cmd_ready = _WAsys_preapre_cmd_address(&_cmd_st);
	size_t _cmd_len=sizeof(_cmd_ready)+arg_dummy_count;

	//preparing target command
	uint8_t _cmd[sizeof(_cmd_ready)+arg_dummy_count];
	//memset(_cmd,0,sizeof(_cmd));//not really required
	memset(_cmd,0,_cmd_len);//not really required
	memcpy(_cmd,&_cmd_ready,sizeof(_cmd_ready));

	int _res;

	/*
	Continues read has some issues over page cross over, so partial/ page-page read is appreciated, just like write op
	*/
//	if(arg_is_read){//read op
//		_WAIT_BUSY_
//		_CS_ON //enabling CS
//		//sending command
//		_res = _WAsys_write(_cmd,sizeof(_cmd));
//		WAsys_DELAY;
//		//proceed for recieving
//		_res = _res | _WAsys_read(arg_from_to_buff,arg_byte_len);
//		_CS_OFF
//	}
//	else
	{//write op
		uint32_t _proceed_bytes=0;
		uint16_t _to_read_write;
		uint16_t page_i=0;
		#ifdef __WAsys_flash_DEBUG__
		log_serial("before write while");
		#endif
		while(_proceed_bytes < arg_byte_len){
			/*let the first block be partial write(non-zero), to keep rest blocks completed (for better performance) see section 25 in doc*/
			if(_proceed_bytes==0){
	//				/*target offset*/
	//				uint32_t _target_offset = arg_from_offset + arg_byte_len ;
	//				/*bytes supposed to be left at the end*/
	//				uint16_t _end_left = _target_offset % WAsys_FLASH_PAGE_SIZE;
	//				uint16_t _begin_left= (arg_byte_len - _end_left) % WAsys_FLASH_PAGE_SIZE;
	//				_to_read_write = _begin_left;
				if(arg_byte_len<WAsys_FLASH_PAGE_SIZE){
					_to_read_write=arg_byte_len;
				}else{
					_to_read_write= WAsys_FLASH_PAGE_SIZE - (arg_from_offset % WAsys_FLASH_PAGE_SIZE);
					if(_to_read_write==0){
						_to_read_write=WAsys_FLASH_PAGE_SIZE;
					}
				}
			}else{
				_to_read_write=_WAsys_MIN(WAsys_FLASH_PAGE_SIZE,(arg_byte_len-_proceed_bytes));
				_cmd_st.page_address=_target_page+page_i;
				//to trigger page-rease / buffer-out op at target slave (see section 6.6 and 25 of doc)
				_cmd_st.byte_address=0;
				#ifdef __WAsys_flash_DEBUG__
				zeromsg();sprintf(msg,"Extend page(%d) to : %d",_target_page,(_target_page+page_i));log_serial(msg);
				#endif

				uint32_t _cmd_ready = _WAsys_preapre_cmd_address(&_cmd_st);

				memset(_cmd,0,sizeof(_cmd));
				memcpy(_cmd,&_cmd_ready,sizeof(_cmd_ready));
			}
			#ifdef __WAsys_flash_DEBUG__
			zeromsg();sprintf(msg,"_to_read_write: %d , _proceed: %d , offset:%d ",_to_read_write,_proceed_bytes,arg_from_offset);log_serial(msg);
			#endif

			/*NO generic/global cmd send from here, due to partial write ops*/
//			_WAIT_BUSY_
//			_CS_ON //enabling CS
//
//			//sending command
//			_res = _WAsys_write(_cmd,sizeof(_cmd));
//			WAsys_DELAY;
//			#ifdef __WAsys_flash_DEBUG__
//			zeromsg();sprintf(msg,"send cmd res: %d",_res);log_serial(msg);
//			#endif
//
//			/*Due to write timeout*/
//			if(_res != 0){
//				_CS_OFF
//				#ifdef __WAsys_flash_DEBUG__
//				log_serial("Error, TO-0");
//				#endif
//				continue;
//			}else if(_res!=0){
//				_CS_OFF
//				break;
//			}
	//			#ifdef __WAsys_flash_DEBUG__
	//			zeromsg();
	//			log_serial("\nafter sending(writing) /// arr state");
	//			for(int x=0;x<_to_read_write;x++){
	//				sprintf(msg,"%2x ",(arg_from_to_buff+_proceed_bytes)[x]);
	//				log_serial_br(msg,0);
	//			}
	//			#endif
			if(arg_is_read){
				#ifdef __WAsys_flash_DEBUG__
				log_serial("Wait_busy for read op...");
				#endif
				_WAIT_BUSY_
				#ifdef __WAsys_flash_DEBUG__
				log_serial("CS_SELECT_ON");
				#endif
				_CS_ON //enabling CS

				//sending command
				//_res = _WAsys_write(_cmd,sizeof(_cmd));
				#ifdef __WAsys_flash_DEBUG__
				log_serial("BEFORE CMD SEND");
				#endif
				_res = _WAsys_write(_cmd,_cmd_len);
				WAsys_DELAY;
				#ifdef __WAsys_flash_DEBUG__
				zeromsg();sprintf(msg,"send cmd(read) res: %d",_res);log_serial(msg);
				#endif

				/*Due to write timeout*/
				if(_res != 0){
					_CS_OFF
					#ifdef __WAsys_flash_DEBUG__
					log_serial("Error, TO-0");
					#endif
					continue;
				}else if(_res!=0){
					_CS_OFF
					break;
				}
				_res = _WAsys_read((arg_from_to_buff+_proceed_bytes),_to_read_write);
				if(_res != 0){
					_CS_OFF
					#ifdef __WAsys_flash_DEBUG__
					log_serial("Error, TO-1");
					#endif
					continue;
				}else if(_res!=0){
					_CS_OFF
					break;
				}
			}else{
				#ifdef __WAsys_flash_DEBUG__
				zeromsg();
				sprintf(msg,"^^BEFORE write mode check byte_addr: %d , to_write:%d , is_buffer:%d",_cmd_st.byte_address,_to_read_write,arg_is_buffer);
				log_serial(msg);
				#endif

				/*When incompleted page write, started from zero. Auto Page Rewrite will be fooled!*/
				if(_cmd_st.byte_address==0 && _to_read_write < WAsys_FLASH_PAGE_SIZE && arg_is_buffer==0){

					/*==========================================*/
					/*PARTIAL MODE 0 (USING LOCAL CACHE)*/
					#if (_WAsys_PARTIAL_WRITE_MODE == _WAsys_PARTIAL_WRITE_MODE_1)

					/*Loading contents of target page into tmp array*/
					#ifdef __WAsys_flash_DEBUG__
						log_serial("Loading contents of target page into tmp array");
					#endif
					_res = WAsys_flash_read((_target_page+page_i),1,_WAsys_tmp_array);
					if(_res != 0){
						_CS_OFF
						#ifdef __WAsys_flash_DEBUG__
						log_serial("Error, TO-2");
						#endif
						continue;
					}else if(_res!=0){
						_CS_OFF
						break;
					}

					/*Changes content with new vals*/
					#ifdef __WAsys_flash_DEBUG__
						log_serial("Changes content with new vals");
						zeromsg();
					#endif
					for(int a=0 ; a< _to_read_write ; a++){
						#ifdef __WAsys_flash_DEBUG__
						sprintf(msg,"CHX %2x -> %2x",_WAsys_tmp_array[a],(arg_from_to_buff+_proceed_bytes)[a]);
						log_serial_br(msg,0);
						#endif
						_WAsys_tmp_array[a]=(arg_from_to_buff+_proceed_bytes)[a];
					}

					/*Write temp page at target page*/
					#ifdef __WAsys_flash_DEBUG__
						log_serial("Write temp page at target page");
					#endif
					_res = WAsys_flash_write((_target_page+page_i),1,_WAsys_tmp_array);
					if(_res != 0){
						_CS_OFF
						#ifdef __WAsys_flash_DEBUG__
						log_serial("Error, TO-2");
						#endif
						continue;
					}else if(_res!=0){
						_CS_OFF
						break;
					}

					/*==========================================*/
					/*PARTIAL MODE 0 (USING FLASH INTERNAL SRAM) , not working!*/
					#else
					/*Loading the page content into buffer1(secnd buffer)*/
					#ifdef __WAsys_flash_DEBUG__
						log_serial("Loading the page content into buffer1(secnd buffer)");
					#endif
					_res = WAsys_flash_buffer_fill_from_main_array(WAsys_FLASH_BUFFER_ID_BUFFER1,_target_page+page_i);
					if(_res != 0){
						_CS_OFF
						#ifdef __WAsys_flash_DEBUG__
						log_serial("Error, TO-2");
						#endif
						continue;
					}else if(_res!=0){
						_CS_OFF
						break;
					}

					/*Write to target buffer, and apply affected bytes from zero (partial)*/
					#ifdef __WAsys_flash_DEBUG__
						log_serial("Write to target buffer, and apply affected bytes from zero (partial)");
					#endif
					_res = WAsys_flash_buffer_write_bytes(WAsys_FLASH_BUFFER_ID_BUFFER1,_cmd_st.byte_address,WAsys_FLASH_PAGE_SIZE,(arg_from_to_buff+_proceed_bytes));
					if(_res != 0){
						_CS_OFF
						#ifdef __WAsys_flash_DEBUG__
						log_serial("Error, TO-3");
						#endif
						continue;
					}else if(_res!=0){
						_CS_OFF
						break;
					}
					/*FLush buffer to target page for good*/
					#ifdef __WAsys_flash_DEBUG__
						log_serial("FLush buffer to target page for good");
					#endif
					_res = WAsys_flash_buffer_write_to_main_array(WAsys_FLASH_BUFFER_ID_BUFFER1,_target_page+page_i);
					if(_res != 0){
						_CS_OFF
						#ifdef __WAsys_flash_DEBUG__
						log_serial("Error, TO-4");
						#endif
						continue;
					}else if(_res!=0){
						_CS_OFF
						break;
					}
					#endif

				}else{
					#ifdef __WAsys_flash_DEBUG__
						log_serial("Writing whole page complete!");
					#endif
					_WAIT_BUSY_
					#ifdef __WAsys_flash_DEBUG__
						log_serial("Write/tread , before cmd CS_ON");
					#endif
					_CS_ON //enabling CS

					//sending command
					//_res = _WAsys_write(_cmd,sizeof(_cmd));
					#ifdef __WAsys_flash_DEBUG__
						log_serial("Before SPI transfer/CMD...");
					#endif
					_res = _WAsys_write(_cmd,_cmd_len);
					//WAsys_DELAY;
					#ifdef __WAsys_flash_DEBUG__
					zeromsg();sprintf(msg,"send cmd(write) res: %d",_res);log_serial(msg);
					#endif

					/*Due to write timeout*/
					if(_res != 0){
						_CS_OFF
						#ifdef __WAsys_flash_DEBUG__
						log_serial("Error, TO-9");
						#endif
						continue;
					}else if(_res!=0){
						_CS_OFF
						break;
					}
					_res = _WAsys_write((arg_from_to_buff+_proceed_bytes),_to_read_write);
					if(_res != 0){
						_CS_OFF
						#ifdef __WAsys_flash_DEBUG__
						log_serial("Error, TO-5");
						#endif
						continue;
					}else if(_res!=0){
						_CS_OFF
						break;
					}
				}//else-ful-write
			}//else if write

			_CS_OFF
//			#ifdef __WAsys_flash_DEBUG__
//	//			zeromsg();
//	//			log_serial("\nAfter sending(writing) /// arr state");
//	//			for(int x=0;x<_to_read_write;x++){
//	//				sprintf(msg,"%2x ",(arg_from_to_buff+_proceed_bytes)[x]);
//	//				log_serial_br(msg,0);
//	//			}
//			zeromsg();sprintf(msg,"\nsend data res: %d",_res);log_serial(msg);
//			#endif
//			if(_res != 0){/*Due to write timeout*/
//				log_serial("Error, TO-1");
//				continue;
//			}else if(_res!=0){
//				break;
//			}
			if(!arg_is_read){
				//HAL_Delay(_WAsys_FLASH_TYPICAL_TP_PAGE_PROGRAMMING_TIME);
				WAsys_DELAY
			}
			_proceed_bytes = _proceed_bytes + _to_read_write;
			page_i = page_i + 1;
		}
	}
	_CS_OFF
	return _res;
}

/*=========================main memory read funcs=========================*/

/*generic buffer/main-array read*/
//int _WAsys_flash_generic_read_bytes(uint8_t arg_cmd,uint8_t arg_dummy_count,uint16_t arg_is_buffer,uint16_t arg_from_offset, uint16_t arg_byte_len,uint8_t* arg_to_buff)
int _WAsys_flash_generic_read_bytes(uint8_t arg_cmd,uint8_t arg_dummy_count,uint16_t arg_is_buffer,uint32_t arg_from_offset, uint16_t arg_byte_len,uint8_t* arg_to_buff){
	return _WAsys_flash_generic_read_write_bytes(arg_cmd,arg_dummy_count,1,arg_is_buffer,arg_from_offset,arg_byte_len,arg_to_buff);
}

#define _WAsys_FLASH_MEMORY_READ_CMD 0x1B
#define _WAsys_FLASH_MEMORY_READ_CMD_DUMMY 2 //Required dummy bytes(who cares?)
int WAsys_flash_read(uint16_t arg_page_offset, uint16_t arg_page_len,uint8_t* arg_to_buff){
//	_WAsys_CMD_ADDRESS_T _cmd_st;
//	_cmd_st.cmd=_WAsys_FLASH_MEMORY_READ_CMD;
//	_cmd_st.page_address=arg_page_offset;
//	_cmd_st.byte_address=0;
//
//	//get preapred command + address
//	uint32_t _cmd_ready = _WAsys_preapre_cmd_address(&_cmd_st);
//
//	//preparing target command
//	uint8_t _cmd[sizeof(_cmd_ready)+_WAsys_FLASH_MEMORY_READ_CMD_DUMMY];
//	memset(_cmd,0,sizeof(_cmd));//not really required
//	memcpy(_cmd,&_cmd_ready,sizeof(_cmd_ready));
//
//	int _res;
//
//	_WAIT_BUSY_
//	_CS_ON //enabling CS
//	//sending command
//	_res = _WAsys_write(_cmd,sizeof(_cmd));
//	WAsys_DELAY;
//	//proceed for recieving
//	_res = _res | _WAsys_read(arg_to_buff,(arg_page_len*WAsys_FLASH_PAGE_SIZE));
//	_CS_OFF //disabling CS
//
//	return _res;



	uint32_t _target_byte=arg_page_offset*WAsys_FLASH_PAGE_SIZE;
	uint16_t _len = (arg_page_len*WAsys_FLASH_PAGE_SIZE);

	#ifdef __WAsys_flash_DEBUG__
	sprintf(msg,"page read WAsys_flash_read target address:%d , target_len:%d",_target_byte,_len);
	log_serial(msg);
	#endif

	return WAsys_flash_read_bytes(_target_byte,_len,arg_to_buff);

}


int WAsys_flash_read_bytes(uint32_t arg_from_offset, uint16_t arg_byte_len,uint8_t* arg_to_buff){
	//return _WAsys_flash_generic_read_bytes(_WAsys_FLASH_MEMORY_READ_CMD,_WAsys_FLASH_MEMORY_READ_CMD_DUMMY,arg_from_offset,arg_byte_len,arg_to_buff);
	return _WAsys_flash_generic_read_bytes(_WAsys_FLASH_MEMORY_READ_CMD,_WAsys_FLASH_MEMORY_READ_CMD_DUMMY,0,arg_from_offset,arg_byte_len,arg_to_buff);
}

/*=========================main memory write funcs=========================*/

/*Generic main-array/buff write ops*/
int _WAsys_flash_generic_write_bytes(uint8_t arg_cmd,uint8_t arg_dummy_count,uint16_t arg_is_buffer,uint32_t arg_memory_offset,uint16_t arg_write_len, uint8_t* arg_from_buff){
	return _WAsys_flash_generic_read_write_bytes(arg_cmd,arg_dummy_count,0,arg_is_buffer,arg_memory_offset,arg_write_len,arg_from_buff);
}

#define _WAsys_FLASH_MEMORY_WRITE_CMD 0x59 //Read-Modify-Write mode, like EEPROM (see adesto doc, section 6.6), using buffer1(second buffer)
#define _WAsys_FLASH_MEMORY_WRITE_CMD_DUMMY 0 //Required dummy bytes(who cares?)
int WAsys_flash_write(uint16_t arg_to_sector,uint16_t arg_page_len, uint8_t* arg_from_buff){
	uint32_t _target_byte=arg_to_sector*WAsys_FLASH_PAGE_SIZE;
	uint16_t _len = (arg_page_len*WAsys_FLASH_PAGE_SIZE);
	#ifdef __WAsys_flash_DEBUG__
	zeromsg();sprintf(msg,"flash_write_page:%d , len:%d ",_target_byte,_len);log_serial(msg);
	#endif
	return WAsys_flash_write_bytes(_target_byte,_len,arg_from_buff);
}

int WAsys_flash_write_bytes(uint32_t arg_memory_offset,uint16_t arg_write_len, uint8_t* arg_from_buff){
	//return _WAsys_flash_generic_write_bytes(_WAsys_FLASH_MEMORY_WRITE_CMD,_WAsys_FLASH_MEMORY_WRITE_CMD_DUMMY,arg_memory_offset,arg_write_len,arg_from_buff);
	return _WAsys_flash_generic_write_bytes(_WAsys_FLASH_MEMORY_WRITE_CMD,_WAsys_FLASH_MEMORY_WRITE_CMD_DUMMY,0,arg_memory_offset,arg_write_len,arg_from_buff);
}

/*=========================Buffer read - write funcs=========================*/
#define _WAsys_FLASH_BUFFER0_READ_CMD 0xD4
#define _WAsys_FLASH_BUFFER0_READ_CMD_DUMMY 1
#define _WAsys_FLASH_BUFFER1_READ_CMD 0xD6
#define _WAsys_FLASH_BUFFER1_READ_CMD_DUMMY _WAsys_FLASH_BUFFER0_READ_CMD_DUMMY
int WAsys_flash_buffer_read_bytes(WAsys_FLASH_BUFFER_ID_T arg_buff_id,uint16_t arg_from_offset, uint16_t arg_byte_len,uint8_t* arg_to_buff){
	uint8_t _target_cmd, _target_dummy_len;
	if(arg_buff_id == WAsys_FLASH_BUFFER_ID_BUFFER0){
		_target_cmd=_WAsys_FLASH_BUFFER0_READ_CMD;
		_target_dummy_len=_WAsys_FLASH_BUFFER0_READ_CMD_DUMMY;
	}else{
		_target_cmd=_WAsys_FLASH_BUFFER1_READ_CMD;
		_target_dummy_len=_WAsys_FLASH_BUFFER1_READ_CMD_DUMMY;
	}
	//return _WAsys_flash_generic_read_bytes(_target_cmd,_target_dummy_len,arg_from_offset,arg_byte_len,arg_to_buff);
	return _WAsys_flash_generic_read_bytes(_target_cmd,_target_dummy_len,1,arg_from_offset,arg_byte_len,arg_to_buff);
}

#define _WAsys_FLASH_BUFFER0_WRITE_CMD 0x84
#define _WAsys_FLASH_BUFFER0_WRITE_CMD_DUMMY 0
#define _WAsys_FLASH_BUFFER1_WRITE_CMD 0x87
#define _WAsys_FLASH_BUFFER1_WRITE_CMD_DUMMY _WAsys_FLASH_BUFFER0_READ_CMD_DUMMY
int WAsys_flash_buffer_write_bytes(WAsys_FLASH_BUFFER_ID_T arg_buff_id,uint16_t arg_buff_offset, uint16_t arg_write_len, uint8_t* arg_from_buff){
	uint8_t _target_cmd, _target_dummy_len;
	if(arg_buff_id == WAsys_FLASH_BUFFER_ID_BUFFER0){
		_target_cmd=_WAsys_FLASH_BUFFER0_WRITE_CMD;
		_target_dummy_len=_WAsys_FLASH_BUFFER0_WRITE_CMD_DUMMY;
	}else{
		_target_cmd=_WAsys_FLASH_BUFFER1_WRITE_CMD;
		_target_dummy_len=_WAsys_FLASH_BUFFER1_WRITE_CMD_DUMMY;
	}
	//return _WAsys_flash_generic_write_bytes(_target_cmd,_target_dummy_len,arg_buff_offset,arg_write_len,arg_from_buff);
	return _WAsys_flash_generic_write_bytes(_target_cmd,_target_dummy_len,1,arg_buff_offset,arg_write_len,arg_from_buff);
}

int _WAsys_flash_buffer_fill_or_write_main_array(uint8_t arg_cmd,uint8_t arg_dummy,uint16_t arg_main_array_page){
	_WAsys_CMD_ADDRESS_T _cmd_st;
	_cmd_st.cmd=arg_cmd;
	_cmd_st.page_address=arg_main_array_page;
	_cmd_st.byte_address=0;
	//_cmd_st.byte_address=_extra_offset_byte;

	//get preapred command + address
	uint32_t _cmd_ready = _WAsys_preapre_cmd_address(&_cmd_st);

	//preparing target command
	size_t _cmd_len=sizeof(_cmd_ready)+arg_dummy;
	uint8_t _cmd[_cmd_len];
	memset(_cmd,0,sizeof(_cmd));//not really required
	memcpy(_cmd,&_cmd_ready,sizeof(_cmd_ready));

	_WAIT_BUSY_
	_CS_ON
	//int _res=_WAsys_write(_cmd,sizeof(_cmd));
	int _res=_WAsys_write(_cmd,_cmd_len);
	_CS_OFF
	_WAIT_BUSY_
	return _res;
}

#define _WAsys_FLASH_BUFFER0_FILL_FROM_MAIN_ARRAY 0x53
#define _WAsys_FLASH_BUFFER0_FILL_FROM_MAIN_ARRAY_DUMMY 0
#define _WAsys_FLASH_BUFFER1_FILL_FROM_MAIN_ARRAY 0x55
#define _WAsys_FLASH_BUFFER1_FILL_FROM_MAIN_ARRAY_DUMMY 0
int WAsys_flash_buffer_fill_from_main_array(WAsys_FLASH_BUFFER_ID_T arg_buff_id,uint16_t arg_main_array_page){
	uint8_t _cmd_val;
	uint8_t _dummy_len;
	if(arg_buff_id==WAsys_FLASH_BUFFER_ID_BUFFER0){
		_cmd_val = _WAsys_FLASH_BUFFER0_FILL_FROM_MAIN_ARRAY;
		_dummy_len = _WAsys_FLASH_BUFFER0_FILL_FROM_MAIN_ARRAY_DUMMY;
	}else{
		_cmd_val = _WAsys_FLASH_BUFFER1_FILL_FROM_MAIN_ARRAY;
		_dummy_len = _WAsys_FLASH_BUFFER1_FILL_FROM_MAIN_ARRAY_DUMMY;
	}
	return _WAsys_flash_buffer_fill_or_write_main_array(_cmd_val,_dummy_len,arg_main_array_page);
}

#define _WAsys_FLASH_BUFFER0_WRITE_TO_MAIN_ARRAY 0x83
#define _WAsys_FLASH_BUFFER0_WRITE_TO_MAIN_ARRAY_DUMMY 0
#define _WAsys_FLASH_BUFFER1_WRITE_TO_MAIN_ARRAY 0x86
#define _WAsys_FLASH_BUFFER1_WRITE_TO_MAIN_ARRAY_DUMMY 0
int WAsys_flash_buffer_write_to_main_array(WAsys_FLASH_BUFFER_ID_T arg_buff_id,uint16_t arg_main_array_page){
	uint8_t _cmd_val;
	uint8_t _dummy_len;
	if(arg_buff_id==WAsys_FLASH_BUFFER_ID_BUFFER0){
		_cmd_val = _WAsys_FLASH_BUFFER0_WRITE_TO_MAIN_ARRAY;
		_dummy_len = _WAsys_FLASH_BUFFER0_WRITE_TO_MAIN_ARRAY_DUMMY;
	}else{
		_cmd_val = _WAsys_FLASH_BUFFER1_WRITE_TO_MAIN_ARRAY;
		_dummy_len = _WAsys_FLASH_BUFFER1_WRITE_TO_MAIN_ARRAY_DUMMY;
	}
	return _WAsys_flash_buffer_fill_or_write_main_array(_cmd_val,_dummy_len,arg_main_array_page);
}

#define _WAsys_FLASH_PAGE_ERASE_CMD 0x81
#define _WAsys_FLASH_PAGE_ERASE_CMD_DUMMY 0
int WAsys_flash_erase(uint16_t arg_page_offset,uint16_t arg_page_len){
	int _res = 0;
	for(int a=0;a<arg_page_len;a++){
		_res |= _WAsys_flash_buffer_fill_or_write_main_array(_WAsys_FLASH_PAGE_ERASE_CMD,_WAsys_FLASH_PAGE_ERASE_CMD_DUMMY,arg_page_offset+a);
	}
	return _res;
}

#ifdef __cplusplus
}
#endif
