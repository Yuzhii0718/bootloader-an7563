#!/bin/sh

if [ -d "mbedtls-2.18.0" ]; then
	exit 0
fi

git clone https://github.com/Mbed-TLS/mbedtls.git mbedtls-2.18.0
if [ -d "mbedtls-2.18.0" ]; then
	cd mbedtls-2.18.0
	git checkout 85da85555e5b086b0250780693c3ee584f63e79f
	exit 0
fi

git clone https://fuchsia.googlesource.com/third_party/github.com/ARMmbed/mbedtls mbedtls-2.18.0
if [ -d "mbedtls-2.18.0" ]; then
	cd mbedtls-2.18.0
	git checkout 85da85555e5b086b0250780693c3ee584f63e79f
	exit 0
fi

git clone https://gerrit.mediatek.inc/platform/external/mbedtls mbedtls-2.18.0
if [ -d "mbedtls-2.18.0" ]; then
	cd mbedtls-2.18.0
	git checkout 85da85555e5b086b0250780693c3ee584f63e79f
	exit 0
fi

