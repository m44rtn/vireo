
# build & copy kernel
cd kernel
make all
cp bin/vireo.sys ../bootdisk/vireo.sys

# build & copy syslib
cd ../syslib
make
cp bin/* ../bootdisk/syslib/
cp lib/include/* ../bootdisk/syslib/include/
cd ..

# build external drivers
FILES=$(find drv -maxdepth 1 | grep -e 'drv/')

for i in $FILES ;
    do

    cd $i
    make 
    cd ..

    # FIXME: copy driver to sys folder on bootdisk
done;


# make the Vireo CD
grub-mkrescue -o vireo.iso bootdisk