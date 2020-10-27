#Python 3 support

import os 
import urllib.request
import ntpath
import zipfile
import tarfile
import sys
import shutil
import glob
from subprocess import Popen, PIPE

from pprint import pprint

def overrideOptions():
	global DevEnvPath,DepsToBuild,IgnoreBuilds

	DevEnvPath = 'C:\\Program Files (x86)\\Microsoft Visual Studio 12.0\\Common7\\IDE\\devenv.com'
	#DevEnvPath = 'C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\Community\\Common7\\IDE\\devenv.com'
	IgnoreBuilds = 0 # Set this to 0 to skip the build step
	DepsToBuild = "all" # Change it to array of lib names to build and copy only the specific ones

#### DEFAULT OPTIONS ####
DevEnvPath = 'C:\\Program Files (x86)\\Microsoft Visual Studio 12.0\\Common7\\IDE\\devenv.com'
IgnoreBuilds = 0
DepsToBuild = "all"

#### OVERRIDE OPTIONS ####
overrideOptions()

#### ACTUAL SCRIPT ####
AbsoluteDir = os.path.dirname(os.path.realpath(__file__))
RootDir = os.path.dirname(AbsoluteDir)
InstallDir = RootDir + "\\DepsSrc\\"
DependenciesDir = RootDir + "\\Deps\\"
DependenciesIncludes = DependenciesDir + "Include\\"
DependenciesLibraries = DependenciesDir + "Library\\"
DependenciesDlls = DependenciesDir + "Dll\\"
ZlibUrl = 'http://www.zlib.net/fossils/zlib-1.2.11.tar.gz'
LibOggName = 'libogg-1.3.0'
SdlName = 'sdl2-2.0.12'

if DepsToBuild == "all":
	DepsToBuild = [
		'curl-7.27.0',
		'expat-2.1.0',
		'fmodapi-375-win',
		'jpeg-6b',
		'json-0.6.0-rc2',
		'libogg-1.3.0',
		'libtheora-1.1.1',
		'libvorbis-1.3.3',
		'lpng-1512',
		'metakit-2.4.9.7',
		SdlName,
		'sdl-ttf-2.0.14',
		'zlib'
	]

