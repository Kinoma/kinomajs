CFLAGS="-arch ppc -arch i386 -isysroot /Developer/SDKs/MacOSX10.4u.sdk" make
if [ -f libtommath.a ]; then mkdir -p mac; mv -f libtommath.a mac; fi
