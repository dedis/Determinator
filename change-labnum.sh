#!/bin/sh

if [ $# -lt "2" ]; then
    echo "Usage: $0 src-lab# dst-lab#"
    exit 1
fi

src=$1
dst=$2

replace() {
    grep "$1" * -rl --exclude $0 | while read f
    do
	sed -i -e "s/$1/$2/g" $f
    done
}

replace "LAB >= $src" "LAB >= $dst"
replace "SOL >= $src" "SOL >= $dst"
replace "LAB $src" "LAB $dst"
replace "LAB$src" "LAB$dst"