dependencies = {
	'curl-7.27.0': {
		'urls': ['https://curl.haxx.se/download/curl-7.27.0.zip'],
		'include': {
			'curl-7.27.0/include/curl': 'curl'
		},
		'builds': [
			{
				'sln': 'curl-7.27.0/vc6curl.dsw',
				'ignoreErrors': 1
			},
			{
				'sln': 'curl-7.27.0/vc6curl.sln',
				'configs': [
					['LIB Debug', '/project', 'libcurl'],
					['LIB Release', '/project', 'libcurl'],
					['DLL Debug', '/project', 'libcurl'],
					['DLL Release', '/project', 'libcurl'],
				]
			}
		],
		'libs': {
			'curl-7.27.0/lib/DLL-Debug/libcurld_imp.lib': 'Debug',
			'curl-7.27.0/lib/DLL-Release/libcurl_imp.lib': 'Release'
		},
		'dlls': {
			'curl-7.27.0/lib/DLL-Debug/libcurld.dll': 'Debug',
			'curl-7.27.0/lib/DLL-Release/libcurl.dll': 'Release'
		}
	},
	'expat-2.1.0': {
		'urls': ['https://downloads.sourceforge.net/project/expat/expat/2.1.0/expat-2.1.0.tar.gz'],
		'builds': [
			{
				'sln': 'expat-2.1.0/expat.dsw',
				'ignoreErrors': 1
			},
			{
				'sln': 'expat-2.1.0/expat.sln',
				'configs': [
					['Debug', '/project', 'expat'],
					['Release', '/project', 'expat'],
				]
			}
		],
		'include': {
			'expat-2.1.0/lib/amigaconfig.h': '',
			'expat-2.1.0/lib/ascii.h': '',
			'expat-2.1.0/lib/asciitab.h': '',
			'expat-2.1.0/lib/expat.h': '',
			'expat-2.1.0/lib/expat_external.h': '',
			'expat-2.1.0/lib/iasciitab.h': '',
			'expat-2.1.0/lib/internal.h': '',
			'expat-2.1.0/lib/latin1tab.h': '',
			'expat-2.1.0/lib/macconfig.h': '',
			'expat-2.1.0/lib/nametab.h': '',
			'expat-2.1.0/lib/utf8tab.h': '',
			'expat-2.1.0/lib/winconfig.h': '',
			'expat-2.1.0/lib/xmlrole.h': '',
			'expat-2.1.0/lib/xmltok.h': '',
			'expat-2.1.0/lib/xmltok_impl.h': '',
		},
		'libs':{
			'expat-2.1.0/win32/bin/Debug/libexpat.lib': 'Debug',
			'expat-2.1.0/win32/bin/Release/libexpat.lib': 'Release',
		},
		'dlls': {
			'expat-2.1.0/win32/bin/Debug/libexpat.dll': 'Debug',
			'expat-2.1.0/win32/bin/Release/libexpat.dll': 'Release'
		}
	},
	'fmodapi-375-win': {
		# No longer available on the internet
		'urls': ['http://cdn.retrocade.net/fmodapi375win.zip'],
		'include': {
			'fmodapi375win/api/inc/': ''
		},
		'libs': {
			'fmodapi375win/api/lib/': ['Release', 'Debug']
		}, 
		'dlls': {
			'fmodapi375win/api/fmod.dll': ['Release', 'Debug']
		}
	},
	'jpeg-6b':{
		# Custom version with prepared VS project files
		'urls': ['http://cdn.retrocade.net/jpeg-6b.zip'],
		'builds': [
			{
				'sln': 'jpeg-6b/jpeglib/jpeglib.sln',
				'configs': [
					['Debug', '/project', 'jpeglib'],
					['Release', '/project', 'jpeglib']
				]
			}
		],
		'include': {
			'jpeg-6b/cderror.h': '',
			'jpeg-6b/cdjpeg.h': '',
			'jpeg-6b/jchuff.h': '',
			'jpeg-6b/jconfig.h': '',
			'jpeg-6b/jdct.h': '',
			'jpeg-6b/jdhuff.h': '',
			'jpeg-6b/jerror.h': '',
			'jpeg-6b/jinclude.h': '',
			'jpeg-6b/jmemsys.h': '',
			'jpeg-6b/jmorecfg.h': '',
			'jpeg-6b/jpegint.h': '',
			'jpeg-6b/jpeglib.h': '',
			'jpeg-6b/jversion.h': '',
			'jpeg-6b/transupp.h': ''
		},
		'libs': {
			'jpeg-6b/jpeglib/Debug/jpeglib.lib': 'Debug',
			'jpeg-6b/jpeglib/Release/jpeglib.lib': 'Release'
		}
	},
	'json-0.6.0-rc2': {
		# Release build had to have assembly generation removed from lib_json
		'urls': ['http://cdn.retrocade.net/jsoncpp-src-0.6.0-rc2.zip'],
		'builds': [
			{
				'sln': 'jsoncpp-src-0.6.0-rc2/makefiles/vs71/jsoncpp.sln',
				'configs': [
					['Debug', '/project', 'lib_json'],
					['Release', '/project', 'lib_json']
				]
			}
		],
		'include': {
			'jsoncpp-src-0.6.0-rc2/include/json/autolink.h': 'json',
			'jsoncpp-src-0.6.0-rc2/include/json/config.h': 'json',
			'jsoncpp-src-0.6.0-rc2/include/json/features.h': 'json',
			'jsoncpp-src-0.6.0-rc2/include/json/forwards.h': 'json',
			'jsoncpp-src-0.6.0-rc2/include/json/json.h': 'json',
			'jsoncpp-src-0.6.0-rc2/include/json/reader.h': 'json',
			'jsoncpp-src-0.6.0-rc2/include/json/value.h': 'json',
			'jsoncpp-src-0.6.0-rc2/include/json/writer.h': 'json'
		},
		'libs': {
			'jsoncpp-src-0.6.0-rc2/build/vs71/debug/lib_json/json_vc71_libmtd.lib': 'Debug',
			'jsoncpp-src-0.6.0-rc2/build/vs71/release/lib_json/json_vc71_libmt.lib': 'Release'
		}
	},
	'libogg-1.3.0': {
		'urls': ['http://downloads.xiph.org/releases/ogg/libogg-1.3.0.tar.gz'],
		'builds': [
			{
				'sln': 'libogg-1.3.0/win32/VS2010/libogg_static.sln',
				'configs': [
					['Debug', '/project', 'libogg_static.vcxproj'],
					['Release', '/project', 'libogg_static.vcxproj']
				]
			}
		],
		'include': {
			'libogg-1.3.0/include/ogg/ogg.h': 'ogg',
			'libogg-1.3.0/include/ogg/os_types.h': 'ogg'
		},
		'libs': {
			'libogg-1.3.0/win32/VS2010/Win32/Debug/libogg_static.lib': 'Debug',
			'libogg-1.3.0/win32/VS2010/Win32/Release/libogg_static.lib': 'Release'
		}
	},
	'libtheora-1.1.1': {
		'urls': ['http://downloads.xiph.org/releases/theora/libtheora-1.1.1.tar.gz'],
		'needsLibOgg': 1,
		'builds': [
			{
				'sln': 'libtheora-1.1.1/win32/VS2008/libtheora_static.sln',
				'configs': [
					['Debug', '/project', 'libtheora_static'],
					['Release', '/project', 'libtheora_static']
				]
			}
		],
		'include': {
			'libtheora-1.1.1/include/theora/codec.h': 'theora',
			'libtheora-1.1.1/include/theora/theora.h': 'theora',
			'libtheora-1.1.1/include/theora/theoradec.h': 'theora',
			'libtheora-1.1.1/include/theora/theoraenc.h': 'theora'
		},
		'libs': {
			'libtheora-1.1.1/win32/VS2008/Win32/Debug/libtheora_static.lib': 'Debug',
			'libtheora-1.1.1/win32/VS2008/Win32/Release/libtheora_static.lib': 'Release'
		}
	},
	'libvorbis-1.3.3': {
		'urls': ['http://downloads.xiph.org/releases/vorbis/libvorbis-1.3.5.tar.gz'],
		'needsLibOgg': 1,
		'builds': [
			{
				'sln': 'libvorbis-1.3.5/win32/VS2010/vorbis_static.sln',
				'configs': [
					['Debug', '/project', 'vorbisenc'],
					['Release', '/project', 'vorbisenc'],
					['Debug', '/project', 'vorbisdec'],
					['Release', '/project', 'vorbisdec'],
					['Debug', '/project', 'libvorbisfile'],
					['Release', '/project', 'libvorbisfile'],
					['Debug', '/project', 'libvorbis_static'],
					['Release', '/project', 'libvorbis_static']
				]
			}
		],
		'include': {
			'libvorbis-1.3.5/include/vorbis/codec.h': 'vorbis',
			'libvorbis-1.3.5/include/vorbis/vorbisenc.h': 'vorbis',
			'libvorbis-1.3.5/include/vorbis/vorbisfile.h': 'vorbis'
		},
		'libs': {
			'libvorbis-1.3.5/win32/VS2010/Win32/Debug/libvorbis_static.lib': 'Debug',
			'libvorbis-1.3.5/win32/VS2010/Win32/Debug/libvorbisfile_static.lib': 'Debug',
			'libvorbis-1.3.5/win32/VS2010/Win32/Release/libvorbis_static.lib': 'Release',
			'libvorbis-1.3.5/win32/VS2010/Win32/Release/libvorbisfile_static.lib': 'Release'
		}
	},
	'lpng-1512': {
		'urls': ['https://downloads.sourceforge.net/project/libpng/libpng15/older-releases/1.5.12/lpng1512.zip'],
		'needsZlib': 1,
		'builds': [
			{
				'sln': 'lpng1512/projects/visualc71/libpng.sln',
				'configs': [
					['LIB Debug', '/project', 'libpng.vcxproj'],
					['LIB Release', '/project', 'libpng.vcxproj'],
					['DLL Debug', '/project', 'libpng.vcxproj'],
					['DLL Release', '/project', 'libpng.vcxproj']
				]
			}
		],
		'include': {
			'lpng1512/png.h': '',
			'lpng1512/pngconf.h': '',
			'lpng1512/pngdebug.h': '',
			'lpng1512/pnginfo.h': '',
			'lpng1512/pnglibconf.h': '',
			'lpng1512/pngpriv.h': '',
			'lpng1512/pngstruct.h': ''
		},
		'libs': {
			'lpng1512/projects/visualc71/Win32_LIB_Debug/libpngd.lib': 'Debug',
			'lpng1512/projects/visualc71/Win32_LIB_Release/libpng.lib': 'Release',
			'lpng1512/projects/visualc71/Win32_DLL_Debug/ZLib/zlib.lib': 'Debug',
			'lpng1512/projects/visualc71/Win32_DLL_Release/ZLib/zlib.lib': 'Release'
		},
		'dlls':{
			'lpng1512/projects/visualc71/Win32_DLL_Debug/ZLib/zlib1d.dll': 'Debug',
			'lpng1512/projects/visualc71/Win32_DLL_Release/ZLib/zlib1.dll': 'Release'
		}
	},
	'metakit-2.4.9.7': {
		'urls': ['https://github.com/jnorthrup/metakit/archive/master.zip'],
		'builds': [
			{
				'sln': 'metakit-master/win/msvc70/mksrc.sln',
				'configs': [
					['Debug', '/project', 'mkbug.vcxproj', '/projectconfig', 'Debug'],
					['Release', '/project', 'mkbug.vcxproj', '/projectconfig', 'Release']
				]
			}
		],
		"include": {
			"metakit-master/include/mk4.h": "",
			"metakit-master/include/mk4dll.h": "",
			"metakit-master/include/mk4io.h": "",
			"metakit-master/include/mk4str.h": "",
			"metakit-master/include/mk4.inl": ""
		},
		'libs': {
			'metakit-master/builds/mk4vc70s_d.lib': 'Debug',
			'metakit-master/builds/mk4vc70s.lib': 'Release'
		}
	},
	SdlName: {
		'urls': ['https://www.libsdl.org/release/SDL2-2.0.12.zip'],
		'builds': [
			{
				'sln': 'sdl2-2.0.12/VisualC/SDL.sln',
				'configs': [
					['Debug', '/project', 'SDL2'],
					['Release', '/project', 'SDL2'],
					['Debug', '/project', 'SDL2main'],
					['Release', '/project', 'SDL2main']
				]
			}
		],
		'include': {
			'SDL2-2.0.12/include': '' #Too many files to list them one by one
		},
		'libs': {
			'SDL2-2.0.12/VisualC/Win32/Debug/SDL2.lib': 'Debug',
			'SDL2-2.0.12/VisualC/Win32/Release/SDL2.lib': 'Release',
			'SDL2-2.0.12/VisualC/Win32/Debug/SDL2main.lib': 'Debug',
			'SDL2-2.0.12/VisualC/Win32/Release/SDL2main.lib': 'Release'
		},
		'dlls': {
			'SDL2-2.0.12/VisualC/Win32/Debug/SDL2.dll': 'Debug',
			'SDL2-2.0.12/VisualC/Win32/Release/SDL2.dll': 'Release'
		}
	},
	'sdl-ttf-2.0.14': {
		'urls': ['https://www.libsdl.org/projects/SDL_ttf/release/SDL2_ttf-devel-2.0.14-VC.zip'],
		'include': {
			'SDL2_ttf-2.0.14/include': ''
		},
		'libs': {
			'SDL2_ttf-2.0.14/lib/x86/SDL2_ttf.lib': ['Debug', 'Release']
		},
		'dlls': {
			'SDL2_ttf-2.0.14/lib/x86/SDL2_ttf.dll': ['Debug', 'Release'],
			'SDL2_ttf-2.0.14/lib/x86/libfreetype-6.dll': ['Debug', 'Release']
		}
	},
	'zlib': {
		'urls': [ZlibUrl],
		'include': {
			'zlib-1.2.11/crc32.h': '',
			'zlib-1.2.11/deflate.h': '',
			'zlib-1.2.11/gzguts.h': '',
			'zlib-1.2.11/inffast.h': '',
			'zlib-1.2.11/inffixed.h': '',
			'zlib-1.2.11/inflate.h': '',
			'zlib-1.2.11/inftrees.h': '',
			'zlib-1.2.11/trees.h': '',
			'zlib-1.2.11/zconf.h': '',
			'zlib-1.2.11/zlib.h': '',
			'zlib-1.2.11/zutil.h': ''
		}
	}
}

