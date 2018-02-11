import subprocess

output = subprocess.check_output("i686-elf-nm -f posix kernel.o | c++filt -p | grep -F \" T \" | grep -v \"loader\"", shell=True)

lines = output.splitlines()

print ("#include \"trace.h\"\n\ntrace_elem trace_elems[] = {")

i = 0
while i < len(lines):
	line = filter(lambda a: a != 'T', lines[i].split())

	if len(line) == 2:
		j = 0
		while j < len(line):
			print ("\t{%s,%s, %s}," % ("\"" + line[j].strip() + "\"", "0x"+line[j+1].strip(), "0x00000000"))
			j += 2
	else:
		j = 0
		while j < len(line):
			print ("\t{%s,%s, %s}," % ("\"" + line[j].strip() + "\"", "0x"+line[j+1].strip(), "0x"+line[j+2].strip()))
			j += 3

	i += 1

print ("};\n")

print ("uint32_t ntrace_elems = %u;" % len(lines))


