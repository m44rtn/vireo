cd kernel
make all
cp bin/vireo.sys ../bootdisk/vireo.sys

cd ../syslib
make
cp bin/* ../bootdisk/syslib/
cp lib/include/* ../bootdisk/syslib/include/
cd ..

grub-mkrescue -o vireo.iso bootdisk