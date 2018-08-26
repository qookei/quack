import subprocess

output = subprocess.check_output("i686-elf-nm -f posix kernel.o | c++filt -p | grep -F \" T \" | grep -v \"loader\"", shell=True)

lines = output.splitlines()

print ("#include \"trace.h\"\n\ntrace_elem trace_elems[] = {")

i = 0
while i < len(lines):
	line = lines[i].split()

	if len(line) == 3:
		print ("\t{{{},{},{}}},".format("\"" + line[0].strip().decode("utf-8") + "\"",
		 "0x" + line[2].strip().decode("utf-8"), "0x00000000"))	
	else:
		print ("\t{{{},{},{}}},".format("\"" + line[0].strip().decode("utf-8") + "\"", 
		"0x" +
		line[2].strip().decode("utf-8"), "0x" +
		line[3].strip().decode("utf-8")))

	i += 1

print ("};\n")

print ("uint32_t ntrace_elems = {};".format(len(lines)))
