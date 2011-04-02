snp: *.cpp
	g++ -O3 *.cpp -lsnappy -o snp

test: snp
	./snp < snp.cpp > snp.cpp.snp
	./snp -d < snp.cpp.snp | cmp - snp.cpp
	rm snp.cpp.snp

