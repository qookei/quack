import subprocess, fileinput, re, sys

ansi_escape = re.compile(r'\x1B\[[0-?]*[ -/]*[@-~]')
stack_pattern = re.compile(r'[a-zA-Z0-9]*: #[0-9]+: (\[[0-9a-fA-F]{8,16}\])')
reg_pattern = re.compile(r'cpu: rip: ([0-9a-f]{8,16})')

def addr2line(addr):
    out = subprocess.check_output("addr2line -pfse {} {}".format(sys.argv[1], addr), shell=True)
    return re.sub(r'(\(discriminator [0-9]*\))', '', out.decode("utf-8").strip())

if len(sys.argv) < 2:
        print("missing required argument: <filename>")
        sys.exit(1)

with fileinput.FileInput("-") as file_in:
        for line in file_in:
                line = ansi_escape.sub('', line).strip('\n')
                stack_match = stack_pattern.search(line);
                reg_match = reg_pattern.search(line);
                if not stack_match and not reg_match:
                        print(line)
                elif stack_match:
                        print(line + " -> " + addr2line(stack_match.group(1)[1:-1]))
                else:
                        print(line + "\nrip -> " + addr2line(reg_match.group(1)))