def execute():
	for name in DepsToBuild:
		dependency = dependencies[name]
		print('')
		print('')
		print('===== DEPENDENCY: ' + name + ' =====')
		filename = ''
		absolutePath = ''

		for url in dependency['urls']:
			print('Downloading dependency ' + name)

			fileName = ntpath.basename(url)
			finalPath = InstallDir + fileName

			if (os.path.exists(finalPath)):
				print('Already downloaded.')
				break

			
			urllib.request.urlretrieve(url, finalPath)

			if os.path.exists(finalPath):
				break

		if not os.path.exists(finalPath):
			print('Failed to download this dependency')
			continue

		print('Unzipping')
		unzipDir = InstallDir + name + "\\"
		if (os.path.exists(unzipDir)):
			print('Already unzipped')
		else:
			os.makedirs(unzipDir)
			unpack(finalPath, unzipDir)

		if 'needsZlib' in dependency:
			downloadCodependency(ZlibUrl, unzipDir, 'zlib-1.2.11', 'zlib')

		if 'needsLibOgg' in dependency:
			toDir = unzipDir + "libogg"
			if (os.path.exists(toDir)):
				shutil.rmtree(toDir)
			shutil.copytree(InstallDir + LibOggName + "/libogg-1.3.0/", toDir)

		if not IgnoreBuilds and 'builds' in dependency:
			for build in dependency['builds']:
				ignoreErrors = 'ignoreErrors' in build

				if 'pre' in build:
					for pre in build['pre']:
						runShell(pre, ignoreErrors)
				
				SlnPath = unzipDir + build['sln']

				if (not SlnPath.endswith('.sln')):
					targetName = os.path.splitext(SlnPath)[0]+'.sln'
					if (os.path.exists(targetName)):
						continue;

				print('Upgrading SLN ' + build['sln'])
				runShell([DevEnvPath, SlnPath, '/upgrade'], ignoreErrors)
				
				if not 'configs' in build:
					continue

				for commands in build['configs']:
					commands.insert(0, DevEnvPath)
					commands.insert(1, SlnPath)
					commands.insert(2, '/build')
					runShell(commands, ignoreErrors)

		if 'include' in dependency:
			for (path, target) in dependency['include'].items():
				fromPath = unzipDir + path
				toPath = DependenciesIncludes + target
				copyDirOrFile(fromPath, toPath)

		if 'libs' in dependency:
			for (path, target) in dependency['libs'].items():
				fromPath = unzipDir + path
				if isinstance(target, list):
					for path in target:
						toPath = DependenciesLibraries + path
						copyDirOrFile(fromPath, toPath)
				else:
					toPath = DependenciesLibraries + target
					copyDirOrFile(fromPath, toPath)


		if 'dlls' in dependency:
			for (path, target) in dependency['dlls'].items():
				fromPath = unzipDir + path
				if isinstance(target, list):
					for path in target:
						toPath = DependenciesDlls + path
						copyDirOrFile(fromPath, toPath)
				else:
					toPath = DependenciesDlls + target
					copyDirOrFile(fromPath, toPath)


