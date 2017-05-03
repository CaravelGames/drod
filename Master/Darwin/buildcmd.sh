#!/bin/sh

#Currently used to make Steam builds only
# via setting specialbuildname and buildtype below

#Version info:
architecture=64bit
#architecture=32bit
tagver=build6435.$architecture
ver=5.1.0
fullver=$ver.$tagver
version=osx_$fullver

drodlocaldir=DROD_Dev

#Names of Data dir bundles used for different versions and builds.  Add more as needed.
datfile_demo=DemoData.$ver.rar
datfile_full=FullData.$ver.rar

#Name to give the .dmg (i.e. install bundle) as it's created
gamebuildname=drod
dmg_base=$gamebuildname-$ver
dmg_build_name=$dmg_base.dmg
dmg_demo_build_name=$dmg_base-demo.dmg

#Final name for the .dmg (based on build type).  Add more options as needed.
#specialbuildname=
specialbuildname=-Steam

officialname=drod-tss
dmg_beta_download_name=$officialname-osx-beta-$fullver.dmg
dmg_demo_download_name=$officialname-osx$specialbuildname-demo-$fullver.dmg
dmg_full_download_name=$officialname-osx$specialbuildname-full-$fullver.dmg

buildtype=steam
#buildtype=custom

#cat $0

base_working_dir=~/Caravel2
cd $base_working_dir

mv $drodlocaldir $drodlocaldir-$version
cd $drodlocaldir-$version
cd Master/Darwin
#make $gamebuildname-beta
#chmod -R +w $buildtype
#make clean-$buildtype

#Alternate builds:
#For SDL_mixer sound lib:
#make VARIANT_FLAGS="-DMANIFESTO -DCARAVELBUILD -DUSE_SDL_MIXER" $gamebuildname-$buildtype
#For fmod sound lib:
#make VARIANT_FLAGS="-DMANIFESTO -DCARAVELBUILD" $gamebuildname-$buildtype

#########################################################################
#Types of builds to produce

#Beta demo version (not currently supported):
#rm ../../Data.rar
#ln -s $datfile_demo ../../Data.rar
#make BUNDLE_NAME=\"DROD\ TSS\ Beta\ Demo.app\" demobundle-beta
#mv beta/bin/$dmg_demo_build_name ../../../$dmg_beta_download_name

#Demo version:
rm ../../Data.rar
ln -s $datfile_demo ../../Data.rar
make BUNDLE_NAME=\"DROD\ TSS\ Demo.app\" demobundle-$buildtype
mv $buildtype/bin/$dmg_demo_build_name ../../../$dmg_demo_download_name

#Full version:
rm ../../Data.rar
ln -s $datfile_full ../../Data.rar
make BUNDLE_NAME=\"DROD\ TSS.app\" bundle-$buildtype
mv $buildtype/bin/$dmg_build_name ../../../$dmg_full_download_name

####################################################################

cd $base_working_dir
mv $drodlocaldir-$version $drodlocaldir
