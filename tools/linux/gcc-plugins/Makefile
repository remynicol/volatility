all: debug-randstruct.so check

debug-randstruct.so: debug-randstruct.cpp
	$(CXX) -I`g++ -print-file-name=plugin`/include -fPIC -shared -o $@ $<

check: debug-randstruct.so
	$(CXX) -fplugin=./$< -c -x c++ /dev/null -o /dev/null

clean:
	rm -f debug-randstruct.so
