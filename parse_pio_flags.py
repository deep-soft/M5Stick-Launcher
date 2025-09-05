import configparser, glob, json, os, sys
env = sys.argv[1]
root = os.path.dirname(__file__)
files = [os.path.join(root,'platformio.ini')]
files += glob.glob(os.path.join(root,'boards','*.ini'))
files += glob.glob(os.path.join(root,'boards','*','platformio.ini'))
config = configparser.ConfigParser(allow_no_value=True, delimiters=('='), interpolation=None)
config.read(files)

def expand(section, key):
    if section not in config or key not in config[section]:
        return []
    raw = config[section][key].splitlines()
    lines = []
    for l in raw:
        l = l.split(';',1)[0].strip()
        if l:
            if l.startswith('-D '):
                l = '-D' + l[3:]
            lines.append(l)
    result = []
    for line in lines:
        if line.startswith('${') and line.endswith('}'):
            ref_section, ref_key = line[2:-1].split('.')
            result.extend(expand(ref_section, ref_key))
        else:
            result.append(line)
    return result

base_flags = expand('env', 'build_flags')
flags = base_flags + [f for f in expand(f'env:{env}', 'build_flags') if f not in base_flags]
base_libs = expand('env', 'lib_deps')
libs = base_libs + [l for l in expand(f'env:{env}', 'lib_deps') if l not in base_libs]
partitions = expand(f'env:{env}', 'board_build.partitions')
board = config.get(f'env:{env}', 'board', fallback='').strip()
target = ''
if board:
    board_json = os.path.join(root, 'boards', '_jsonfiles', f'{board}.json')
    if os.path.exists(board_json):
        with open(board_json) as f:
            data = json.load(f)
        target = data.get('build', {}).get('mcu', '')
        extra = data.get('build', {}).get('extra_flags', [])
        if isinstance(extra, str):
            extra = extra.split()
        flags.extend(extra)
out_dir = os.path.join(root, 'build')
os.makedirs(out_dir, exist_ok=True)
with open(os.path.join(out_dir, 'pio_flags.json'), 'w') as f:
    json.dump({
        'flags': flags,
        'libs': libs,
        'target': target,
        'partitions': os.path.basename(partitions[0]) if partitions else ''
    }, f, indent=2)
print('FLAGS=' + ';'.join(flags))
print('LIBS=' + ';'.join(libs))
print('TARGET=' + target)
part_file = os.path.basename(partitions[0]) if partitions else ''
print('PARTITIONS=' + part_file)
