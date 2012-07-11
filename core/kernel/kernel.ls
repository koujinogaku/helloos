OUTPUT_FORMAT("binary");
IPL_KERNEL = 0x100000;

SECTIONS
{
  . = IPL_KERNEL;
  .text :	{*(.text)}
  .rodata :	{*(.rodata*)}
  .data :	{*(.data) *(.bss) *(.comment)}
}
