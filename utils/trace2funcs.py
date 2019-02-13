import subprocess, fileinput, re

ansi_escape = re.compile(r'\x1B\[[0-?]*[ -/]*[@-~]')

for line in fileinput.input():
    line = ansi_escape.sub('', line)
    if not line.startswith("trace: #"):
        print(line.strip('\n'))
    else:
        ready = line.split('[', 1)[-1][:8]
        command = "addr2line -pfse kernel.elf " + ready
        out = subprocess.check_output(command, shell=True)
        print (line.strip('\n') + " -> " + out.decode("utf-8").strip())
