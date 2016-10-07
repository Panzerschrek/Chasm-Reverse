import os
import sys

def GetCELFiles(dir):
	result = []

	for path in os.listdir(dir):
		full_name= os.path.join( dir, path )

		if os.path.isdir(full_name):
			result+= GetCELFiles( full_name )

		elif path.endswith(".CEL"):
			result.append( full_name )

	return result


def main():
	converter_path= os.path.join( "build-Cel-Depacker-Desktop_Qt_5_3_0_MinGW_32bit-Release", "release", "Cel-Depacker.exe" )

	files= GetCELFiles( "CSM.BIN.depacked" )

	for file in files:
		os.system( converter_path + " -i " + file )

if __name__ == "__main__":
	sys.exit(main())

