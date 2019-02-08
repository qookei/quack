#!/bin/bash

if ! type dialog > /dev/null; then
	exit
fi

echo -n "1" > build_opts
echo -n "1 3" > log_opts

read -r -d '' main_cmd << EOM
dialog --title "Kernel configuration" --menu "Select category(press Cancel when you're done):" 15 45 5 \
1 "Build options" \
2 "Log output levels" 2> /tmp/dialog_out
EOM

while eval "$main_cmd"
do 
	out=$(cat /tmp/dialog_out)
	
	if [ $out = "1" ]
	then
		dialog --title "Kernel configuration" --checklist "Select build options:" 15 45 5 \
1 "Debugging symbols" on 2> build_opts
	elif [ $out = "2" ]
	then
		dialog --title "Kernel configuration" --checklist "Select log levels to enable:" 15 45 5 \
1 "Info" on \
2 "Warnings" off \
3 "Errors" on \
4 "Debug messages" off 2> log_opts
	fi
done

echo "Generating config.mk"
python opts2conf.py > ../config.mk
rm *_opts
