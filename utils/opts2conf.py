drivers_files = [
	"ps2m.cc",
	"ps2k.cc"	
]

drivers_flags = [
	"-DDRIVER_PS2M",
	"-DDRIVER_PS2K"
]

build_flags = [
	"-DPANIC_NICE",
	"-DPCI_DISABLE_LOG",
	"-g",
	"-DDISABLE_PS2_INIT"
]

log_levels = [
	"-DEARLY_LOG_INFO",
	"-DEARLY_LOG_WARN",
	"-DEARLY_LOG_ERR",
	"-DEARLY_LOG_DBG"
]

out_driver_flags = ""
out_driver_files = ""
out_build_flags = ""
out_build_flags_c = ""

with open("driv_opts") as f:
	opts = f.read().split(" ")
	for i in opts:
		if i.isdigit():
			out_driver_flags += drivers_flags[int(i) - 1] + " "
			out_driver_files += drivers_files[int(i) - 1] + " "

with open("build_opts") as f:
	opts = f.read().split(" ")
	for i in opts:
		if i.isdigit():
			out_build_flags += build_flags[int(i) - 1] + " "
			if build_flags[int(i) - 1] == "-g":
				out_build_flags_c += build_flags[int(i) - 1] + " "

with open("log_opts") as f:
	opts = f.read().split(" ")
	for i in opts:
		if i.isdigit():
			out_build_flags += log_levels[int(i) - 1] + " "


print ("DRIVERS_FILES = " + out_driver_files)
print ("DRIVERS_PATH = kernel/drivers/devs/")
print ("DRIVERS_OBJS = $(patsubst %.cc,%.o,$(addprefix $(DRIVERS_PATH),$(DRIVERS_FILES)))")
print ("DRIVERS_FLAGS = " + out_driver_flags)
print ("FLAGS_CXX = " + out_build_flags)
print ("FLAGS_C = " + out_build_flags_c)
