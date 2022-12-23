
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
cd drv
FILES=$(find -maxdepth 1 | tr -d './')

for i in $FILES ;
    do

    cd $i
    make
    cp $i.drv ../../bootdisk/sys/
    cd ..
done;
cd ..

# build programs
cd prog
FILES=$(find -maxdepth 1 | tr -d './')

for i in $FILES ;
    do

    cd $i
    make
    cp $i.elf ../../bootdisk/bin/
    cd ..
done;
cd ..

# make the Vireo CD
grub-mkrescue -o vireo.iso bootdisk