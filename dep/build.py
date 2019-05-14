import os
def do_os(cmd):
	b = os.system(cmd)
	if b != 0:
		exit(1)
do_os('rm -rf include')
do_os('rm -rf lib')

do_os('mkdir include')
do_os('mkdir lib')
print 'download micro-ecc'

do_os("wget https://github.com/nicehero/micro-ecc/archive/v1.0.tar.gz")
do_os("tar xvf v1.0.tar.gz")
do_os("mv micro-ecc-1.0 include/micro-ecc")
do_os("rm -rf v1.0.tar.gz")

print 'build micro-ecc'

if os.name == "nt":
	do_os("gcc -c include/micro-ecc/uECC.c")
	do_os("ar -r lib/uECC.lib uECC.o")
else:
	#do_os("gcc -c include/micro-ecc/uECC.c")
	#do_os("ar -rcs lib/libuECC.a uECC.o")
	do_os("gcc -shared -fPIC -o lib/libuECC.so include/micro-ecc/uECC.c")
do_os("rm -rf uECC.o")

print 'end micro-ecc'

print 'download asio'
do_os("wget https://github.com/chriskohlhoff/asio/archive/asio-1-12-2.tar.gz")
do_os("tar xvf asio-1-12-2.tar.gz")
do_os("mv asio-asio-1-12-2/asio/include/asio include/")
do_os("mv asio-asio-1-12-2/asio/include/asio.hpp include/asio/")
do_os("rm -rf asio-asio-1-12-2")
do_os("rm -rf asio-1-12-2.tar.gz")

print 'download tiny_sha3'
do_os("git clone https://github.com/mjosaarinen/tiny_sha3")
do_os("mv tiny_sha3 include/")
if os.name == "nt":
	do_os("gcc -c include/tiny_sha3/sha3.c")
	do_os("ar -r lib/sha3.lib sha3.o")
else:
	#do_os("gcc -c include/tiny_sha3/sha3.c")
	#do_os("ar -rcs lib/libsha3.a sha3.o")
	do_os("gcc -shared -fPIC -o lib/libsha3.so include/tiny_sha3/sha3.c")
do_os("rm -rf sha3.o")

print 'download mongo-c' #mongo-c-driver-1.14.0.tar.gz
do_os("wget https://github.com/mongodb/mongo-c-driver/releases/download/1.14.0/mongo-c-driver-1.14.0.tar.gz")
print 'build mongoc'
do_os('tar xvf mongo-c-driver-1.14.0.tar.gz')
do_os('mkdir build_mongoc')
os.chdir('./build_mongoc')
if os.name == "nt":
	p = os.getcwd()
	do_os('cmake -G "Visual Studio 14 2015 Win64" "-DCMAKE_INSTALL_PREFIX=%s\\..\\mongo-c-driver" "-DCMAKE_PREFIX_PATH=%s\\..\\mongo-c-driver" ..\\mongo-c-driver-1.14.0'%(p,p))
	do_os('devenv mongo-c-driver.sln /Build "RelWithDebInfo|x64"')
	do_os('devenv mongo-c-driver.sln /Build "RelWithDebInfo|x64" /project INSTALL')
	os.chdir('../')
	do_os('mv mongo-c-driver/include/libmongoc-1.0/mongoc include/')
	do_os('mv mongo-c-driver/include/libbson-1.0/bson include/')
	do_os('mv mongo-c-driver/lib/* lib/')
	do_os('mv mongo-c-driver/bin/* lib/')
	do_os('rm -rf mongo-c-driver')
	do_os('rm -rf build_mongoc')
else:
	p = os.getcwd()
	do_os('cmake "-DCMAKE_INSTALL_PREFIX=%s/../mongo-c-driver" "-DCMAKE_PREFIX_PATH=%s/../mongo-c-driver" "-DCMAKE_BUILD_TYPE=Release" ../mongo-c-driver-1.14.0'%(p,p))
	do_os('make install')
	os.chdir('../')
	do_os('mv mongo-c-driver/include/libmongoc-1.0/mongoc include/')
	do_os('mv mongo-c-driver/include/libbson-1.0/bson include/')
	try:
		do_os('mv mongo-c-driver/lib/* lib/')
	except:
		pass
	try:
		do_os('mv mongo-c-driver/lib64/* lib/')
	except:
		pass
	do_os('mv mongo-c-driver/bin/* lib/')
	do_os('rm -rf mongo-c-driver')
	do_os('rm -rf build_mongoc')
do_os('rm -rf mongo-c-driver-1.14.0')
do_os('rm -rf mongo-c-driver-1.14.0.tar.gz')

print 'download kcp'
do_os("wget https://github.com/skywind3000/kcp/archive/1.3.tar.gz")
do_os("tar xvf 1.3.tar.gz")
do_os("mv kcp-1.3 include/kcp")
do_os("rm -rf 1.3.tar.gz")
print 'build kcp'
if os.name == "nt":
	body = open("include/kcp/ikcp.c","rb").read()
	body = body.replace("vsprintf(buffer","vsnprintf(buffer,1024")
	open("include/kcp/ikcp.c","wb").write(body)
	do_os("gcc -c include/kcp/ikcp.c")
	do_os("ar -r lib/ikcp.lib ikcp.o")
else:
	do_os("gcc -shared -fPIC -o lib/libikcp.so include/kcp/ikcp.c")
do_os("rm -rf ikcp.o")



do_os("echo 0.1 > done")
print 'done'