def downloadCodependency(url, unzipPath, fromName, toName):
	zlibFileName = os.path.basename(ZlibUrl)
	zipPath = unzipPath + zlibFileName
	if not os.path.exists(zipPath):
		urllib.request.urlretrieve(url, zipPath)
		if not os.path.exists(zipPath):
			raise FileNotFoundError("Failed to download Zlib dependency")

	if (not os.path.exists(unzipPath + toName)):
		unpack(zipPath, unzipPath)
		os.rename(unzipPath + fromName, unzipPath + toName)


def runShell(args, ignoreErrors):
	print("Execute: " + ' '.join(args))
	p = Popen(args, stdin=PIPE, stdout=PIPE, stderr=PIPE)
	output, err = p.communicate(b"input data that is passed to subprocess' stdin")
	output = bytes.decode(output)
	err = bytes.decode(err)
	rc = p.returncode

	if (not ignoreErrors and rc != 0):
		print('OUT >> ' + output.replace('\n', '\n       '))
		print('ERR >> ' + err.replace('\n', '\n       '))
		print('RC  >> ' + str(rc))
		input("AN ERROR OCCURED, Press enter to continue")

def unpack(file, unzipDir):
	if (file.endswith('zip')):
		zip_ref = zipfile.ZipFile(file, 'r')
		zip_ref.extractall(unzipDir)
		zip_ref.close()
	else:
		tar = tarfile.open(file)
		tar.extractall(unzipDir)
		tar.close()

