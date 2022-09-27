COMPONENTS :=  \
libsylixos \
libcextern \
libreadline \
libsqlite3 \
pciutils \
openssl \
libgtest

all: $(COMPONENTS)
	@for target in $(COMPONENTS); do make -C $$target all; done

clean: $(COMPONENTS)
	@for target in $(COMPONENTS); do make -C $$target clean; done
