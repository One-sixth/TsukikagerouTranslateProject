import os
import glob
import click
import shutil
import subprocess


png2cgf_exe_path = f'{os.path.dirname(os.path.abspath(__file__))}/png2cgf_tool/png2cgf.exe'
arctool_exe_path = f'{os.path.dirname(os.path.abspath(__file__))}/arc_tool.exe'

png2cgf_cmd = f'{png2cgf_exe_path} {{}} {{}} 1'
arc_pack_cmd = f'{arctool_exe_path} --action archive --in {{}} --out {{}}'

@click.command()
@click.argument('in_dir')
@click.argument('temp_dir')
@click.argument('out_arc_file')
def main(in_dir, temp_dir, out_arc_file):
    print(png2cgf_cmd.format(in_dir, temp_dir))
    subprocess.call(png2cgf_cmd.format(in_dir, temp_dir))

    for file in glob.glob(f'{in_dir}/*.map'):
        basename = os.path.basename(file)
        outfile = f'{temp_dir}/{basename}'
        shutil.copy(file, outfile)
        print(f'{file} -> {outfile}')
    
    print(arc_pack_cmd.format(temp_dir, out_arc_file))
    os.makedirs(os.path.dirname(out_arc_file), exist_ok=True)
    subprocess.call(arc_pack_cmd.format(temp_dir, out_arc_file))
    print('Success')


if __name__ == '__main__':
    main()
