import os

os.system('rm -rf include')
os.system('rm -rf lib')

os.system('mkdir include')
os.system('mkdir lib')
print 'download micro-ecc'

os.system("wget https://github.com/nicehero/micro-ecc/archive/v1.0.tar.gz")
os.system("tar xvf v1.0.tar.gz")
os.system("mv micro-ecc-1.0 include/micro-ecc")
os.system("rm -rf v1.0.tar.gz")

print 'build micro-ecc'

os.system("gcc -c include/micro-ecc/uECC.c")
os.system("ar -r lib/uECC.lib uECC.o")
os.system("rm -rf uECC.o")

print 'end micro-ecc'

print 'download asio'
os.system("wget https://github.com/chriskohlhoff/asio/archive/asio-1-12-2.tar.gz")
os.system("tar xvf asio-1-12-2.tar.gz")
os.system("mv asio-asio-1-12-2/asio/include include/asio")
os.system("rm -rf asio-asio-1-12-2")
os.system("rm -rf asio-1-12-2.tar.gz")

print 'download tiny_sha3'
os.system("git clone https://github.com/mjosaarinen/tiny_sha3")
os.system("mv tiny_sha3 include/")
os.system("gcc -c include/tiny_sha3/sha3.c")
os.system("ar -r lib/sha3.lib sha3.o")
os.system("rm -rf sha3.o")


os.system("echo 0.1 > done")
print 'done'