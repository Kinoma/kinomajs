#makefile for TomsFastMath
#
#
VERSION=0.09

CFLAGS += -Wall -W -Wshadow -I./ 

ifndef MAKE
   MAKE=make
endif

ifndef IGNORE_SPEED

CFLAGS += -O3 -funroll-loops

#profiling
#PROF=-pg -g
#CFLAGS += $(PROF)

#speed
CFLAGS += -fomit-frame-pointer

endif

OBJECTS = \
fp_set.o \
\
fp_rshd.o fp_lshd.o fp_div_2d.o fp_mod_2d.o fp_mul_2d.o fp_2expt.o \
fp_mul_2.o fp_div_2.o  \
\
fp_cnt_lsb.o \
\
fp_add.o fp_sub.o fp_mul.o fp_sqr.o fp_div.o fp_mod.o \
s_fp_add.o s_fp_sub.o \
\
fp_cmp_d.o fp_add_d.o fp_sub_d.o fp_mul_d.o fp_div_d.o fp_mod_d.o \
fp_addmod.o fp_submod.o fp_mulmod.o fp_sqrmod.o fp_invmod.o \
fp_gcd.o fp_lcm.o fp_prime_miller_rabin.o fp_isprime.o \
fp_prime_random_ex.o fp_mul_comba.o fp_sqr_comba.o \
\
fp_montgomery_setup.o fp_montgomery_calc_normalization.o fp_montgomery_reduce.o \
\
fp_exptmod.o \
\
fp_cmp.o fp_cmp_mag.o \
\
fp_unsigned_bin_size.o fp_read_unsigned_bin.o fp_to_unsigned_bin.o \
fp_signed_bin_size.o fp_read_signed_bin.o fp_to_signed_bin.o \
fp_read_radix.o fp_toradix.o fp_radix_size.o fp_count_bits.o fp_reverse.o fp_s_rmap.o \
\
fp_ident.o 

HEADERS=tfm.h

ifndef LIBPATH
   LIBPATH=/usr/lib
endif

ifndef INCPATH
   INCPATH=/usr/include
endif

ifndef INSTALL_GROUP
   GROUP=wheel
else
   GROUP=$(INSTALL_GROUP)
endif

ifndef INSTALL_USER
   USER=root
else
   USER=$(INSTALL_USER)
endif

ifndef LIBNAME
	LIBNAME=libtfm.a
endif

default: $(LIBNAME)

$(LIBNAME): $(OBJECTS)
	$(AR) $(ARFLAGS) $@ $(OBJECTS)
	ranlib $@

install: $(LIBNAME)
	install -d -g $(GROUP) -o $(USER) $(DESTDIR)$(LIBPATH)
	install -d -g $(GROUP) -o $(USER) $(DESTDIR)$(INCPATH)
	install -g $(GROUP) -o $(USER) $(LIBNAME) $(DESTDIR)$(LIBPATH)
	install -g $(GROUP) -o $(USER) $(HEADERS) $(DESTDIR)$(INCPATH)

mtest/mtest: mtest/mtest.o
	cd mtest ; CFLAGS="$(CFLAGS) -I../" MAKE=${MAKE} ${MAKE} mtest

test: $(LIBNAME) demo/test.o mtest/mtest
	$(CC) $(CFLAGS) demo/test.o $(LIBNAME) $(PROF) -o test

timing: $(LIBNAME) demo/test.o
	$(CC) $(CFLAGS) demo/test.o $(LIBNAME) $(PROF) -o test
	
stest: $(LIBNAME) demo/stest.o 
	$(CC) $(CFLAGS) demo/stest.o $(LIBNAME) -o stest

rsatest: $(LIBNAME) demo/rsa.o
	$(CC) $(CFLAGS) demo/rsa.o $(LIBNAME) -o rsatest

docdvi: tfm.tex
	touch tfm.ind
	latex tfm >/dev/null
	latex tfm >/dev/null
	makeindex tfm
	latex tfm >/dev/null

docs: docdvi
	latex tfm >/dev/null
	dvipdf tfm
	mv -f tfm.pdf doc

#This rule cleans the source tree of all compiled code, not including the pdf
#documentation.
clean:
	rm -f `find . -type f | grep "[.]o" | xargs`
	rm -f `find . -type f | grep "[.]lo"  | xargs`
	rm -f `find . -type f | grep "[.]a" | xargs`
	rm -f `find . -type f | grep "[.]la"  | xargs`
	rm -f `find . -type f | grep "[.]obj" | xargs`
	rm -f `find . -type f | grep "[.]lib" | xargs`
	rm -f `find . -type f | grep "[.]exe" | xargs`
	rm -f `find . -type f | grep "[.]gcda" | xargs`
	rm -f `find . -type f | grep "[.]gcno" | xargs`
	rm -f `find . -type f | grep "[.]il" | xargs`
	rm -f `find . -type f | grep "[.]dyn" | xargs`
	rm -f `find . -type f | grep "[.]dpi" | xargs`
	rm -rf `find . -type d | grep "[.]libs" | xargs`
	rm -f tfm.aux  tfm.dvi  tfm.idx  tfm.ilg  tfm.ind  tfm.lof  tfm.log  tfm.toc 
	cd mtest ; MAKE=${MAKE} ${MAKE} clean

no_oops: clean
	cd .. ; cvs commit
	echo Scanning for scratch/dirty files
	find . -type f | grep -v CVS | xargs -n 1 bash mess.sh

zipup: no_oops docs clean
	perl gen.pl ; mv mpi.c pre_gen/ ; \
	cd .. ; rm -rf tfm* tomsfastmath-$(VERSION) ; mkdir tomsfastmath-$(VERSION) ; \
	cp -R ./tomsfastmath/* ./tomsfastmath-$(VERSION)/ ; \
	tar -c tomsfastmath-$(VERSION)/* | bzip2 -9vvc > tfm-$(VERSION).tar.bz2 ; \
	zip -9r tfm-$(VERSION).zip tomsfastmath-$(VERSION)/* ; \
	mv -f tfm* ~ ; rm -rf tomsfastmath-$(VERSION)

# $Source$ 
# $Revision$ 
# $Date$ 
