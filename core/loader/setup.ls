OUTPUT_FORMAT("binary");
IPL_SETUP = 0x1000;

SECTIONS
{
  . = IPL_SETUP;
  .text :	{*(.text)}
  .rodata :	{*(.rodata*)}
  .data :	{*(.data)}
  .bss  :	{*(.bss)}
  .comment :	{*(.comment)}
}