def forceMakeDir(path):
	if not os.path.exists(path):
		os.makedirs(path)
		if not os.path.exists(path):
			sys.exit('Failed to create directory: ' + path)

def copyDirOrFile(fromPath, toPath):
	if (isinstance(toPath, list)):
		for path in toPath:
			copyDirOrFile(fromPath, path)
		return

	print("Copying " + fromPath + " => " + toPath)
	if (os.path.exists(fromPath)):
		if (os.path.isdir(fromPath)):
			if not os.path.exists(toPath):
				os.makedirs(toPath)

			for filename in glob.glob(os.path.join(fromPath, '**')):
				if (os.path.isdir(filename)):
					targetPath = toPath + "/" + os.path.basename(filename)
					if os.path.exists(targetPath):
						shutil.rmtree(targetPath)
					shutil.copytree(filename, targetPath)
				else:
					if filename.endswith('.obj'):
						continue;
					if filename.endswith('.log'):
						continue;
					if filename.endswith('.exe'):
						continue;
					print(" > " + filename + " => " + toPath)
					shutil.copy(filename, toPath)
		else:
			if not os.path.exists(toPath):
				os.makedirs(toPath)

			print(" > " + fromPath + " => " + toPath)
			shutil.copy(fromPath, toPath)
	else:
		print("File does not exist: " + fromPath)

forceMakeDir(InstallDir)
forceMakeDir(DependenciesDir)
forceMakeDir(DependenciesIncludes)
forceMakeDir(DependenciesLibraries)
forceMakeDir(DependenciesLibraries + "Debug")
forceMakeDir(DependenciesLibraries + "Release")
forceMakeDir(DependenciesDlls)
forceMakeDir(DependenciesDlls + "Debug")
forceMakeDir(DependenciesDlls + "Release")

execute()