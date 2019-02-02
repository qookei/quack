build_flags = [
	"-g",
]

log_levels = [
	"-DEARLY_LOG_INFO",
	"-DEARLY_LOG_WARN",
	"-DEARLY_LOG_ERR",
	"-DEARLY_LOG_DBG"
]

out_build_flags = ""

with open("build_opts") as f:
	opts = f.read().split(" ")
	for i in opts:
		if i.isdigit():
			out_build_flags += build_flags[int(i) - 1] + " "
			if build_flags[int(i) - 1] == "-g":
				out_build_flags += build_flags[int(i) - 1] + " "

with open("log_opts") as f:
	opts = f.read().split(" ")
	for i in opts:
		if i.isdigit():
			out_build_flags += log_levels[int(i) - 1] + " "


print ("FLAGS_C = " + out_build_flags)
