CONFIG_BLK_DEV_IDE=1
AR=i386-elf-ar
LD=i386-elf-ld
CC=i386-elf-gcc
LANG=C
CFLAGS="-I../../include -D__KERNEL__ -DCONFIG_BLK_DEV_IDE -DCONFIG_BLK_DEV_IDECD -DDEBUG -fcall-used-ebx -fno-builtin -nostdlib -static"

export CONFIG_BLK_DEV_IDE
export AR
export LD
export CC
export LANG
export CFLAGS
