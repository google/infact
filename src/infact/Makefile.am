AM_CPPFLAGS = -I. -Wall

testdir=${exec_prefix}/test-bin
test_PROGRAMS = bin/stream-tokenizer-test \
		bin/environment-test \
		bin/interpreter-test

SRCS =  stream-tokenizer.cc environment.cc environment-impl.cc factory.cc \
	interpreter.cc example.cc

lib_LIBRARIES = lib/libinfact.a
lib_libinfact_a_SOURCES = $(SRCS)

# The following are test executables (similar to unit tescibts).
bin_stream_tokenizer_test_SOURCES = $(SRCS) stream-tokenizer-test.cc
bin_environment_test_SOURCES = $(SRCS) environment-test.cc
bin_interpreter_test_SOURCES = $(SRCS) interpreter-test.cc
