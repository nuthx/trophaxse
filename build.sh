export VITASDK=/usr/local/vitasdk
export PATH=$VITASDK/bin:$PATH


cd kernel/
cmake .
make install



cd ../user/
cmake .
make install


cd ../app/
mv ../kernel/kernel.skprx kernel.skprx
mv ../user/user.suprx user.suprx
cmake .
make