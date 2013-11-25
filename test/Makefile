CPPFLAGS= -I/home/users/zhangli10/third-64/gtest/include -I.. -Wall -g3
LDFLAGS=  -L/home/users/zhangli10/third-64/gtest/lib -lgtest -lgtest_main -lpthread

build: zling_test

clean:
	rm -f zling_test

zling_test: \
	zling_codebuf_test.cpp \
	zling_lz_test.cpp \
	../src/zling_lz.cpp
	$(CXX) -o $@ $^ $(CPPFLAGS) $(LDFLAGS)

.PHONY: build
.PHONY: clean
