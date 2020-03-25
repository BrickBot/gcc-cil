OUTPUT_FORMAT("coff-h8300")
OUTPUT_ARCH(h8300)
/* OUTPUT_FORMAT(srec) */

ENTRY(_main)

MEMORY {
 
  rom (r!wxai)	:	ORIGIN = 0x0000, LENGTH = 0x4000
  /* all rom functions lay here, note the gap between rom and ram (there's nothing) */
  
  ram1          :	ORIGIN = 0x8000, LENGTH = 0x6f30
  /* first part of memory for SOSSE code and data */
  
  lcd		:	ORIGIN = 0xef30, LENGTH = 0x0020
  /* lcd data mappings */
  
  ram2 (wxai)	:	ORIGIN = 0xef50, LENGTH = 0x00b0
  /* second part of memory for SOSSE code and data */
  
  actuators	:	ORIGIN = 0xf000, LENGTH = 0x0010
  /* memory mappings of the output controller */
  
  ram3 (wxai)	:	ORIGIN = 0xf010, LENGTH = 0x0b70
  /* third part of memory for SOSSE code and data */
  
  reserved	:	ORIGIN = 0xfb80, LENGTH = 0x0200
  /* reserved for internal use of RCX */
      
  ocram	        :	ORIGIN = 0xfd80, LENGTH = 0x0200
  /* on-chip ram */
      
  ram4 (wxai)	:	ORIGIN = 0xff80, LENGTH = 0x0008
  /* fourth part of memory for SOSSE code and data */
      
  ocreg	: 	ORIGIN = 0xff88, LENGTH = 0x0078
  /* on chip registers */
    
}

SECTIONS {
  .text : {
    *(.init)
    *(.text)
    *(.rodata)
    ___text = . ;
  } > ram1
	 
  .data : {
    ___data = . ;
    *(.data)
  } > ram1
	
  .bss : {
    ___bss     = . ;
    *(.bss)
    _memory_start = . ;
  } > ram1
}

INCLUDE H8_3297.lds
INCLUDE Rcx.lds
