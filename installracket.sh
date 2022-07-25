cd ..
tar xvf archive/racket-minimal-8.6.0.2-src.tgz
cd racket-8.6.0.2
cd src
mkdir build
cd build
../configure  --enable-bc --enable-cs --enable-libs --enable-shared --prefix=/usr/local
make
make bc
sudo make install
sudo make install-bc

