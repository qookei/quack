import subprocess, fileinput, re, sys

ansi_escape = re.compile(r'\x1B\[[0-?]*[ -/]*[@-~]')
pattern = re.compile(r'[a-zA-Z0-9]*: #[0-9]+: (\[[0-9a-fA-F]{8,16}\])')

if len(sys.argv) < 2:
        print("missing required argument: <filename>")
        sys.exit(1)

with fileinput.FileInput("-") as file_in:
        for line in file_in:
                line = ansi_escape.sub('', line).strip('\n')
                match = pattern.search(line);
                if not match:
                        print(line)
                else:
                        address = match.group(1)[1:-1]
                        out = subprocess.check_output("addr2line -pfse {} {}".format(sys.argv[1], address), shell=True)
                        print(line + " -> " + out.decode("utf-8").strip())
