#!/bin/bash

set -e

files=
outname=

flags="-g -Werror=declaration-after-statement -Werror=switch-enum -Werror=switch-default -Wall -Wextra -pedantic -std=c99"
incl=
libs=

if [[ -z $1 ]]; then
    echo "provide build option"
    exit 1
fi

case $1 in
    vm)
        files=vm.c
        outname=vm
        ;;
    assembler)
        files="assembler/*.c lib/os.c lib/mem.c lib/path.c lib/sstring.c"
        outname=asm
        ;;
    preprocessor)
        files="preprocessor/main.c lib/os.c lib/mem.c lib/path.c lib/sstring.c"
        outname=ppc
        ;;
    disassembler)
        files="disassembler/main.c"
        outname=diasm
        ;;
    all)
        ./build.sh vm $2 &
        ./build.sh assembler $2 &
        ./build.sh preprocessor $2 &
        ./build.sh disassembler $2 &
        wait
        exit 0
        ;;
    *)
        echo "unknown build option $1"
        exit 1
        ;;
esac

if [[ $2 = "prod" ]]; then
    flags=${flags/-g/-O2}
fi

gcc $flags -o $outname $files $incl $libs
