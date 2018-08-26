#!/bin/sh

echo -n "" > build_opts
echo -n "3" > log_opts
echo -n "1" > driv_opts

read -r -d '' main_cmd << EOM
dialog --title "Kernel configuration" --menu "Select category(press Cancel when you're done):" 15 45 5 \
1 "Build options" \
2 "Log output levels" \
3 "Drivers" 2> /tmp/dialog_out
EOM

while eval "$main_cmd"
do 
	out=$(cat /tmp/dialog_out)
	
	if [ $out = "1" ]
	then
		dialog --title "Kernel configuration" --checklist "Select build options:" 15 45 5 \
1 "Nice Panic" off \
2 "Don't display PCI devices" off \
3 "Debugging symbols" off \
4 "Disable PS2 Controller reset" off 2> build_opts
	elif [ $out = "2" ]
	then
		dialog --title "Kernel configuration" --checklist "Select log levels to enable:" 15 45 5 \
1 "Info" off \
2 "Warnings" off \
3 "Errors" on \
4 "Debug messages" off 2> log_opts
	elif [ $out = "3" ]
	then
		dialog --title "Kernel configuration" --checklist "Select drivers:" 15 45 5 \
1 "PS2 Mouse" on \
2 "PS2 Keyboard" on 2> driv_opts

	fi
done

echo "Generating config.mk"
python opts2conf.py > ../config.mk
rm *_opts
