#export WITH_INSTALL=1
export BUILD_BASE_DIR=$PWD/../temp_build
export CONFIG_OPTIONS="--disable-shared --enable-signed-triang-algo=intmathabs --enable-unsigned-triang-algo=intmathabs --enable-default-instruments-db-location=~/Library/linuxsampler/linuxsampler.db --enable-plugin-dir=~/Library/linuxsampler/plugins --enable-refill-streams=2 --enable-stream-size=320000 --enable-max-voices=200 --enable-max-streams=220 --enable-pthread-testcancel"

case "$BUILD_STYLE" in
    Deployment_i386)
	OPT="-O3 -march=pentium4 -mfpmath=sse -ffast-math -fomit-frame-pointer -funroll-loops"
	;;
	Deployment_ppc)
	OPT="-O3 -ffast-math -fomit-frame-pointer -funroll-loops"
	;;
esac

export CFLAGS="-I$BUILD_BASE_DIR/$BUILD_STYLE/local/include -D_APPLE_C_SOURCE $OPT"
export CXXFLAGS=$CFLAGS
export PKG_CONFIG_PATH="$BUILD_BASE_DIR/$BUILD_STYLE/local/lib/pkgconfig:/usr/local/lib/pkgconfig"
export HAVE_UNIX98=1
export UB_PRODUCTS=bin/linuxsampler
PATH=/opt/local/bin:$PATH  #  Use MacPorts
source $PROJECT_DIR/autoconf_builder.sh
make install-exec
