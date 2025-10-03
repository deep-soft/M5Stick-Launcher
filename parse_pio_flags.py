import configparser, glob, json, os, sys

env = sys.argv[1]
board_dir = sys.argv[2] if len(sys.argv) > 2 else env

root = os.path.dirname(__file__)
files = [os.path.join(root, 'platformio.ini')]
files += glob.glob(os.path.join(root, 'boards', '*.ini'))
files += glob.glob(os.path.join(root, 'boards', '*', 'platformio.ini'))
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
sdk_lines = []
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
        flash_size = data.get('upload', {}).get('flash_size')
        if flash_size:
            sdk_lines.append(f'CONFIG_ESPTOOLPY_FLASHSIZE="{flash_size}"')
            sdk_lines.append(f'CONFIG_ESPTOOLPY_FLASHSIZE_{flash_size}=y')
        flash_mode = data.get('build', {}).get('flash_mode')
        if flash_mode:
            sdk_lines.append(f'CONFIG_ESPTOOLPY_FLASHMODE="{flash_mode}"')
            sdk_lines.append(f'CONFIG_ESPTOOLPY_FLASHMODE_{flash_mode.upper()}=y')
        f_flash = data.get('build', {}).get('f_flash')
        if f_flash:
            try:
                mhz = int(str(f_flash).rstrip('L')) // 1000000
                sdk_lines.append(f'CONFIG_ESPTOOLPY_FLASHFREQ="{mhz}m"')
                sdk_lines.append(f'CONFIG_ESPTOOLPY_FLASHFREQ_{mhz}M=y')
            except Exception:
                pass
        if target:
            sdk_lines.append(f'CONFIG_IDF_TARGET="{target}"')

#pinout = os.path.join(root, 'boards', 'pinouts', f'{board_dir}.h')
#if os.path.exists(pinout):
#    with open(pinout) as f:
#        for line in f:
#            line = line.split('//',1)[0].strip()
#            if line.startswith('#define'):
#                parts = line.split()
#                if len(parts) >= 2:
#                    name = parts[1]
#                    if name == 'Pins_Arduino_h':
#                        continue
#                    value = ' '.join(parts[2:]) if len(parts) >= 3 else None
#                    if not value:
#                        flags.append(f'-D{name}')
#                    else:
#                        flags.append(f'-D{name}={value}')
            # elif line.startswith('static const uint8_t'):
            #     # pins_arduino.h exposes these as global variables; avoid converting to macros
            #     line = line.replace(';', '')
            #     parts = line.split()
            #     if len(parts) >= 6 and parts[4] == '=':
            #         name = parts[3]
            #         value = parts[5]
            #         flags.append(f'-D{name}={value}')

# ensure standard Arduino pin definitions are available at build time
# the pins_arduino.h header will be force-included via CMake compile options
out_dir = os.path.join(root, 'build')
os.makedirs(out_dir, exist_ok=True)
with open(os.path.join(out_dir, 'pio_flags.json'), 'w') as f:
    json.dump({
        'flags': flags,
        'libs': libs,
        'target': target,
        'partitions': os.path.basename(partitions[0]) if partitions else ''
    }, f, indent=2)
flags = [f.replace('"', '\"').replace("'", "") for f in flags]
print('FLAGS=' + ';'.join(flags))
print('LIBS=' + ';'.join(libs))
print('TARGET=' + target)
part_file = os.path.basename(partitions[0]) if partitions else ''
print('PARTITIONS=' + part_file)
sdk_path = ''
if sdk_lines:
    sdk_path = os.path.join(out_dir, 'pio_sdkconfig.defaults')
    with open(sdk_path, 'w') as f:
        f.write('\n'.join(sdk_lines) + '\n')
print('SDKCONFIG=' + os.path.basename(sdk_path) if sdk_path else 'SDKCONFIG=')
