#! /bin/sh
set -eux

if [ -z ${1} ] 
then
    echo "Destination must be provided"
    exit 1
fi

apt-get -y install --no-install-recommends libksba-dev libgpg-error-dev libgcrypt-dev libassuan-dev  libnpth-dev libgnutls28-dev pkg-config libldap-dev wget ca-certificates bzip2 patch texinfo

DESTINATION=${1}
if [ -e "${DESTINATION}/bin/gpg" ]
then
   echo "Already installed"
   exit 0
fi

if [ -e "${DESTINATION}" ]
then
   echo "Please use a nonexisting destination"
   exit 1
fi

GNUPG_VERSION=2.4.1
GPGME_VERSION=1.19.0

WORKDIR=$(mktemp -d)

cd ${WORKDIR}

wget https://gnupg.org/ftp/gcrypt/gnupg/gnupg-${GNUPG_VERSION}.tar.bz2
tar xf gnupg-${GNUPG_VERSION}.tar.bz2
wget https://gnupg.org/ftp/gcrypt/gpgme/gpgme-${GPGME_VERSION}.tar.bz2
tar xf gpgme-${GPGME_VERSION}.tar.bz2


mkdir -p ${WORKDIR}/gnupg-${GNUPG_VERSION}/build
cd gnupg-${GNUPG_VERSION}
cd build
../configure --prefix=${DESTINATION}
make install

cd ${WORKDIR}

mkdir gpgme-${GPGME_VERSION}/build
cd gpgme-${GPGME_VERSION}/build
../configure --prefix=${DESTINATION} --enable-fixed-path=${DESTINATION}/bin --enable-languages=cpp
PATH=${DESTINATION}/bin:$PATH make -j5 install


