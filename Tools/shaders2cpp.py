import argparse
import os
import os.path

def get_shaders(path):
	files = []
	for file in os.listdir(path):
		if file.endswith('.comp'):
			files.append(os.path.join(path, file))
	files.sort()
	return files

def read_file_as_single_line(path):
	content = ''
	with open(path, 'r') as f:
		for line in f:
			line = line.strip()
			if len(line) > 0:
				content += line
				content += '\\n'
	return content

def var_name_from_path(path):
	varname = os.path.basename(path)
	varname = varname.replace('.', '_')
	varname = varname.replace('-', '_')
	return varname



def main():
	parser = argparse.ArgumentParser()
	parser.add_argument('-f', '--folder', help='folder containing the shaders', required=True)
	parser.add_argument('-o', '--output', help='output file (C header)', required = True)
	args = parser.parse_args()

	files = get_shaders(args.folder)
	with open(args.output, 'w') as of:
		of.write('// Auto-generated file with shaders2cpp.py utility\n\n')
		for file in files:
			varname = var_name_from_path(file)
			of.write('const char %s[] = \n"' % varname)
			of.write(read_file_as_single_line(file))
			of.write('";\n')

if __name__ == '__main__':
	main()
