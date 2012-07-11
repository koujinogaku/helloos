OUTPUT_FORMAT("binary");
IPL_START = 0x7c00;

SECTIONS
{
  . = IPL_START;
  .text	:	{*(.text)}
  .rodata :	{*(.rodata*)}
  .data	:	{*(.data)}
  .bss	:	{*(.bss)}
  .comment :    {*(.comment)}

  . = IPL_START + 510;
  .sign	:	{SHORT(0xAA55)}

}
