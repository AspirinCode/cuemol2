#!/bin/sh

cwd=`pwd`
top_srcdir=$cwd/../src/

install_dir=$HOME/app/cuemol2-dev201608

#debug="--enable-debug --enable-m64"
debug="--disable-debug --enable-m64"

usepybr="--with-python=$HOME/app/Python3/"
#usepybr="--disable-python"

#usexrbr="--with-xmlrpc=$HOME/proj64/xmlrpc-c"
usexrbr="--without-xmlrpc"

lcms="--without-lcms"
xz="--with-xz=$HOME/proj/xz"

#use_boost_timer="--enable-boosttimer"

##

boost_dir=/home/ishitani/app/boost_1_60_0/
fftw_dir=/home/ishitani/app/fftw/
cgal_dir=/home/ishitani/app/CGAL-4.6.1/

#######################

config_scr=../src/configure

if test ! -f $config_scr; then
    (
	cd ../src
	aclocal; libtoolize; aclocal; autoheader; automake -a; autoconf;
	cd js
#	aclocal; autoheader; automake -a; autoconf;
	aclocal; libtoolize; aclocal; autoheader; automake -a; autoconf;
    )	
fi

# CFLAGS="-O3" CXXFLAGS="-O3" \

env CC=gcc CXX=g++ CXXFLAGS="-std=c++0x -O" \
$config_scr \
--disable-static \
--enable-shared \
--prefix=$install_dir \
$lcms \
$xz \
$usepybr \
$usexrbr \
$use_boost_timer \
--enable-cli \
--with-boost=$boost_dir \
--with-fftw=$fftw_dir \
--with-cgal=$cgal_dir \
$debug

