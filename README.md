# adesto-AT45DB321E
A simple lib to make `AT45DB321E` flash chip working

ANSI-C compatible

For debugging mode, you need [WAsys_lib_log](https://github.com/911992/WAsys_lib_log) repository.

Compiled by `WAsys_adesto_AT45DB321E_flash_spi_DEBUG` def defines, or could be explicitly defined in header file.

## Usage
* Init(define) an instance `WAsys_FLASH_CONFIG_T` type.
    * implement CS(chip select), io read/write functions
* Init the lib by pass the above to `WAsys_flash_init` func