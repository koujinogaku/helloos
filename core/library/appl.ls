OUTPUT_FORMAT("binary");
USR_START = 0x80000000;

SECTIONS
{
  . = USR_START;
  .text :	{*(.text)}
  .rodata :	{*(.rodata*)}
  .data :	{*(.data) *(.bss) *(.comment)}
}
