all:	pxit-encoder pxit-decoder pxit-scope

pxit-encoder:
	mkdir -p bin
	g++ -o bin/pxit-encoder pxit-encoder.cpp TargaImage.cpp Checksum.cpp

pxit-decoder:
	mkdir -p bin
	g++ -o bin/pxit-decoder pxit-decoder.cpp ImageProcessor.cpp TargaImage.cpp Checksum.cpp

pxit-scope:
	mkdir -p bin
	g++ -o bin/pxit-scope pxit-scope.cpp TargaImage.cpp

