#!/bin/bash
base_dir="$PWD/cmocka"
mkdir -p "$base_dir" || exit 1

tmp_dir=$(mktemp -d)

cd $tmp_dir || exit 1
wget https://cmocka.org/files/1.1/cmocka-1.1.7.tar.xz || exit 1
tar -xJf cmocka-1.1.7.tar.xz || exit 1

src_dir="$tmp_dir/cmocka-1.1.7"
build_dir="$src_dir/build"
install_dir="$src_dir/install"

mkdir -p "$build_dir" || exit 1
mkdir -p "$install_dir" || exit 1

cd "$build_dir"

cmake -DCMAKE_INSTALL_PREFIX="$install_dir" -DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=Debug "$src_dir" || exit 1
make -j4 || exit 1
make install || exit 1

echo "installation du fichier libcmocka.a..."
cp "${install_dir}/lib/libcmocka.a" "${base_dir}" || exit 1
echo -e "\tOK !"

echo "installation du fichier cmocka.h..."
cp ${install_dir}/include/cmocka*.h "${base_dir}" || exit 1
echo -e "\tOK !"

cd "$base_dir"
#rm -rf "$tmp_dir"
