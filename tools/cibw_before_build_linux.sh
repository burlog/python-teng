#!/bin/bash
set -e

echo "Running cibw_before_build_linux.sh"
yum update -q -y
yum install -y pcre2-devel glib2-devel bison flex nss-tools python3 python3-pip
python3 -m pip install -I -U meson ninja

git config --global url."https://github.com/".insteadOf "git@github.com:"

mkdir -p /usr/local/lib/pkgconfig
cat << EOF > /usr/local/lib/pkgconfig/boost.pc
prefix=/usr
exec_prefix=\${prefix}
libdir=\${exec_prefix}/lib/x86_64-linux-gnu
includedir=\${prefix}/include/boost

Name: libboost
Description: boost library.
Version: 1.82.0
Requires:
URL: [not yet]
Libs: -L\${libdir}
Libs.private:
Cflags: -I\${includedir}
EOF
cat << EOF > /usr/local/lib/pkgconfig/boost-python3.pc
prefix=/usr
exec_prefix=\${prefix}
libdir=\${exec_prefix}/lib/x86_64-linux-gnu
includedir=\${prefix}/include/boost

Name: libboost-python3
Description: boost-python3 library.
Version: 1.82.0
Requires:
URL: [not yet]
Libs: -L\${libdir} -lboost_python311 -lpython3.11
EOF
cd /project

# Check if in /usr/lib/x86_64-linux-gnu is an libboost_python*.so.*
# and delete it if it exists
if ls /usr/lib/x86_64-linux-gnu/libboost_python*.so.* 1> /dev/null 2>&1; then
    echo "Removing existing libboost_python*.so* and libboost_python*.a files"
    rm -f /usr/lib/x86_64-linux-gnu/libboost_python*.so*
    rm -f /usr/lib/x86_64-linux-gnu/libboost_python*.a
else
    echo "No existing libboost_python*.so* files found, skipping removal."
fi

BOOST_VERSION="1.82.0"
BOOST_ARCHIVE="boost_1_82_0.tar.gz"
BOOST_URL="https://archives.boost.io/release/${BOOST_VERSION}/source/${BOOST_ARCHIVE}"
BOOST_SHA256="66a469b6e608a51f8347236f4912e27dc5c60c60d7d53ae9bfe4683316c6f04c"
INSTALL_DIR="/usr"
PYTHON_VERSION=$(python3 --version | awk '{print $2}' | cut -d. -f1,2)
echo "PYTHON_VERSION=$PYTHON_VERSION"
BOOST_PYTHON_PC="/usr/local/lib/pkgconfig/boost-python3.pc"

if [ -f "$BOOST_PYTHON_PC" ]; then
    sed -i "s/-lboost_python[0-9]*/-lboost_python${PYTHON_VERSION//./}/" "$BOOST_PYTHON_PC"
    sed -i "s/-lpython[0-9.]* /-lpython${PYTHON_VERSION} /" "$BOOST_PYTHON_PC"
    sed -i "s/-lpython[0-9.]*\$/-lpython${PYTHON_VERSION}/" "$BOOST_PYTHON_PC"
    sed -i "s/Version: [0-9.]*\$/Version: ${BOOST_VERSION}/" "$BOOST_PYTHON_PC"
    echo "Updated $BOOST_PYTHON_PC"
else
    echo "File $BOOST_PYTHON_PC not found, skipping update."
fi

# Check if Boost directory already exists
if [ ! -d "boost_1_82_0" ]; then
    curl -s -o $BOOST_ARCHIVE $BOOST_URL
    echo "$BOOST_SHA256  $BOOST_ARCHIVE" | sha256sum -c -
    echo "Extracting Boost from $BOOST_ARCHIVE"
    tar -xzf $BOOST_ARCHIVE
else
    echo "Boost directory 'boost_1_82_0' already exists, skipping download and extraction."
fi
cd boost_1_82_0

# Boost installation
./bootstrap.sh --prefix="$INSTALL_DIR" > /dev/null
./b2 install --prefix="$INSTALL_DIR" \
    --libdir="$INSTALL_DIR/lib/x86_64-linux-gnu" \
    --includedir="$INSTALL_DIR/include" \
    --with-python python=$PYTHON_VERSION \
    -j$(nproc) -q -d0

cd /project
