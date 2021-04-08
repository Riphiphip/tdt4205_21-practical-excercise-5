TARGETS=$(shell ls *.vsl | sed s/\.vsl/\.tree/g)
all: ${TARGETS}
%: %.tree
%.tree: %.vsl
	../src/vslc <$*.vsl > $*.tree
clean:
	-rm -f *.tree
purge: clean
	-rm -f ${TARGETS}
