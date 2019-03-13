// asio_server.cpp : Defines the entry point for the console application.
//

#include "Service.h"
#include "TcpServer.h"
#include <micro-ecc/uECC.h>
#include <windows.h>
#include "Log.h"
int main(int argc, char* argv[])
{
	int i = 0;
	int c = 0;
	uint8_t privateKey[32] = { 0 };
	uint8_t publicKey[64] = { 0 };
	uint8_t hash[32] = { 0 };
	uint8_t sig[64] = { 0 };

	const struct uECC_Curve_t * curves[5];
	int num_curves = 0;
#if uECC_SUPPORTS_secp256k1
	curves[num_curves++] = uECC_secp256k1();
#endif
	if (!uECC_make_key(publicKey, privateKey, curves[c])) {
		printf("uECC_make_key() failed\n");
		return 1;
	}
	memcpy(hash, publicKey, sizeof(hash));

	if (!uECC_sign(privateKey, hash, sizeof(hash), sig, curves[c])) {
		printf("uECC_sign() failed\n");
		return 1;
	}
	double dff;
	LARGE_INTEGER  large_interger;
	QueryPerformanceFrequency(&large_interger);
	dff = double(large_interger.QuadPart);
	LARGE_INTEGER  large_interger1;
	LARGE_INTEGER  large_interger2;
	QueryPerformanceCounter(&large_interger1);
	if (!uECC_verify(publicKey, hash, sizeof(hash), sig, curves[c])) {
		printf("uECC_verify() failed\n");
		return 1;
	}
	QueryPerformanceCounter(&large_interger2);
	__int64  x = large_interger2.QuadPart - large_interger1.QuadPart;
	printf("%lf\n", x * 1000 / dff);

	auto tcpServer = nicehero::TcpServer("0.0.0.0", 7000);
	auto privateKey1 = tcpServer.GetPrivateKeyString();
	tcpServer.SetPrivateKeyString(privateKey1);
	auto privateKey2 = tcpServer.GetPrivateKeyString();
	nicehero::post([] {
		nlog("post test");
	});
	nicehero::start();
	return 0;
}
